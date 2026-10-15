// SPDX-License-Identifier: GPL-3.0-or-later
#include "pstube.hpp"
#include <cassert>
#include <string>
#include <iostream>
#include <fstream>
#include <iterator>
int main(int argc,char** argv){
 // Representative real Invidious /api/v1/search response structure.
 const std::string search=R"JSON([
 {"type":"video","title":"Exploring PS5 Worlds","videoId":"dQw4w9WgXcQ","author":"Creator","lengthSeconds":186,"viewCount":98765,
 "videoThumbnails":[{"quality":"medium","url":"https://i.ytimg.com/vi/dQw4w9WgXcQ/mqdefault.jpg","width":320,"height":180}]},
 {"type":"channel","author":"Not a video","authorId":"UC123"},
 {"type":"video","title":"Another Video","videoId":"9bZkp7q19f0","author":"Channel","lengthSeconds":260,"viewCount":120}
 ])JSON";
 const auto result=pstube::parse_search(search);
 assert(result.size()==2);
 assert(result[0].id=="dQw4w9WgXcQ");
 assert(result[0].author=="Creator");
 assert(result[0].length==186);
 assert(result[1].id=="9bZkp7q19f0");
 // Trending API objects omit "type", but still contain videoId and title.
 const auto trending=pstube::parse_search(R"JSON([
 {"title":"Trending Gameplay","videoId":"J---aiyznGQ","author":"Arcade","viewCount":35276,
 "videoThumbnails":[{"quality":"medium","url":"/vi/J---aiyznGQ/mqdefault.jpg"}]}
 ])JSON");
 assert(trending.size()==1);
 assert(trending[0].title=="Trending Gameplay");
 assert(pstube::parse_search("{}").empty());
 assert(pstube::parse_search("<html>Rate limited</html>").empty());
 const std::string liveFixture=R"HTML(<html><script>var ytInitialData = {"contents":[
 {"videoRenderer":{"videoId":"Xy_Cznzdfj0","thumbnail":{"thumbnails":[{"url":"https://i.ytimg.com/vi/Xy_Cznzdfj0/hq720.jpg"}]},
 "title":{"runs":[{"text":"A Real Gaming Video"}]},"longBylineText":{"runs":[{"text":"Creator Channel"}]},
 "lengthText":{"simpleText":"5:06"},"viewCountText":{"simpleText":"56,396 views"}}}
 ]};</script></html>)HTML";
 const auto parsedPage=pstube::parse_youtube_html(liveFixture);
 assert(parsedPage.size()==1);
 assert(parsedPage[0].id=="Xy_Cznzdfj0");
 assert(parsedPage[0].title=="A Real Gaming Video");
 assert(parsedPage[0].author=="Creator Channel");
 assert(parsedPage[0].length==306);
 if(argc>1){
  std::ifstream source(argv[1],std::ios::binary);
  std::string actual((std::istreambuf_iterator<char>(source)),std::istreambuf_iterator<char>());
  auto live=pstube::parse_youtube_html(actual);
  std::cout<<"Live YouTube renderer parser: "<<live.size()<<" real videos"<<std::endl;
  assert(!live.empty());
 }
 const std::string continuationFixture=R"DATA(
 {"onResponseReceivedCommands":[{"appendContinuationItemsAction":{"continuationItems":[
  {"itemSectionRenderer":{"contents":[{"videoRenderer":{
   "videoId":"J---aiyznGQ","title":{"runs":[{"text":"Page 2 video"}]},
   "ownerText":{"runs":[{"text":"Uploader"}]},"lengthText":{"simpleText":"10:22"}
  }}]}},
  {"continuationItemRenderer":{"trigger":"CONTINUATION_TRIGGER_ON_ITEM_SHOWN",
    "continuationEndpoint":{"continuationCommand":{"token":"CONTINUATION_PAGE_3"}}}}
 ]}}]})DATA";
 const auto page2=pstube::parse_youtube_html(continuationFixture);
 assert(page2.size()==1 && page2[0].title=="Page 2 video");
 assert(pstube::youtube_continuation(continuationFixture)=="CONTINUATION_PAGE_3");
 const std::string initialFixture=R"DATA({"ytInitialData":{"contents":[
 {"continuationItemRenderer":{"continuationEndpoint":{"continuationCommand":{"token":"PAGE_TWO"}}}}
 ]}})DATA";
 assert(pstube::youtube_continuation(initialFixture)=="PAGE_TWO");
 assert(pstube::valid_id("J---aiyznGQ"));
 assert(!pstube::valid_id("../etc/passw"));
 std::cout<<"PSTUBE catalog parser: 2 search results, trending and malformed responses passed\n";
 return 0;
}
