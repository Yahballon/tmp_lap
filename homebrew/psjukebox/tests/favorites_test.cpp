// SPDX-License-Identifier: GPL-3.0-or-later
#include "../native/favorites.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>
int main(){
 char name[]="/tmp/ps5-jbx-favs-XXXXXX";
 int fd=mkstemp(name);assert(fd>=0);
 close(fd);unlink(name);
 std::string error;bool added=false;
 jukebox::Favorites f(name);
 assert(f.load(error));
 assert(jukebox::favorite_key("jbox://8/Tupac/Album/01 Song.mp3")=="Tupac/Album/01 Song.mp3");
 assert(jukebox::favorite_key("/data/PSJukebox/Music/Tupac/Album/01 Song.mp3")=="Tupac/Album/01 Song.mp3");
 assert(jukebox::favorite_key("jbox://8/Tupac/../secrets")=="");
 assert(f.toggle("jbox://8/Tupac/Album/01 Song.mp3",added,error)&&added);
 assert(f.contains("jbox://1/Tupac/Album/01 Song.mp3"));
 assert(f.contains("/mnt/usb0/Music/Tupac/Album/01 Song.mp3"));
 assert(f.size()==1);
 jukebox::Favorites reloaded(name);
 assert(reloaded.load(error)&&reloaded.size()==1);
 assert(reloaded.toggle("/data/PSJukebox/Music/Tupac/Album/01 Song.mp3",added,error)&&!added);
 jukebox::Favorites empty(name);
 assert(empty.load(error)&&empty.size()==0);
 assert(!empty.contains("jbox://8/Tupac/Album/01 Song.mp3"));
 unlink(name);
 std::puts("PASS favorites persist, remove, reload, USB root independence");
}
