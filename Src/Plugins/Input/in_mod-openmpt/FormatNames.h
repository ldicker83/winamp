#pragma once

// Returns the name of the module format that uses a file extension (given without the dot,
// any case), e.g. "Impulse Tracker" for "it", or nullptr for an extension not in the table.
const char *GetModFormatName(const char *extension);
const char *GetModFormatName(const wchar_t *extension);
