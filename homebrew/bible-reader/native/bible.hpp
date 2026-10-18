// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <string>
#include <vector>
namespace bible {
struct Verse { int book=0, chapter=1, number=1; std::string text; };
struct Ref { int book=0, chapter=1, verse=1; };
struct SearchHit { Ref ref; std::string text; };
extern const std::array<const char*,66> kBooks;
class Library {
public:
 bool load(const std::string& path,std::string& error);
 int chapterCount(int book) const;
 const std::vector<Verse>& chapter(int book,int chapter) const;
 const Verse* verse(Ref ref) const;
 std::vector<SearchHit> search(std::string query,size_t limit=150) const;
 size_t totalVerses() const { return count_; }
private:
 std::array<std::vector<std::vector<Verse>>,66> data_{};
 size_t count_=0;
};
struct Settings {
 int book=0, chapter=1, selectedVerse=1, fontSize=29, theme=0;
 bool daily=true;
 int language=0; // EN 0, DE 1, FR 2, ES 3, retro EN 4, Russian 5
 int voiceStyle=1; // 0 classic, 1 neural preview when supplied (else classic fallback)
 std::vector<Ref> bookmarks;
};
bool same(Ref a,Ref b);
bool bookmarked(const Settings& s,Ref r);
void toggle(Settings& s,Ref r);
// Retro speech is a temporary language preset, not a permanent Classic mode.
 // When leaving older saved Retro settings, restore the normal Neural HD default.
int voiceStyleAfterLanguageChange(int previousLanguage,int nextLanguage,int currentStyle);
bool readSettings(const std::string& path,Settings& s);
bool writeSettings(const std::string& path,const Settings& s);
Ref dailyReference(int dayOfYear);
}
