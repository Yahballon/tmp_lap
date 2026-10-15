// SPDX-License-Identifier: GPL-3.0-or-later
#include "chapters.hpp"
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
}
#include <algorithm>
namespace audio {
std::vector<Chapter> chapters(const std::string& path,double* duration,std::string* author,std::string* title){
 std::vector<Chapter> result;AVFormatContext* fmt=nullptr;
 if(avformat_open_input(&fmt,path.c_str(),nullptr,nullptr)<0)return result;
 if(avformat_find_stream_info(fmt,nullptr)<0){avformat_close_input(&fmt);return result;}
 if(duration && fmt->duration>0)*duration=fmt->duration/(double)AV_TIME_BASE;
 if(author){auto* t=av_dict_get(fmt->metadata,"artist",nullptr,0);if(t)*author=t->value;}
 if(title){auto* t=av_dict_get(fmt->metadata,"title",nullptr,0);if(t)*title=t->value;}
 for(unsigned i=0;i<fmt->nb_chapters && i<512;i++){
  const AVChapter* ch=fmt->chapters[i];if(!ch)continue;
  const double start=ch->start*av_q2d(ch->time_base),end=ch->end*av_q2d(ch->time_base);
  if(start<0||end<start)continue;
  auto* name=av_dict_get(ch->metadata,"title",nullptr,0);
  result.push_back({name?std::string(name->value):"Chapter "+std::to_string(i+1),start,end});
 }
 std::sort(result.begin(),result.end(),[](auto& a,auto& b){return a.start<b.start;});
 avformat_close_input(&fmt);return result;
}
}
