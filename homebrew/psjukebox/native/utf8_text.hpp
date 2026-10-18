// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
namespace jukebox {
// Truncate UTF-8 by Unicode scalar rather than slicing a byte sequence.
// In particular "Künstler", "Björk", "София" and Japanese UTF-8 titles
// must never become an invalid lead-byte or a broken continuation byte.
inline std::string utf8_cut(std::string_view input,std::size_t max_chars){
 if(max_chars==0)return {};
 std::size_t pos=0,chars=0;
 while(pos<input.size()&&chars<max_chars){
  const unsigned char b=static_cast<unsigned char>(input[pos]);
  const std::size_t len=b<0x80?1:(b>=0xc2&&b<0xe0?2:(b>=0xe0&&b<0xf0?3:(b>=0xf0&&b<0xf5?4:1)));
  if(pos+len>input.size())break;
  bool valid=true;
  for(std::size_t k=1;k<len;k++)
   if((static_cast<unsigned char>(input[pos+k])&0xc0)!=0x80){valid=false;break;}
  // Invalid bytes are omitted rather than fed to the SDF renderer.
  if(!valid||len==1&&b>=0x80){++pos;continue;}
  pos+=len;++chars;
 }
 if(pos==input.size())return std::string(input);
 if(max_chars<=3)return std::string(input.substr(0,pos));
 // Reserve three character positions for ASCII ellipsis.
 pos=0;chars=0;
 while(pos<input.size()&&chars<max_chars-3){
  const unsigned char b=static_cast<unsigned char>(input[pos]);
  const std::size_t len=b<0x80?1:(b>=0xc2&&b<0xe0?2:(b>=0xe0&&b<0xf0?3:(b>=0xf0&&b<0xf5?4:1)));
  if(pos+len>input.size())break;
  bool valid=true;
  for(std::size_t k=1;k<len;k++)
   if((static_cast<unsigned char>(input[pos+k])&0xc0)!=0x80){valid=false;break;}
  if(!valid||len==1&&b>=0x80){++pos;continue;}
  pos+=len;++chars;
 }
 return std::string(input.substr(0,pos))+"...";
}
}
