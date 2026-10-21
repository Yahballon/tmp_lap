// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/quest_index.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

static void touch(const std::string& path) {
  FILE* f = fopen(path.c_str(), "wb");
  assert(f);
  assert(fputs("data", f) >= 0);
  assert(fclose(f) == 0);
}
int main() {
  using namespace zcps5;
  assert(is_quest_filename("demo.qst"));
  assert(is_quest_filename("DEMO.QST"));
  assert(!is_quest_filename("other.qsu"));
  assert(!is_quest_filename("quest.qst.bak"));
  assert(!is_quest_filename(".qst"));
  assert(display_name("first_quest.qst") == "first quest");
  char name[] = "/tmp/zcps5-test-XXXXXX";
  char* p = mkdtemp(name);
  assert(p != nullptr);
  const std::string root(p);
  assert(mkdir((root + "/nested").c_str(), 0700) == 0);
  touch(root + "/intro.QST");
  touch(root + "/nested/maze.qst");
  touch(root + "/nested/skip.txt");
  assert(symlink("/etc", (root + "/outside").c_str()) == 0);
  QuestIndex index;
  index.begin({root, root + "/missing"});
  for (int i = 0; i < 200 && !index.done(); ++i) index.step(1);
  assert(index.done());
  assert(index.quests().size() == 2);
  assert(index.quests()[0].name == "intro");
  assert(index.quests()[1].name == "maze");
  assert(index.quests()[0].bytes == 4);
  index.begin({root + "/missing"});
  index.step(2);
  assert(index.done() && index.quests().empty());
  assert(unlink((root + "/outside").c_str()) == 0);
  assert(unlink((root + "/nested/maze.qst").c_str()) == 0);
  assert(unlink((root + "/nested/skip.txt").c_str()) == 0);
  assert(unlink((root + "/intro.QST").c_str()) == 0);
  assert(rmdir((root + "/nested").c_str()) == 0);
  assert(rmdir(root.c_str()) == 0);
  puts("ZQuest quest index host tests passed");
  return 0;
}
