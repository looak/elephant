#!/usr/bin/env python3
"""Writes VS Code workspaces for this machine: one with everything, one per target.

    python gen/generate_workspace.py                   every workspace, for this OS
    python gen/generate_workspace.py all uci           only these
    python gen/generate_workspace.py --os linux        for another OS

Then open one: code --profile "C++" .vscode/workspaces/win/uci.code-workspace. VS Code keeps a profile per workspace, so
it's only needed once, and without it a new workspace gets the default profile. CMake Tools asks for a kit the first
time, take the same one in each, they all share a build tree. The Debug or Release variant in its status bar decides
which binary a launch runs, Debug on a fresh workspace, and a launch builds it first.

A workspace is generated rather than committed since a launch configuration's debugger can't depend on the OS, and what
it hides is a list of what's on disk: run this again after adding a folder. Needs the extensions ms-vscode.cmake-tools
(1.24 or newer) and ms-vscode.cpptools. Standard library only.
"""
import argparse
import json
import os
import platform

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FOLDER = "elephant"

INDEXER_TESTS = "DrawFixture.*:GameIndexFixture.*:IndexerInput.*:PolyglotFixture.*"
UCI_TESTS = "Uci*"

# A launch configuration is its name, the CMake target and the arguments. CMake Tools finds the target's file: eg-uci's
# name has the version in it, which changes on every merge.
UCI = ("eg-uci", "eg-uci", [])
BENCH = ("eg-uci bench", "eg-uci", ["bench"])
CLI = ("ElephantGambit", "ElephantGambit", [])
TUNER = ("ElephantTuner", "ElephantTuner", [])
INDEXER = ("phant-indexer", "PhantIndexer", ["--help"])
SUITE = ("ElephantSuite", "ElephantSuite", [])
ALL_TESTS = ("ElephantTest", "ElephantTest", [])
ENGINE_TESTS = ("ElephantTest, engine", "ElephantTest", [f"--gtest_filter=-{UCI_TESTS}:{INDEXER_TESTS}"])
ONLY_UCI_TESTS = ("ElephantTest, uci", "ElephantTest", [f"--gtest_filter={UCI_TESTS}"])
ONLY_INDEXER_TESTS = ("ElephantTest, indexer", "ElephantTest", [f"--gtest_filter={INDEXER_TESTS}"])

# ElephantTest is one binary over every target, so a target's tests are its files and a filter. The engine is a library,
# not a binary, but it's where most of the work is. Every workspace is the whole repository with the rest hidden: a target's
# CMakeLists.txt can't configure on its own, and go to definition still reaches what's hidden.
WORKSPACES = {
    "all": {"launch": [UCI, BENCH, CLI, TUNER, INDEXER, ALL_TESTS, SUITE]},
    "engine": {
        "show": ["src/engine", "test"],
        "hide": ["test/src/indexer", "test/src/io/uci_test.cpp"],
        "launch": [BENCH, UCI, ENGINE_TESTS, SUITE],
    },
    "uci": {"show": ["src/uci", "test/src/io/uci_test.cpp"], "launch": [UCI, BENCH, ONLY_UCI_TESTS]},
    "cli": {"show": ["src/cli"], "launch": [CLI]},
    "tuner": {"show": ["src/tuner"], "launch": [TUNER]},
    "indexer": {
        "show": ["src/indexer", "test/src/indexer", "test/pgn", "test/indexer_smoke.py"],
        "launch": [INDEXER, ONLY_INDEXER_TESTS],
    },
}

ALWAYS_HIDDEN = [".build", ".vs", "logs"]


def host_os():
    return {"Windows": "win", "Darwin": "mac"}.get(platform.system(), "linux")


def inside(path, parent):
    return path == parent or path.startswith(parent + "/")


def hidden_paths(show, hide, directory=""):
    """What to hide so that only show is left, the files at the top of the repository staying."""
    found = []
    for entry in sorted(os.listdir(os.path.join(ROOT, directory))):
        path = f"{directory}/{entry}" if directory else entry
        is_dir = os.path.isdir(os.path.join(ROOT, path))
        if entry == ".git" or (not directory and not is_dir):
            continue
        if any(inside(path, parent) for parent in hide):
            found.append(path)
        elif any(inside(path, shown) for shown in show):
            if is_dir and any(inside(parent, path) for parent in hide):
                found += hidden_paths(show, hide, path)
        elif is_dir and any(inside(shown, path) for shown in show):
            found += hidden_paths(show, hide, path)
        else:
            found.append(path)
    return found


def debugger(target_os):
    if target_os == "win":
        return {"type": "cppvsdbg", "console": "integratedTerminal"}
    return {"type": "cppdbg", "MIMode": "lldb" if target_os == "mac" else "gdb", "externalConsole": False}


def launch_section(configurations, target_os):
    """The launch configurations and, per target, the input that looks its file up by name."""
    # a workspace's own configurations aren't in a folder, their variables have to name one.
    cwd = f"${{workspaceFolder:{FOLDER}}}"
    return {
        "version": "0.2.0",
        "configurations": [
            {"name": name, "request": "launch", "program": f"${{input:{target}}}", "args": args, "cwd": cwd,
             "stopAtEntry": False, **debugger(target_os)}
            for name, target, args in configurations
        ],
        "inputs": [
            {"id": target, "type": "command", "command": "cmake.launchTargetPath", "args": {"targetName": target}}
            for target in sorted({target for _, target, _ in configurations})
        ],
    }


def workspace(spec, target_os):
    if "show" in spec:
        hidden = hidden_paths(spec["show"], spec.get("hide", []))
    else:
        hidden = ALWAYS_HIDDEN
    return {
        "folders": [{"name": FOLDER, "path": "../../.."}],
        "settings": {
            "cmake.generator": "Ninja",
            # with the OS in it: the same checkout can be open from Windows and from WSL.
            "cmake.buildDirectory": f"${{workspaceFolder}}/.build/vscode-{target_os}-${{buildType}}",
            "cmake.configureSettings": {
                "ELEPHANT_BUILD_TESTS": "ON",
                "ELEPHANT_BUILD_TEST_SUITE": "ON",
                "ELEPHANT_BUILD_TUNER": "ON",
                "ELEPHANT_BUILD_INDEXER": "ON",
            },
            "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
            "editor.rulers": [120],
            "files.exclude": {path: True for path in hidden},
        },
        "launch": launch_section(spec["launch"], target_os),
        "extensions": {"recommendations": ["ms-vscode.cmake-tools", "ms-vscode.cpptools"]},
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n", maxsplit=1)[0])
    parser.add_argument("names", nargs="*", help=f"any of {', '.join(WORKSPACES)}; all of them by default")
    parser.add_argument("--os", choices=["win", "linux", "mac"], default=host_os(), help="this machine's by default")
    args = parser.parse_args()
    for name in args.names:
        if name not in WORKSPACES:
            parser.error(f"no workspace {name}, there's {', '.join(WORKSPACES)}")

    directory = os.path.join(ROOT, ".vscode", "workspaces", args.os)
    os.makedirs(directory, exist_ok=True)
    for name in args.names or WORKSPACES:
        path = os.path.join(directory, f"{name}.code-workspace")
        with open(path, "w", encoding="utf-8", newline="\n") as file:
            json.dump(workspace(WORKSPACES[name], args.os), file, indent=4)
            file.write("\n")
        print(os.path.relpath(path, ROOT).replace("\\", "/"))


if __name__ == "__main__":
    main()
