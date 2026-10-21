#include "../native/controls.hpp"
#include <cassert>
#include <cstdio>
using namespace wins4;
int main(){
 Game g;Controller c;
 assert(g.screen==Screen::Home&&g.focus==0);
 c.handle(g,Input{.down=true},100);assert(g.focus==1);
 c.handle(g,Input{.confirm=true},200);assert(g.screen==Screen::Mode);
 c.handle(g,Input{.down=true},300);assert(g.submenu==1);
 c.handle(g,Input{.confirm=true},400);assert(g.screen==Screen::Difficulty&&g.opponent==Opponent::Bot);
 c.handle(g,Input{.down=true},500);assert(g.submenu==2);
 c.handle(g,Input{.confirm=true},600);assert(g.level==Level::Hard&&g.screen==Screen::Home);
 c.handle(g,Input{.up=true},700);assert(g.focus==0);
 c.handle(g,Input{.confirm=true},800);assert(g.screen==Screen::Playing);
 c.handle(g,Input{.right=true},900);assert(g.selected==4);
 c.handle(g,Input{.confirm=true},1000);assert(g.moves==1&&g.turn==2&&c.last_drop_col==4);
 c.handle(g,Input{},2000);assert(g.moves==1&&c.bot_pending);
 c.handle(g,Input{},432100);assert(g.moves==2&&g.turn==1);
 c.handle(g,Input{.options=true},440000);assert(g.screen==Screen::Paused);
 c.handle(g,Input{.down=true},441000);assert(g.submenu==1);
 c.handle(g,Input{.confirm=true},442000);assert(g.screen==Screen::Playing&&g.moves==0);
 c.handle(g,Input{.back=true},443000);assert(g.screen==Screen::Paused);
 c.handle(g,Input{.down=true},444000);c.handle(g,Input{.down=true},444100);assert(g.submenu==2);
 c.handle(g,Input{.confirm=true},445000);assert(g.screen==Screen::Home);
 c.handle(g,Input{.back=true},446000);assert(g.screen==Screen::Exit&&g.submenu==1);
 c.handle(g,Input{.up=true},447000);assert(g.submenu==0);
 assert(c.handle(g,Input{.confirm=true},448000));
 puts("PASS: mode, AI selection, input, pause, restart, exit state");
}
