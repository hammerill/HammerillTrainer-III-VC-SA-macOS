// Trainer data (vehicle/weapon/location lists, font) compiled into the plugin
// from resource/, so nothing has to be copied next to the game.
#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace data
{
struct Entry
{
    std::string key;
    std::string value;
};

struct Category
{
    std::string name; // empty for entries before the first [section]
    std::vector<Entry> entries;
};

// Embedded file contents by name ("vehicles.toml", "text.ttf", ...).
// A file with the same name in HammerillTrainerVC/ inside the game's Resources
// folder takes precedence, so lists can be customised without rebuilding.
std::string_view File(const char *name);

// Parses the flat TOML subset the trainer data files use:
// [Category] headers and "key" = "value" lines.
std::vector<Category> ParseToml(std::string_view text);
} // namespace data
