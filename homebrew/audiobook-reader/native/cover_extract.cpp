// SPDX-License-Identifier: GPL-3.0-or-later
#include "cover_extract.hpp"
#include "audio_cover_store.hpp"
#include "mp4_cover.hpp"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/codec_par.h>
}
#include <cstdint>
#include <string>
namespace audio_cover {
bool extract(const std::string& file,std::string& encoded,std::string& diagnostic){
 encoded.clear();diagnostic.clear();
 AVFormatContext* fmt=nullptr;
 if(avformat_open_input(&fmt,file.c_str(),nullptr,nullptr)<0){
  diagnostic="Cannot open M4B cover metadata";return false;
 }
 // Most M4B iTunes covr atoms are presented as an attached-picture video
 // stream with a complete JPEG/PNG packet, without any frame decoding needed.
 if(avformat_find_stream_info(fmt,nullptr)<0){
  diagnostic="M4B stream information unavailable";
  avformat_close_input(&fmt);return false;
 }
 const unsigned count=fmt->nb_streams;
 for(unsigned i=0;i<count;i++){
  const AVStream* stream=fmt->streams[i];
  if(!stream || !stream->codecpar)continue;
  if(!(stream->disposition&AV_DISPOSITION_ATTACHED_PIC))continue;
  if(stream->codecpar->codec_id!=AV_CODEC_ID_MJPEG &&
     stream->codecpar->codec_id!=AV_CODEC_ID_PNG &&
     stream->codecpar->codec_id!=AV_CODEC_ID_GIF)continue;
  const AVPacket& pic=stream->attached_pic;
  if(pic.data && pic.size>=8 && pic.size<=6*1024*1024){
   std::string value(reinterpret_cast<const char*>(pic.data),(size_t)pic.size);
   if(audio_cover::image(value)){
    encoded=std::move(value);
    diagnostic="Embedded M4B cover extracted";
    avformat_close_input(&fmt);
    return true;
   }
  }
 }
 avformat_close_input(&fmt);
 if(audio_cover::extract_mp4_covr(file,encoded)){
  diagnostic="iTunes covr box extracted directly";
  return true;
 }
 diagnostic="No attached art or readable MP4 covr box; companion JPG/PNG will be checked";
 return false;
}
}
