// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/inventory.hpp"
#include <cassert>
#include <cstdio>
using namespace uninstaller;
int main() {
  assert(validTitleId("PPSA99873"));
  assert(validTitleId("CUSA12345"));
  assert(!validTitleId("PPSA0000/"));
  assert(!validTitleId("../../../.."));
  assert(!normalName(".."));
  assert(!normalName("../homebrew"));
  assert(endsPkg("my_game.pkg") && endsPkg("MY_GAME.PKG"));
  assert(!endsPkg("my_game.pkg.exe"));
  Item root;root.kind=Kind::Homebrew;root.id="PPSA99873";root.path="/data/homebrew";
  assert(!isAllowedRemoval(root));
  Item sys;sys.kind=Kind::Installed;sys.id="PPSA99873";sys.path="/user/app/PPSA99873";
  assert(!isAllowedRemoval(sys));
  Item fake;fake.kind=Kind::UsbPackage;fake.path="/etc/passwd";assert(!isAllowedRemoval(fake));
  assert(field(R"({"localizedParameters":{"en-US":{"titleName":"Demo"}},"contentVersion":"01.000.001"})","titleName")=="Demo");
  assert(!bytes(1024).empty());
  puts("Inventory parsing and unsafe-path rejection: PASS");
}
