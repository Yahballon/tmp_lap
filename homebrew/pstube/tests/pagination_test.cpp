// SPDX-License-Identifier: GPL-3.0-or-later
#include "pstube.hpp"
#include <cassert>
#include <iostream>
namespace pstube {
std::string youtube_continuation(const std::string&);
std::string youtube_client_version(const std::string&);
std::string continuation_body(const std::string&,const std::string&);
}
int main(){
 const std::string html=R"HTML(<html><script>ytcfg.set({"INNERTUBE_CONTEXT_CLIENT_VERSION":"2.20261015.00.00"});var ytInitialData = {"contents":[
 {"videoRenderer":{"videoId":"dQw4w9WgXcQ","title":{"runs":[{"text":"First genuine result"}]},"ownerText":{"runs":[{"text":"Channel"}]}}},
 {"continuationItemRenderer":{"continuationEndpoint":{"continuationCommand":{"token":"ABCDEFGHIJKLMN0123456789abcdefghijklmnopqrstuvwxyz"}}}}
 ]};</script></html>)HTML";
 const auto videos=pstube::parse_youtube_html(html);
 assert(videos.size()==1 && videos[0].title=="First genuine result");
 const std::string token=pstube::youtube_continuation(html);
 assert(token=="ABCDEFGHIJKLMN0123456789abcdefghijklmnopqrstuvwxyz");
 assert(pstube::youtube_client_version(html)=="2.20261015.00.00");
 const std::string body=pstube::continuation_body(token,pstube::youtube_client_version(html));
 assert(body.find("\"continuation\":\""+token+"\"")!=std::string::npos);
 assert(body.find("\"clientName\":\"WEB\"")!=std::string::npos);
 const std::string next=R"JSON({"onResponseReceivedCommands":[{"appendContinuationItemsAction":{"continuationItems":[
 {"itemSectionRenderer":{"contents":[
   {"videoRenderer":{"videoId":"9bZkp7q19f0","title":{"runs":[{"text":"Another genuine result"}]},"ownerText":{"runs":[{"text":"Channel B"}]}}}
 ]}},
 {"continuationItemRenderer":{"continuationEndpoint":{"continuationCommand":{"token":"NEXTCONTINUATIONTOKEN12345678901234567890"}}}}
]}}]})JSON";
 const auto more=pstube::parse_youtube_html(next);
 assert(more.size()==1 && more[0].id=="9bZkp7q19f0");
 assert(pstube::youtube_continuation(next)=="NEXTCONTINUATIONTOKEN12345678901234567890");
 std::cout<<"PSTUBE continuation parser and request builder: PASS\n";
}
