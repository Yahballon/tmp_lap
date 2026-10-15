// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <string>
#include <vector>
namespace comic {
struct Book {std::string path;std::vector<std::string> pages;std::string error;
    size_t pageCount() const {return pages.size();}
};
// Native RAR4/RAR5 CBR and ZIP CBZ; images are decoded by the renderer.
bool open(const std::string& path, Book& book);
bool extract(const Book& book, size_t index, std::string& compressedImage, std::string& error);
bool cover(const std::string& path,std::string& image);
bool is_comic(const std::string& path);
} // namespace comic
