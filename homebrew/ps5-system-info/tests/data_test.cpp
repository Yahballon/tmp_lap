// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/data.hpp"
#include <cassert>
#include <cstdio>
int main(){
 auto settings=info::loadSettings();
 assert(settings.refreshSeconds==0||settings.refreshSeconds==5||settings.refreshSeconds==10||settings.refreshSeconds==30||settings.refreshSeconds==60);
 auto snap=info::collect(1920,1080,false);
 assert(!snap.system.empty()&&!snap.hardware.empty()&&!snap.network.empty());
 assert(snap.system[3].value=="Unavailable");
 assert(!snap.video.empty()&&snap.video[0].value=="1920 x 1080");
 assert(!info::compactBytes(1048576).empty());
 puts("Read-only data provider tests passed");
}
