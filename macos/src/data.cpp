#include "data.h"

#include <cctype>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>

#include "embedded_files.h"

namespace data
{
namespace
{
std::string_view Trim(std::string_view s)
{
    while (!s.empty() && isspace(static_cast<unsigned char>(s.front())))
        s.remove_prefix(1);
    while (!s.empty() && isspace(static_cast<unsigned char>(s.back())))
        s.remove_suffix(1);
    return s;
}

std::string Unquote(std::string_view s)
{
    s = Trim(s);
    if (s.size() >= 2 && (s.front() == '"' || s.front() == '\'') && s.back() == s.front())
        s = s.substr(1, s.size() - 2);
    return std::string(Trim(s));
}

// Index of the first '=' that isn't inside a quoted key.
size_t FindAssignment(std::string_view line)
{
    char quote = 0;
    for (size_t i = 0; i < line.size(); ++i)
    {
        char c = line[i];
        if (quote)
        {
            if (c == quote)
                quote = 0;
        }
        else if (c == '"' || c == '\'')
            quote = c;
        else if (c == '=')
            return i;
    }
    return std::string_view::npos;
}
} // namespace

std::string_view File(const char *name)
{
    // Overrides are read once and kept for the lifetime of the process.
    static std::map<std::string, std::string> overrides;
    auto it = overrides.find(name);
    if (it == overrides.end())
    {
        std::ifstream in(std::string("HammerillTrainerVC/") + name, std::ios::binary);
        std::string contents;
        if (in)
        {
            std::stringstream ss;
            ss << in.rdbuf();
            contents = ss.str();
        }
        it = overrides.emplace(name, std::move(contents)).first;
    }
    if (!it->second.empty())
        return it->second;

    for (size_t i = 0; i < kEmbeddedFileCount; ++i)
    {
        if (strcmp(kEmbeddedFiles[i].name, name) == 0)
            return {reinterpret_cast<const char *>(kEmbeddedFiles[i].data), kEmbeddedFiles[i].size};
    }
    return {};
}

std::vector<Category> ParseToml(std::string_view text)
{
    std::vector<Category> categories(1);
    while (!text.empty())
    {
        size_t eol = text.find('\n');
        std::string_view line = Trim(text.substr(0, eol));
        text = eol == std::string_view::npos ? std::string_view{} : text.substr(eol + 1);

        if (line.empty() || line.front() == '#')
            continue;
        if (line.front() == '[' && line.back() == ']')
        {
            categories.push_back({Unquote(line.substr(1, line.size() - 2)), {}});
            continue;
        }
        size_t eq = FindAssignment(line);
        if (eq == std::string_view::npos)
            continue;
        categories.back().entries.push_back({Unquote(line.substr(0, eq)), Unquote(line.substr(eq + 1))});
    }
    if (categories.front().entries.empty())
        categories.erase(categories.begin());
    return categories;
}
} // namespace data
