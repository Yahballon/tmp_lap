// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/helper_bridge.hpp"
#include <cassert>
#include <cstdio>
#include <string>
int main(){
  using namespace uninstaller;
  std::string metadata=R"({"titleId":"PPSA30123","titleName":"My App","contentVersion":"01.000.001"})";
  const std::string folder="/mnt/usb0/homebrew/PPSA30123";
  const std::string name="PPSA30123";
  std::string data="PS5 loader banner\nUNSCN1\n";
  data+="R 1 0 0 9\n"+std::string("/mnt/usb0");
  char h[128];
  std::snprintf(h,sizeof(h),"I H %zu %zu %zu 0\n",folder.size(),name.size(),metadata.size());
  data+=h+folder+name+metadata;
  const std::string pkg="/mnt/usb0/Downloads/Demo.pkg";
  const std::string pkgName="Demo.pkg";
  std::snprintf(h,sizeof(h),"I P %zu %zu 0 9876\n",pkg.size(),pkgName.size());
  data+=h+pkg+pkgName+"DONE 2 8\n";
  auto parsed=bridge::parse(data);
  assert(parsed.complete&&parsed.denied==1);
  assert(parsed.items.size()==2);
  assert(parsed.items[0].id=="PPSA30123");
  assert(parsed.items[0].name=="My App");
  assert(parsed.items[0].remote);
  assert(parsed.items[1].sizeKnown && parsed.items[1].size==9876);
  assert(!isAllowedRemoval(parsed.items[0]) && !isAllowedRemoval(parsed.items[1]));
  std::vector<Item> native{parsed.items[0]}; native[0].remote=false;
  bridge::merge(native,parsed);
  assert(native.size()==2);
  std::string bad="UNSCN1\nI P 3000 0 0 123\n";auto rejected=bridge::parse(bad);
  assert(!rejected.complete && rejected.items.empty());
  const std::string evil="../etc/passwd";
  assert(!bridge::pathAllowed(evil,'P'));
  puts("PASS: sandbox read-only helper binary protocol & safe dedupe");
}
