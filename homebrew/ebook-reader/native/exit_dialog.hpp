// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Pure controller state for the main-library exit confirmation.
// Confirm only with Cross; Circle is always a safe cancellation.
namespace ebook_exit {
enum class Action { None, Cancel, Exit };
struct Dialog {
 bool visible=false;
 void show(){visible=true;}
 Action handle(bool cross,bool circle){
   if(!visible)return Action::None;
   if(circle){visible=false;return Action::Cancel;}
   if(cross){visible=false;return Action::Exit;}
   return Action::None;
 }
};
} // namespace ebook_exit
