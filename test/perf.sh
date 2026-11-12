#!/usr/bin/env bash
# Compares the speed of two builds that search the same tree: a refactor, or anything else that shouldn't change the bench.
# Strength is OpenBench's question, this is for what it can't see in time: a layout change in the hot path.
#
#   test/perf.sh <A> <B> [rounds]     A & B take turns on one pinned core, so a busy or warm machine hits both alike
#   test/perf.sh <A> [rounds]         only A's numbers
#
# Compare builds made the same way, ob_build/makefile and g++ is what OpenBench and releases run. Compare a build with itself
# for the machine's noise floor: a difference smaller than that isn't one. Nodes are deterministic on one thread, so they
# have to match between A & B too, or it isn't a refactor. Needs Linux for taskset, on Windows run it in WSL.
set -u

a="$1"
b=""
rounds=5
if [ $# -ge 2 ]; then
    case "$2" in
        *[!0-9]*) b="$2"; rounds="${3:-5}" ;;
        *) rounds="$2" ;;
    esac
fi
core="${PERF_CORE:-4}"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

# Run from names of the same length and take turns going first. A longer path alone shifts the stack and with it the
# alignment of everything on it: a build against a copy of itself under another name lost every round.
shown_a="$a"
shown_b="$b"
cp "$a" "$tmp/engine-a"
a="$tmp/engine-a"
if [ -n "$b" ]; then
    cp "$b" "$tmp/engine-b"
    b="$tmp/engine-b"
fi

# name, position, depth: about 1 to 3 seconds each, so A against B over 5 rounds takes under 2 minutes. Depths fitted on
# fad9d8e (singular extensions); a search change that grows the tree a lot needs them looked at again, the endgame
# jumped from 1.3 to 4.4 seconds between depth 22 and 23.
workload=(
    "startpos|startpos|17"
    "kiwipete|fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1|16"
    "middlegame|fen r1bq1rk1/pp2b1pp/n1pp1n2/3P1p2/2P1p3/2N1P2N/PP2BPPP/R1BQ1RK1 b - - 2 10|16"
    "endgame|fen 8/8/1p2k1p1/3p3p/1p1P1P1P/1P2PK2/8/8 w - - 3 54|22"
)

# prints "<nodes> <ms>" of one search.
search() {
    local fifo="$tmp/in"
    mkfifo "$fifo"
    taskset -c "$core" "$1" < "$fifo" 2>/dev/null | {
        exec 3> "$fifo"
        printf 'setoption name Hash value 64\nucinewgame\nposition %s\ngo depth %s\n' "$2" "$3" >&3
        while IFS= read -r line; do
            case "$line" in
                "info nodes "*) set -- $line; echo "$3 $5" ;;
                bestmove*) echo quit >&3; break ;;
            esac
        done
        exec 3>&-
    }
    rm -f "$fifo"
}

median() { sort -n | awk '{ v[NR] = $1 } END { print v[int((NR + 1) / 2)] }'; }

echo "cpu: $(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2- | sed 's/^ //'), core $core, $rounds rounds"
echo "A: $shown_a"
[ -n "$b" ] && echo "B: $shown_b"

for ((round = 1; round <= rounds; ++round)); do
    for entry in "${workload[@]}"; do
        IFS='|' read -r name position depth <<< "$entry"
        if [ -n "$b" ] && [ $((round % 2)) = 0 ]; then
            search "$b" "$position" "$depth" >> "$tmp/$name.b"
            search "$a" "$position" "$depth" >> "$tmp/$name.a"
        else
            search "$a" "$position" "$depth" >> "$tmp/$name.a"
            [ -n "$b" ] && search "$b" "$position" "$depth" >> "$tmp/$name.b"
        fi
    done
done

if [ -n "$b" ]; then
    printf '%-11s %11s %11s %11s %8s  %s\n' workload nodes 'A nps' 'B nps' 'B - A' 'rounds B won'
else
    printf '%-11s %11s %11s %11s %11s\n' workload nodes 'best nps' 'median nps' 'worst nps'
fi

sum_a=0; sum_b=0; sum_nodes=0; mismatch=0
for entry in "${workload[@]}"; do
    IFS='|' read -r name _ _ <<< "$entry"
    nodes="$(head -n 1 "$tmp/$name.a" | cut -d' ' -f1)"
    [ "$(cut -d' ' -f1 "$tmp/$name.a" | sort -u | wc -l)" = 1 ] || { echo "$name: A's nodes vary between runs"; mismatch=1; }
    ms_a="$(cut -d' ' -f2 "$tmp/$name.a" | median)"
    sum_nodes=$((sum_nodes + nodes)); sum_a=$((sum_a + ms_a))
    if [ -n "$b" ]; then
        nodes_b="$(head -n 1 "$tmp/$name.b" | cut -d' ' -f1)"
        [ "$nodes" = "$nodes_b" ] || { echo "$name: nodes differ, A $nodes, B $nodes_b, this isn't a refactor"; mismatch=1; }
        ms_b="$(cut -d' ' -f2 "$tmp/$name.b" | median)"
        sum_b=$((sum_b + ms_b))
        wins="$(paste -d' ' "$tmp/$name.a" "$tmp/$name.b" | awk '$4 < $2 { n++ } END { print n + 0 }')"
        printf '%-11s %11s %11s %11s %+7.2f%%  %s of %s\n' "$name" "$nodes" "$((nodes * 1000 / ms_a))" "$((nodes_b * 1000 / ms_b))" \
            "$(awk -v a="$ms_a" -v b="$ms_b" 'BEGIN { print (a / b - 1) * 100 }')" "$wins" "$rounds"
    else
        sorted="$(cut -d' ' -f2 "$tmp/$name.a" | sort -n)"
        printf '%-11s %11s %11s %11s %11s\n' "$name" "$nodes" "$((nodes * 1000 / $(head -n 1 <<< "$sorted")))" "$((nodes * 1000 / ms_a))" "$((nodes * 1000 / $(tail -n 1 <<< "$sorted")))"
    fi
done

if [ -n "$b" ]; then
    printf 'total: %s nodes, A %s nps, B %s nps, B - A %+.2f%%\n' "$sum_nodes" "$((sum_nodes * 1000 / sum_a))" "$((sum_nodes * 1000 / sum_b))" \
        "$(awk -v a="$sum_a" -v b="$sum_b" 'BEGIN { print (a / b - 1) * 100 }')"
else
    echo "total: $sum_nodes nodes, $((sum_nodes * 1000 / sum_a)) nps by medians"
fi
exit "$mismatch"
