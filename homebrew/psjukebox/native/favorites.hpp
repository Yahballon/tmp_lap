// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <set>
#include <string>
#include <cstddef>
namespace jukebox {
std::string favorite_key(const std::string& path);
class Favorites {
public:
 explicit Favorites(std::string filename="/data/PSJukebox/favorites.json");
 bool load(std::string& error);
 bool contains(const std::string& trackPath) const;
 bool toggle(const std::string& trackPath,bool& selected,std::string& error);
 size_t size() const {return keys_.size();}
private:
 bool persist(std::string& error) const;
 std::string filename_;
 std::set<std::string> keys_;
};
}
