#!/usr/bin/env python3
"""Emit a regression harness using the real engine's placed-sentry spawn block.

Compile with actual PhysicsSystem, CallbackSystem, Camera and Map sources. The
spawn code is extracted from the patched engine, not reimplemented in this test.
"""
import sys
from pathlib import Path
engine=Path(sys.argv[1])
out=Path(sys.argv[2])
text=(engine/'src/systems/MapSystem.cpp').read_text()
begin=text.index('      case 90: {  // BULLET BILL')
end=text.index('      /* ****************************************************************** */',begin)
spawn=text[begin:end]
out.write_text('''#include "ECS/Components.h"
#include "Camera.h"
#include "systems/PhysicsSystem.h"
#include "systems/CallbackSystem.h"
#include <cassert>
#include <iostream>
struct FixtureScene { std::shared_ptr<SDL_Texture> enemyTexture; };
void spawnSentry(World* world) {
   FixtureScene fixture;
   auto* scene = &fixture;
   int coordinateX = 40, coordinateY = 8, entityID = 90;
   switch (entityID) {
''' + spawn + '''   }
}
int main() {
   Map::loadEnemyIDS();
   Camera::Get().setCameraX(0);
   Camera::Get().setCameraY(0);
   World world;
   world.registerSystem<PhysicsSystem>();
   world.registerSystem<CallbackSystem>();
   spawnSentry(&world);
   for (int i = 0; i < 30; ++i) world.tick();
   Entity* sentry = world.findFirst<EnemyComponent>();
   assert(sentry && "Placed sentry culled before player reached it");
   assert(sentry->getComponent<PositionComponent>()->getLeft() == 40 * SCALED_CUBE_SIZE);
   const auto box = sentry->getComponent<PositionComponent>()->hitbox;
   assert(box.x == 0 && box.y == 0 && box.w == 32 && box.h == 32);
   assert(sentry->hasComponent<WaitUntilComponent>());
   Camera::Get().setCameraX(1000);
   world.tick();
   assert(!sentry->hasComponent<WaitUntilComponent>());
   assert(sentry->hasComponent<DestroyOutsideCameraComponent>());
   Camera::Get().setCameraX(0);
   world.tick();
   assert(world.findFirst<EnemyComponent>() == nullptr);
   std::cout << "PASS: actual placed sentry survives offscreen, activates, then culls" << std::endl;
}
''',encoding='utf-8')
