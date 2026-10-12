// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
#include <vector>

namespace tracker {
class PinSettings {
public:
    // No existing file -> initial default favourites.
    std::vector<std::string> ids{"bitcoin", "ethereum", "litecoin"};
    bool loaded = false;
    bool persisted = false;
    bool contains(const std::string& id) const;
    bool toggle(const std::string& id);
    bool load();
    bool save();
    bool load_from(const std::string& path);
    bool save_to(const std::string& path) const;
    static bool valid_id(const std::string& id);
};
} // namespace tracker
