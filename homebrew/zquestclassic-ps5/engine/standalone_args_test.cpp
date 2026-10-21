// SPDX-License-Identifier: GPL-3.0-or-later
#include "standalone_args.hpp"
#include <cassert>
#include <cstdio>
int main() {
  using namespace zcps5::engine;
  auto a = make_standalone_args("/data/ZQuestClassic/quests/the_deep.qst");
  assert(a.size() == 4 && a[0] == "zplayer" && a[1] == "-standalone");
  assert(a[2] == "/data/ZQuestClassic/quests/the_deep.qst");
  assert(a[3] == "/data/ZQuestClassic/saves/the_deep.sav");
  a = make_standalone_args("/mnt/usb0/ZQuestClassic/quests/255example.QST");
  assert(a.size() == 4 && a[3] == "/data/ZQuestClassic/saves/255example.sav");
  a = make_standalone_args("/data/ZQuestClassic/quests/Spaces & More.qst");
  assert(a.size() == 4);
  assert(a[2] == "/data/ZQuestClassic/quests/Spaces & More.qst");
  assert(a[3] == "/data/ZQuestClassic/saves/Spaces___More.sav");
  assert(make_standalone_args("/data/ZQuestClassic/quests/../other.qst").empty());
  assert(make_standalone_args("/data/ZQuestClassic/quests/not-a-quest.txt").empty());
  assert(make_standalone_args("/etc/secret.qst").empty());
  assert(make_standalone_args("the_deep.qst").empty());
  assert(make_standalone_args("/data/ZQuestClassic/quests/a.qst/").empty());
  puts("PASS: zplayer -standalone PS5 quest/save argument contract");
}
