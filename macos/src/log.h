#pragma once

// Appends a line to HammerillTrainerVC.log in the game's Resources folder and
// echoes it to stderr.
void Log(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
