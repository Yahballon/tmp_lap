// SPDX-License-Identifier: GPL-3.0-or-later
#include "exit_dialog.hpp"
#include <cassert>
#include <cstdio>
int main(){
 using ebook_exit::Action;
 ebook_exit::Dialog dialog;
 assert(!dialog.visible);
 assert(dialog.handle(true,false)==Action::None);
 assert(dialog.handle(false,true)==Action::None);
 dialog.show();
 assert(dialog.visible);
 assert(dialog.handle(false,false)==Action::None);
 assert(dialog.visible);
 // Circle always means No / remain in My Library.
 assert(dialog.handle(false,true)==Action::Cancel);
 assert(!dialog.visible);
 assert(dialog.handle(true,false)==Action::None);
 dialog.show();
 // Cross means Yes / close the app.
 assert(dialog.handle(true,false)==Action::Exit);
 assert(!dialog.visible);
 dialog.show();
 // If two buttons arrive in one sample, default to safe cancellation.
 assert(dialog.handle(true,true)==Action::Cancel);
 assert(!dialog.visible);
 std::puts("My Library exit confirmation: Cross Yes, Circle No and safe re-entry passed");
}
