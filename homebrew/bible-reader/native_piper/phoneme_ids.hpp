// SPDX-License-Identifier: GPL-3.0-or-later
// Piper VITS phoneme-to-id conversion (portable C++17, no ONNX dependency).
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
namespace bible_piper {
struct Symbol {
 std::uint32_t codepoint;
 std::uint16_t offset;
 std::uint8_t count;
};
struct PhonemeTable {
 const Symbol* symbols=nullptr;
 std::size_t symbolsCount=0;
 const std::int64_t* values=nullptr;
 std::size_t valuesCount=0;
};
struct EncodedIds {
 bool success=false;
 std::vector<std::int64_t> ids;
 std::string error;
};
// Input is already eSpeak IPA normalised to Unicode NFD. Never silently drop
// unknown phonemes: wrong ids can synthesize unexpected words.
EncodedIds encode(std::string_view ipa,const PhonemeTable& table);
} // namespace bible_piper
