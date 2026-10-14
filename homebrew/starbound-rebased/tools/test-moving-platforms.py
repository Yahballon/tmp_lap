#!/usr/bin/env python3
"""Compile real platform factory, PhysicsSystem and PlayerSystem riding code.

The harness checks both axes, full travel, reversals, rider transport, jumping
off, side-contact rejection and off-camera cycling for both player sizes.
"""
import sys
from pathlib import Path
engine,out=map(Path,sys.argv[1:3])
source=(engine/'src/systems/MapSystem.cpp').read_text()
begin=source.index('Entity* MapSystem::createPlatformEntity(')
end=source.index('Entity* MapSystem::createPlatformLevelEntity(',begin)
factory=source[begin:end].replace('Entity* MapSystem::createPlatformEntity(', 'Entity* createPlatformEntity(')
player=(engine/'src/systems/PlayerSystem.cpp').read_text()
begin=player.index('   bool platformMoved = false;')
end=player.index('   checkTrampolineCollisions(world);',begin)
carry=player[begin:end]
out.write_text('''#include "ECS/Components.h"
#include "Camera.h"
#include "systems/PhysicsSystem.h"
#include <cassert>
#include <cmath>
#include <iostream>
struct FixtureScene { std::shared_ptr<SDL_Texture> blockTexture; } fixture;
FixtureScene* scene = &fixture;
''' + factory + '''
void carryRider(World* world, Entity* mario) {
   auto* position = mario->getComponent<PositionComponent>();
   auto* move = mario->getComponent<MovingComponent>();
''' + carry + '''}
void exercise(bool horizontal, int height, bool riderFirst) {
   Camera::Get().setCameraX(0);
   Camera::Get().setCameraY(0);
   World world;
   world.getEntities().reserve(16);
   world.registerSystem<PhysicsSystem>();
   Entity* rider = nullptr;
   auto createRider = [&]() {
      rider=world.create();
      rider->addComponent<PositionComponent>(Vector2f(9*32,(12-height)*32),Vector2i(32,height*32));
      rider->addComponent<MovingComponent>(Vector2f(0,0),Vector2f(0,0));
      rider->addComponent<GravityComponent>();
      rider->addComponent<PlayerComponent>();
   };
   if (riderFirst) createRider();
   Entity* deck=createPlatformEntity(&world,8,12,809,3,
       std::make_tuple(PlatformMotionType::BACK_AND_FORTH,
                      horizontal?Direction::RIGHT:Direction::UP,
                      horizontal?Vector2i(8,24):Vector2i(5,12),false));
   if (!riderFirst) createRider();
   auto* dp=deck->getComponent<PositionComponent>();
   auto* rp=rider->getComponent<PositionComponent>();
   auto* rm=rider->getComponent<MovingComponent>();
   auto* platform=deck->getComponent<MovingPlatformComponent>();
   assert(dp->scale.x==96 && deck->hasComponent<TileComponent>());
   float minimum=10000,maximum=-10000;
   int reversals=0;
   Direction direction=platform->movingDirection;
   for (int frame=0;frame<1600;++frame) {
      world.tick();
      assert(rider->hasComponent<BottomCollisionComponent>());
      carryRider(&world,rider);
      assert(std::abs(rp->getBottom()-dp->getTop())<.01f);
      assert(std::abs(rp->getLeft()-dp->getLeft()-32)<.1f);
      const float point=horizontal?dp->getLeft():dp->getTop();
      minimum=std::min(minimum,point);maximum=std::max(maximum,point);
      assert(point>=platform->minPoint-.1f);
      assert((horizontal?dp->getRight():dp->getBottom())<=platform->maxPoint+.1f);
      if (direction!=platform->movingDirection) {++reversals;direction=platform->movingDirection;}
      rider->remove<BottomCollisionComponent>();
   }
   assert(minimum<=platform->minPoint+1);
   assert(maximum>=(platform->maxPoint-(horizontal?96:32))-1);
   assert(reversals>=2);
   rm->velocity.y=-7.3f;
   world.tick();carryRider(&world,rider);
   assert(rp->getBottom()<dp->getTop()-2 && "Jumping rider was glued to platform");
   // Touching the side must not transport the actor.
   rp->setLeft(dp->getLeft()-20);rp->setTop(dp->getTop()+8);
   rm->velocity.y=0;
   const Vector2f before=rp->position;
   carryRider(&world,rider);
   assert(rp->position==before);
   float awayMin=10000,awayMax=-10000;
   Camera::Get().setCameraX(10000);
   for(int i=0;i<1600;++i) {
      world.tick();
      const float point=horizontal?dp->getLeft():dp->getTop();
      awayMin=std::min(awayMin,point);awayMax=std::max(awayMax,point);
   }
   assert(awayMin<=platform->minPoint+1);
   assert(awayMax>=(platform->maxPoint-(horizontal?96:32))-1);
}
int main() {
   Map::loadBlockIDS();
   for(int height:{1,2})for(bool axis:{false,true})for(bool first:{false,true})
      exercise(axis,height,first);
   std::cout<<"PASS: real platform factory/physics/riding, both axes, both sizes, reversals and jump-off"<<std::endl;
}
''',encoding='utf-8')
