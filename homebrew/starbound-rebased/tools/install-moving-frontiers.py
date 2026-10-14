#!/usr/bin/env python3
"""Real moving-platform creation, bounded motion and top-only player transport.

Run after install-campaign.py and install-deep-signal.py on the pinned engine.
"""
import sys
from pathlib import Path
engine=Path(sys.argv[1]) if len(sys.argv)>1 else Path('build/gold872-engine')
def patch(path,old,new):
    p=engine/path
    text=p.read_text()
    assert text.count(old)==1,(path,old[:90],text.count(old))
    p.write_text(text.replace(old,new),encoding='utf-8')

patch('src/systems/MapSystem.cpp', '''      case 609: {  // MOVING PLATFORM
         createBlockEntity(world, coordinateX, coordinateY, entityID);
      } break;''', '''      case 609: {  // MOVING PLATFORM
         auto platformData = getPlatformCoordinate(scene->getLevelData().movingPlatformDirections,
                                                   Vector2i(coordinateX, coordinateY));
         createPlatformEntity(world, coordinateX, coordinateY, entityID, 1, platformData);
      } break;''')
patch('include/ECS/Components.h', '''   float minPoint;
   float maxPoint;''', '''   float minPoint;
   float maxPoint;
   Vector2f previousPosition;
   Vector2f displacement;''')
patch('src/systems/MapSystem.cpp',
      '   platform->addComponent<CollisionExemptComponent>();',
      '''   platform->addComponent<CollisionExemptComponent>();
   // Ferries must keep cycling beyond the viewport so a missed ride returns.
   platform->addComponent<MoveOutsideCameraComponent>();''')
patch('src/systems/MapSystem.cpp', '''   return platform;
}

Entity* MapSystem::createPlatformLevelEntity(''', '''   if (platform->hasComponent<MovingPlatformComponent>()) {
      auto* motion = platform->getComponent<MovingPlatformComponent>();
      motion->previousPosition = platform->getComponent<PositionComponent>()->position;
      motion->displacement = Vector2f(0, 0);
   }
   return platform;
}

Entity* MapSystem::createPlatformLevelEntity(''')

for edge,coordinate,direction,axis in (
    ('getLeft()', 'setLeft(platform->minPoint)', 'RIGHT', 'x'),
    ('getRight()', 'setRight(platform->maxPoint)', 'LEFT', 'x'),
    ('getTop()', 'setTop(platform->minPoint)', 'DOWN', 'y'),
    ('getBottom()', 'setBottom(platform->maxPoint)', 'UP', 'y')):
    bound='minPoint' if edge in ('getLeft()','getTop()') else 'maxPoint'
    compare='<=' if bound=='minPoint' else '>='
    old=f'''                  if (position->{edge} {compare} platform->{bound}) {{
                     platform->movingDirection = Direction::{direction};
                     break;
                  }}'''
    new=f'''                  if (position->{edge} {compare} platform->{bound}) {{
                     position->{coordinate};
                     platformMove->velocity.{axis} = 0;
                     platform->movingDirection = Direction::{direction};
                     break;
                  }}'''
    patch('src/systems/PhysicsSystem.cpp',old,new)
patch('src/systems/PhysicsSystem.cpp',
      '''float newVelocity = -platform->calculateVelocity(
                         position->getRight() - platform->minPoint,''',
      '''float newVelocity = -1.6f * platform->calculateVelocity(
                         position->getRight() - platform->minPoint,''')
patch('src/systems/PhysicsSystem.cpp',
      '''float newVelocity = platform->calculateVelocity(
                         platform->maxPoint - position->getLeft(),''',
      '''float newVelocity = 1.6f * platform->calculateVelocity(
                         platform->maxPoint - position->getLeft(),''')
patch('src/systems/PhysicsSystem.cpp', '''   });
}

void PhysicsSystem::updatePlatformLevels(World* world) {''',
      '''      platform->displacement = position->position - platform->previousPosition;
      platform->previousPosition = position->position;
      // Preserve grounded input on descending lifts before PlayerSystem runs.
      world->find<PlayerComponent, PositionComponent, MovingComponent>([&](Entity* rider) {
         if (rider->hasAny<DeadComponent, ParticleComponent, FrozenComponent>()) return;
         auto* feet = rider->getComponent<PositionComponent>();
         auto* motion = rider->getComponent<MovingComponent>();
         if (motion->velocity.y >= 0 && feet->getRight() > position->getLeft() + 2 &&
             feet->getLeft() < position->getRight() - 2 &&
             std::abs(feet->getBottom() - position->getTop()) <=
                 std::abs(platform->displacement.y) + 1.0f) {
            rider->addComponent<BottomCollisionComponent>();
            motion->velocity.y = motion->acceleration.y = 0;
         }
      });
   });
}

void PhysicsSystem::updatePlatformLevels(World* world) {''')
patch('src/systems/PlayerSystem.cpp',
      '''      if (!AABBCollision(position, entity->getComponent<PositionComponent>()) || platformMoved) {
         return;
      }
      auto* platformMove = entity->getComponent<MovingComponent>();
      position->position.x += platformMove->velocity.x;
      position->position.y += platformMove->velocity.y;

      if (position->position.x + 16 > Camera::Get().getCameraCenterX() &&
          platformMove->velocity.x > 0) {
         Camera::Get().increaseCameraX(platformMove->velocity.x);
      }''',
      '''      auto* deck = entity->getComponent<PositionComponent>();
      const Vector2f delta = entity->getComponent<MovingPlatformComponent>()->displacement;
      const bool overlapX = position->getRight() > deck->getLeft() + 2 &&
                            position->getLeft() < deck->getRight() - 2;
      const bool onTop = std::abs(position->getBottom() - deck->getTop()) <=
                         std::abs(delta.y) + 1.0f;
      if (!overlapX || !onTop || move->velocity.y < 0 || platformMoved) return;
      // Transport only a rider standing on top, using actual displacement.
      // Snapping feet avoids applying vertical movement twice after collision.
      position->position.x += delta.x;
      position->setBottom(deck->getTop());
      if (position->position.x + 16 > Camera::Get().getCameraCenterX() && delta.x > 0)
         Camera::Get().increaseCameraX(delta.x);''')
print('Moving Frontiers: actual platform entities, bounded motion, displacement-based top-only riding')
