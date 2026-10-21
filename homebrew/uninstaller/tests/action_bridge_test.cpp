// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/action_bridge.hpp"
#include <cassert>
#include <cstdio>
using namespace uninstaller;
int main(){
  Item system;system.remote=true;system.kind=Kind::Installed;
  system.id="PPSA11386";system.path="/user/app/PPSA11386";
  assert(actions::eligible(system));
  system.id="PPSA99986";system.path="/user/app/PPSA99986";
  assert(!actions::eligible(system));
  system.id="PPSA11386";system.path="/system_ex/app/PPSA11386";
  assert(!actions::eligible(system));
  system.path="/user/app/PPSA00001";
  assert(!actions::eligible(system));
  Item archive;archive.kind=Kind::UsbPackage;archive.remote=true;
  archive.path="/mnt/usb0/Downloads/Test.pkg";
  assert(actions::eligible(archive));
  archive.path="/mnt/usb9/Test.pkg";assert(!actions::eligible(archive));
  archive.path="/mnt/usb0/../Test.pkg";assert(!actions::eligible(archive));
  archive.path="/mnt/ext0/Test.pkg";assert(!actions::eligible(archive));
  archive.path="/mnt/usb0/System";assert(!actions::eligible(archive));
  Item dump;dump.remote=true;dump.kind=Kind::Installed;
  dump.id="PPSA11386";dump.path="/data/etaHEN/games/PPSA11386";
  assert(!actions::eligible(dump));
  puts("PASS: only explicitly supported installed title or USB PKG actions");
}
