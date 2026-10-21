// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/shadowmount_api.hpp"
#include <cassert>
#include <cstdio>
using namespace uninstaller::shadow;
int main(){
  const std::string ok=R"({"status":0,"title_id":"PPSA21564","path":"/mnt/usb0/etaHEN/games/AstroBot","source_type":"folder","can_manage_source":true,"source_available":true,"mounted":true})";
  const Source source=parseSource(ok,"PPSA21564");
  assert(source.valid&&source.title=="PPSA21564");
  assert(source.path=="/mnt/usb0/etaHEN/games/AstroBot");
  assert(!parseSource(ok,"PPSA21567").valid);
  assert(!parseSource(R"({"status":0,"title_id":"PPSA21564","path":"/user/app/PPSA21564","source_type":"pkg","can_manage_source":false,"source_available":true})","PPSA21564").valid);
  assert(!parseSource(R"({"status":0,"title_id":"PPSA21564","path":"/tmp","source_type":"folder","can_manage_source":true,"source_available":false})","PPSA21564").valid);
  assert(number(R"({"status":0,"job_id":42})","job_id")==42);
  assert(flag(R"({"can_manage_source":true})","can_manage_source"));
  assert(!validId("PPSA99986"));
  assert(!validId("PPSA2221564"));
  puts("PASS: trusted ShadowMount title-source mapping, ID and API response checks");
}
