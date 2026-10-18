// SPDX-License-Identifier: GPL-3.0-or-later
#include "phoneme_ids.hpp"
namespace bible_piper {
namespace {
bool readUtf8(std::string_view s,std::size_t& cursor,std::uint32_t& cp){
 if(cursor>=s.size())return false;
 const auto a=(std::uint8_t)s[cursor++];
 if(a<0x80){cp=a;return true;}
 const int length=(a>=0xc2&&a<=0xdf)?2:(a>=0xe0&&a<=0xef)?3:(a>=0xf0&&a<=0xf4)?4:0;
 if(!length||cursor+(std::size_t)(length-1)>s.size())return false;
 cp=a&(length==2?0x1fu:length==3?0x0fu:0x07u);
 for(int i=1;i<length;++i){
  const auto b=(std::uint8_t)s[cursor++];
  if((b&0xc0u)!=0x80u)return false;
  cp=(cp<<6)|(b&0x3fu);
 }
 if(cp<0x80u||(length==3&&cp<0x800u)||
    (length==4&&cp<0x10000u)||cp>0x10ffffu||
    (cp>=0xd800u&&cp<=0xdfffu))return false;
 return true;
}
}
EncodedIds encode(std::string_view ipa,const PhonemeTable& table){
 EncodedIds result;
 if(ipa.empty()||ipa.size()>8192||!table.symbols||!table.values||
     !table.symbolsCount||!table.valuesCount){
  result.error="Invalid or empty IPA/table";
  return result;
 }
 result.ids={1,0}; // Piper BOS (^), followed by pad (_)
 std::size_t cursor=0;
 while(cursor<ipa.size()){
  std::uint32_t cp=0;
  if(!readUtf8(ipa,cursor,cp)){result.error="Invalid UTF-8 IPA";result.ids.clear();return result;}
  const Symbol* found=nullptr;
  for(std::size_t i=0;i<table.symbolsCount;++i){
   if(table.symbols[i].codepoint==cp){found=table.symbols+i;break;}
  }
  if(!found||(std::size_t)found->offset+found->count>table.valuesCount||
     found->count==0){
   result.error="Unmapped Piper phoneme U+"+std::to_string(cp);
   result.ids.clear();return result;
  }
  for(std::size_t i=0;i<found->count;++i)
   result.ids.push_back(table.values[found->offset+i]);
  result.ids.push_back(0); // Intersperse pad after each symbol
  if(result.ids.size()>8192){result.error="Too many Piper ids";result.ids.clear();return result;}
 }
 result.ids.push_back(2); // EOS ($)
 result.success=true;
 return result;
}
}
