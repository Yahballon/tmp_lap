// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Exact 1920x1080 PS5 UI geometry shared by rendering, navigation and tests.
// Disabled Daily Verse uses the entire available width to the right margin.
namespace bible_ui {
struct Layout {
 bool showDaily;
 float readerX;
 float readerWidth;
 float verseTextWidth;
};
constexpr Layout layoutFor(bool showDaily){
 constexpr float readerX=469.f;
 constexpr float compactWidth=927.f;
 constexpr float expandedWidth=1416.f; // Ends at 1885px (35px right margin).
 constexpr float paragraphMargin=197.f; // Keeps existing v0.1.1 text spacing.
 const float width=showDaily?compactWidth:expandedWidth;
 return {showDaily,readerX,width,width-paragraphMargin};
}
static_assert(layoutFor(true).readerWidth==927.f);
static_assert(layoutFor(false).readerX+layoutFor(false).readerWidth==1885.f);
static_assert(layoutFor(false).verseTextWidth>layoutFor(true).verseTextWidth);
} // namespace bible_ui
