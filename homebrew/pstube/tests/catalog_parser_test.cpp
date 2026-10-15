// SPDX-License-Identifier: GPL-3.0-or-later
#include "pstube.hpp"
#include <cassert>
#include <string>
#include <iostream>
int main(){
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
 assert(pstube::valid_id("J---aiyznGQ"));
 assert(!pstube::valid_id("../etc/passw"));
 std::cout<<"PSTUBE catalog parser: 2 search results, trending and malformed responses passed\n";
 return 0;
}
