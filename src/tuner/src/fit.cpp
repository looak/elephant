#include <fit.hpp>

#include <algorithm>
#include <cmath>
#include <thread>

#include <find_k.hpp>

namespace tuner {

Values Values::from(const std::vector<TaperedScore>& scores) {
    Values values;
    values.mg.reserve(scores.size());
    values.eg.reserve(scores.size());
    for (TaperedScore score : scores) {
        values.mg.push_back(mg_score(score));
        values.eg.push_back(eg_score(score));
    }
    return values;
}

std::vector<TaperedScore> Values::rounded() const {
    std::vector<TaperedScore> scores;
    scores.reserve(mg.size());
    for (std::size_t i = 0; i < mg.size(); ++i)
        scores.push_back(S(static_cast<i32>(std::lround(mg[i])), static_cast<i32>(std::lround(eg[i]))));
    return scores;
}

namespace {

constexpr double c_ln10 = 2.302585092994046;

// a position's share of midgame & endgame: phase 24 is all midgame, 0 all endgame.
struct PhaseShares {
    double midgame;
    double endgame;
};

PhaseShares phaseShares(const TunerEntry& entry) {
    const double midgame = static_cast<double>(entry.phase) / evaluator_data::maxGamePhase;
    return { midgame, 1.0 - midgame };
}

// a position's evaluation from the values, white's side:
//     sum of coefficient * (mg * midgame share + eg * endgame share), plus the untuned offset.
double evaluate(const TunerData& data, const TunerEntry& entry, const Values& values) {
    const PhaseShares share = phaseShares(entry);
    double eval = entry.fixedOffset;
    for (u32 i = 0; i < entry.coefficientCount; ++i) {
        const Coefficient c = data.coefficients[entry.coefficientStart + i];
        eval += c.value() * (values.mg[c.index()] * share.midgame + values.eg[c.index()] * share.endgame);
    }
    return eval;
}

// what one thread adds up over its slice of the positions in a pass.
struct PassSums {
    double squaredMisses = 0.0;     // sum of (result - expected)^2, the error before dividing by the positions
    std::vector<double> mgGradient;
    std::vector<double> egGradient;
};

// one pass over the positions in [begin, end): their error, and each number's gradient, as in fit.hpp.
void sumPass(const TunerData& data, const Values& values, double k, std::size_t begin, std::size_t end,
             PassSums& sums) {
    sums.squaredMisses = 0.0;
    sums.mgGradient.assign(values.mg.size(), 0.0);
    sums.egGradient.assign(values.eg.size(), 0.0);

    for (std::size_t p = begin; p < end; ++p) {
        const TunerEntry& entry = data.entries[p];

        // 1. how far the prediction was from the game: positive, the evaluation was too pessimistic for white.
        const double expected = expectedResult(evaluate(data, entry, values), k);
        const double miss = entry.result - expected;
        sums.squaredMisses += miss * miss;

        // 2. how much the prediction moves per centipawn of eval, the steepness of the curve at this eval.
        const double slope = expected * (1.0 - expected) * k * c_ln10 / 400.0;

        // 3. how much this position's error moves per centipawn of eval: d(result - expected)^2 / d eval.
        const double errorPerCentipawn = -2.0 * miss * slope;

        // 4. each number this position uses gets that, by how much it counts here: its coefficient and phase share.
        const PhaseShares share = phaseShares(entry);
        for (u32 i = 0; i < entry.coefficientCount; ++i) {
            const Coefficient c = data.coefficients[entry.coefficientStart + i];
            const double perNumber = errorPerCentipawn * c.value();
            sums.mgGradient[c.index()] += perNumber * share.midgame;
            sums.egGradient[c.index()] += perNumber * share.endgame;
        }
    }
}

// a pass split over threads, each its own slice and its own sums, added in slice order so the result doesn't depend
// on the number of threads. Gradients are divided by the positions, like the error, to be per position on average.
double pass(const TunerData& data, const Values& values, double k, std::size_t threads,
            std::vector<double>& mgGradient, std::vector<double>& egGradient) {
    const std::size_t count = data.entries.size();
    threads = std::clamp<std::size_t>(threads, 1, std::max<std::size_t>(count, 1));
    std::vector<PassSums> slices(threads);
    {
        std::vector<std::jthread> workers;
        workers.reserve(threads);
        for (std::size_t t = 0; t < threads; ++t) {
            workers.emplace_back([&, t] {
                sumPass(data, values, k, count * t / threads, count * (t + 1) / threads, slices[t]);
            });
        }
    }   // joins the workers

    double squaredMisses = 0.0;
    mgGradient.assign(values.mg.size(), 0.0);
    egGradient.assign(values.eg.size(), 0.0);
    for (const PassSums& slice : slices) {
        squaredMisses += slice.squaredMisses;
        for (std::size_t i = 0; i < mgGradient.size(); ++i) {
            mgGradient[i] += slice.mgGradient[i];
            egGradient[i] += slice.egGradient[i];
        }
    }
    const double positions = static_cast<double>(std::max<std::size_t>(count, 1));
    for (std::size_t i = 0; i < mgGradient.size(); ++i) {
        mgGradient[i] /= positions;
        egGradient[i] /= positions;
    }
    return squaredMisses / positions;
}

// Adam's memory for one number: the running averages of its gradient and of its gradient squared.
struct Adam {
    static constexpr double c_beta1 = 0.9;     // how long the gradient's average remembers, ~10 passes
    static constexpr double c_beta2 = 0.999;   // how long the squared gradient's average remembers, ~1000 passes
    static constexpr double c_epsilon = 1e-8;  // keeps the division safe while the averages are still ~0

    double average = 0.0;
    double averageSquare = 0.0;

    // the step for this pass's gradient, the number moves by minus this.
    double step(double gradient, double rate, std::size_t passes) {
        average = c_beta1 * average + (1.0 - c_beta1) * gradient;
        averageSquare = c_beta2 * averageSquare + (1.0 - c_beta2) * gradient * gradient;
        // both averages start at 0 and lean towards it for the first passes, dividing by 1 - beta^passes undoes that.
        const double correctedAverage = average / (1.0 - std::pow(c_beta1, static_cast<double>(passes)));
        const double correctedSquare = averageSquare / (1.0 - std::pow(c_beta2, static_cast<double>(passes)));
        // a steady push moves about `rate`, a push that keeps changing sign moves less.
        return rate * correctedAverage / (std::sqrt(correctedSquare) + c_epsilon);
    }
};

// how much each value is used over the positions, the sum of its coefficients' sizes: a count most pieces have
// weighs a lot, a count that hardly ever happens next to nothing.
std::vector<double> usage(const TunerData& data, std::size_t values) {
    std::vector<double> used(values, 0.0);
    for (const Coefficient& c : data.coefficients)
        used[c.index()] += std::abs(c.value());
    return used;
}

// a table's average, weighted by how much each of its values is used, midgame & endgame.
struct Average {
    double mg = 0.0;
    double eg = 0.0;
};

// the values from first to first + size, a table in params:: layout.
struct Table {
    u32 first;
    u32 size;
};

Average average(const Values& values, const std::vector<double>& used, Table table) {
    double weight = 0.0;
    Average sum;
    for (u32 i = table.first; i < table.first + table.size; ++i) {
        weight += used[i];
        sum.mg += used[i] * values.mg[i];
        sum.eg += used[i] * values.eg[i];
    }
    return weight == 0.0 ? Average{} : Average{ sum.mg / weight, sum.eg / weight };
}

// Moves a table's values up or down, the kept ones excepted, so its usage weighted average is back at target. A
// table whose values are all kept, or unused, stays as it is.
void centre(Values& values, const std::vector<double>& used, const std::vector<bool>& kept, Table table,
            Average target) {
    double weight = 0.0, movable = 0.0;
    Average sum;
    for (u32 i = table.first; i < table.first + table.size; ++i) {
        weight += used[i];
        sum.mg += used[i] * values.mg[i];
        sum.eg += used[i] * values.eg[i];
        if (!kept[i])
            movable += used[i];
    }
    if (movable == 0.0)
        return;
    const double mgShift = (target.mg * weight - sum.mg) / movable;
    const double egShift = (target.eg * weight - sum.eg) / movable;
    for (u32 i = table.first; i < table.first + table.size; ++i) {
        if (kept[i])
            continue;
        values.mg[i] += mgShift;
        values.eg[i] += egShift;
    }
}

// the tables kept centred and where: every piece's mobility table at 0, see "What's kept fixed" in fit.hpp, and every
// tuned piece square table on the average it starts with.
struct Centred {
    Table table;
    Average target;
};

std::vector<Centred> centredTables(const Values& values, const std::vector<double>& used,
                                   const FitSettings& settings) {
    std::vector<Centred> tables;
    if (settings.centreMobility && settings.tunes(params::Group::Mobility)) {
        for (u32 pieceId = 1; pieceId <= 4; ++pieceId)
            tables.push_back({ { params::mobility(pieceId, 0), params::mobilityCounts[pieceId] }, Average{} });
    }
    if (settings.centrePsqt) {
        for (u32 pieceId = 0; pieceId < 6; ++pieceId) {
            const Table table{ params::psqt(pieceId, 0), 64 };
            if (settings.tunes(params::groupOf(table.first)))
                tables.push_back({ table, average(values, used, table) });
        }
    }
    return tables;
}

} // namespace

std::vector<std::size_t> positionsUsing(const TunerData& data) {
    std::vector<std::size_t> positions(params::count, 0);
    for (const Coefficient& c : data.coefficients)
        ++positions[c.index()];
    return positions;
}

double fitError(const TunerData& data, const Values& values, double k, std::size_t threads) {
    std::vector<double> unusedMg, unusedEg;
    return pass(data, values, k, threads, unusedMg, unusedEg);
}

void fit(const TunerData& data, Values& values, const FitSettings& settings,
         const std::function<void(std::size_t epoch, double error)>& progress) {
    std::vector<Adam> mgAdam(values.mg.size());
    std::vector<Adam> egAdam(values.eg.size());
    std::vector<double> mgGradient, egGradient;
    const std::vector<double> used = usage(data, values.mg.size());
    // values too few positions use are kept as they are, see "What's kept fixed" in fit.hpp.
    std::vector<bool> kept(values.mg.size(), false);
    if (settings.minUsage > 0) {
        const std::vector<std::size_t> positions = positionsUsing(data);
        for (std::size_t i = 0; i < kept.size(); ++i)
            kept[i] = positions[i] < settings.minUsage;
    }
    const std::vector<Centred> centred = centredTables(values, used, settings);
    for (const Centred& c : centred)
        centre(values, used, kept, c.table, c.target);

    for (std::size_t epoch = 1; epoch <= settings.epochs; ++epoch) {
        const double error = pass(data, values, settings.k, settings.threads, mgGradient, egGradient);

        for (std::size_t i = 0; i < values.mg.size(); ++i) {
            if (!settings.tunes(params::groupOf(static_cast<u32>(i))) || kept[i])
                continue;   // kept as it is, see "What's kept fixed" in fit.hpp
            // against the gradient: the direction that lowers the error.
            values.mg[i] -= mgAdam[i].step(mgGradient[i], settings.rate, epoch);
            values.eg[i] -= egAdam[i].step(egGradient[i], settings.rate, epoch);
        }
        // the step may have moved a table's average, it's put back so only the table's shape changes.
        for (const Centred& c : centred)
            centre(values, used, kept, c.table, c.target);

        progress(epoch, error);
    }
}

} // namespace tuner
