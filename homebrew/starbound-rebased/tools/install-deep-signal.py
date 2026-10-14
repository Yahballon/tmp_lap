#!/usr/bin/env python3
"""Add biome backdrops, reset-safe swimming, drone animation and sentinel fixes.

Run after install-campaign.py. Supports the raw desktop and adapted PS5 engine.
"""
import sys
from pathlib import Path

engine=Path(sys.argv[1]) if len(sys.argv)>1 else Path('build/gold872-engine')
def patch(path,old,new):
    p=engine/path
    s=p.read_text()
    assert s.count(old)==1,(path,old[:70],s.count(old))
    p.write_text(s.replace(old,new),encoding='utf-8')

patch('src/systems/PlayerSystem.cpp', 'void PlayerSystem::reset() {',
      '''void PlayerSystem::reset() {
   // The same player entity survives sector transitions. Always refresh mode;
   // onAddedToWorld alone never handles water entry, exit or checkpoint death.
   setUnderwater(scene->getLevelData().levelType == LevelType::UNDERWATER);''')
patch('src/systems/PlayerSystem.cpp', '''   if (underwater) {
      underwater = false;
   }

   if (!outOfBounds && (isSuperMario() || isFireMario())) {''',
      '''   if (!outOfBounds && (isSuperMario() || isFireMario())) {''')
patch('src/systems/PlayerSystem.cpp', '''      shrink(world);
      return;
   }

   auto* move = mario->getComponent<MovingComponent>();''',
      '''      shrink(world);
      return;
   }

   underwater = false; // Death animation only; taking damage keeps swimming.
   auto* move = mario->getComponent<MovingComponent>();''')
patch('src/systems/FlagSystem.cpp',
      'void FlagSystem::climbFlag(World* world, Entity* player, Entity* flag) {',
      '''void FlagSystem::climbFlag(World* world, Entity* player, Entity* flag) {
   scene->setUnderwater(false); // Surface airlock uses the dry exit sequence.''')

# Fix the pinned sentinel's right-only walking bug. Do not change player motion.
patch('src/systems/MapSystem.cpp', '''                } else {
                   move->velocity.x = 1.0;
                   bowserComponent->lastMoveDirection = Direction::LEFT;''',
      '''                } else {
                   move->velocity.x = -1.0;
                   bowserComponent->lastMoveDirection = Direction::LEFT;''')
patch('include/ECS/Components.h', '   float distanceMoved = 0;',
      '   float distanceMoved = 0;\n   float arenaMinX = 0;\n   float arenaMaxX = 0;')
patch('src/systems/MapSystem.cpp',
      '         bowser->addComponent<BowserComponent>(bowserAttacks, bowserMovements);',
      '''         auto* sentinel = bowser->addComponent<BowserComponent>(bowserAttacks, bowserMovements);
         sentinel->arenaMinX = position->getLeft() - 3 * SCALED_CUBE_SIZE;
         sentinel->arenaMaxX = position->getLeft() + 3 * SCALED_CUBE_SIZE;''')
patch('src/systems/EnemySystem.cpp',
      '   auto* bowserTexture = entity->getComponent<TextureComponent>();',
      '''   auto* bowserTexture = entity->getComponent<TextureComponent>();
   auto* sentinelPosition = entity->getComponent<PositionComponent>();
   auto* sentinelMove = entity->getComponent<MovingComponent>();
   if (sentinelPosition->getLeft() < bowserComponent->arenaMinX) {
      sentinelPosition->setLeft(bowserComponent->arenaMinX);
      sentinelMove->velocity.x = 1.0;
   } else if (sentinelPosition->getLeft() > bowserComponent->arenaMaxX) {
      sentinelPosition->setLeft(bowserComponent->arenaMaxX);
      sentinelMove->velocity.x = -1.0;
   }''')
# Shut off the arena by reaching the upper route and cut-off terminal. The
# existing bridge-collapse sequence provides completion without requiring fire.
# Projectile HP is intentionally not invented: upstream never implemented it.
patch('src/systems/MapSystem.cpp', '''         entity->addComponent<MovingComponent>(Vector2f(-3.0, 0), Vector2f(0, 0));''',
      '''         entity->addComponent<AnimationComponent>(std::vector<int>{90, 91}, 8,
                                                  Map::EnemyIDCoordinates);
         entity->addComponent<MovingComponent>(Vector2f(-3.0, 0), Vector2f(0, 0));''')

patch('include/TextureManager.h', '   void SetBackgroundColor(BackgroundColor color);',
      '   void SetBackgroundColor(BackgroundColor color);\n   void SetCampaignBackdrop(int biome);')
patch('include/TextureManager.h', '   BackgroundColor currentColor;',
      '''   BackgroundColor currentColor;
   std::shared_ptr<SDL_Texture> campaignBackdrops[4];
   std::shared_ptr<SDL_Texture> campaignBackdrop;''')
texture=(engine/'src/TextureManager.cpp').read_text()
native='SB_ASSET_PATH(' in texture
paths=[f'"res/sprites/backgrounds/{name}.png"' for name in
       ('alien_sky','underwater_observatory','crystal_quarry','reactor_core')]
if native:
    paths=[f'SB_ASSET_PATH({p})' for p in paths]
method='''void TextureManager::SetCampaignBackdrop(int biome) {
   if (biome < 0 || biome > 3) biome = 0;
   const char* paths[] = {PATHS};
   if (!campaignBackdrops[biome])
      campaignBackdrops[biome] = LoadSharedTexture(paths[biome], false);
   campaignBackdrop = campaignBackdrops[biome];
}

'''.replace('PATHS',', '.join(paths))
patch('src/TextureManager.cpp', 'void TextureManager::Clear() {',method+'void TextureManager::Clear() {')
prefix='''   if (currentColor == BackgroundColor::BLUE && campaignBackdrop)
      SDL_RenderCopy(renderer, campaignBackdrop.get(), nullptr, &backgroundRect);
   else'''
if native:
    patch('src/TextureManager.cpp',
          '   if (currentColor == BackgroundColor::BLUE && sbBackgroundTexture)',
          prefix+' if (currentColor == BackgroundColor::BLUE && sbBackgroundTexture)')
else:
    patch('src/TextureManager.cpp', '   SDL_RenderFillRect(renderer, &backgroundRect);',
          prefix+'\n      SDL_RenderFillRect(renderer, &backgroundRect);')
patch('src/TextureManager.cpp', '   SDL_DestroyRenderer(renderer);',
      '''   campaignBackdrop.reset();
   for (auto& backdrop : campaignBackdrops) backdrop.reset();
   SDL_DestroyRenderer(renderer);''')
patch('src/scenes/GameScene.cpp', '          playerSystem->reset();',
      '''          const int biome = (level == 5) ? 1 : ((level == 2) ? 2 :
                                ((level == 6 || level == 8) ? 3 : 0));
          TextureManager::Get().SetCampaignBackdrop(biome);
          playerSystem->reset();''')
print('Deep Signal: reset-safe swim, four cached backdrops, sentry animation, bounded sentinel AI')
