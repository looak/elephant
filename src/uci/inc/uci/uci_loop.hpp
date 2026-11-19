#pragma once

// Speaks UCI on standard input & output: announces the engine and its options, then answers commands. quit ends the
// process. Returns when standard input closes, the GUI is gone and nothing more can arrive.
void runUciLoop();
