// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ebook {
struct InlineImage {
    size_t line=0;                 // Absolute text-line position; each image occupies 7 rows.
    std::string encoded;          // Bounded original JPEG / PNG bytes from the EPUB ZIP.
    std::string alt;
};
struct Document {
    std::string title;
    std::string author;
    std::string format;
    std::vector<std::string> lines;
    std::vector<InlineImage> images;
    std::string coverImage;
    std::vector<size_t> chapterStarts;
    std::string error;
    bool isPdfTextPreview = false;
    size_t pageCount() const { return (lines.size()+33)/34; }
};
struct BookEntry {
    std::string path, title, author, collection, format;
    bool favorite=false;
    size_t progress=0;
};
struct ScanSource {
    std::string path;
    std::string error;
    int books=0;
    int folders=0;
    int candidates=0;
    bool opened=false;
};
const std::vector<ScanSource>& last_scan_report();
bool read_document(const std::string& path, Document& out);
bool read_epub_cover(const std::string& path,std::string& encoded);

std::string clean_title(std::string name);
bool scan_books(const std::vector<std::string>& dirs, std::vector<BookEntry>& books);
bool scan_imports(std::vector<BookEntry>& found);
bool import_to_library(const std::string& source, std::string& result);
void load_state(std::vector<BookEntry>& books);
bool save_state(const std::vector<BookEntry>& books);
std::string state_directory();
bool erase_imported(const BookEntry& book);
} // namespace ebook
