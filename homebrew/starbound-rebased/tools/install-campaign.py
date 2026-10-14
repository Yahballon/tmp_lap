#!/usr/bin/env python3
"""Install authored maps and patch the pinned engine's campaign integration."""
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
engine = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('build/gold872-engine')

def patch(path, old, new):
    file = engine / path
    text = file.read_text()
    if text.count(old) != 1:
        raise RuntimeError(f'{path}: expected exactly one patch anchor: {old[:80]}')
    file.write_text(text.replace(old, new), encoding='utf-8')

shutil.copytree(ROOT / 'campaign', ROOT / 'res/data', dirs_exist_ok=True)
patch('include/systems/MenuSystem.h', 'const int maxLevel = 1;\n   const int maxSublevel = 1;',
      'const int maxLevel = 8;\n   const int maxSublevel = 4;')
patch('include/Level.h', '   Vector2i playerStart;',
      '   Vector2i playerStart;\n   Vector2i checkpoint = Vector2i(0, 0);')
patch('include/Level.h', '      data.nextLevel =\n          loadCoordinate(',
      '      data.checkpoint = loadCoordinate("CHECKPOINT(?:\\\\s)?=(?:\\\\s)?" + vectorPattern, levelProperties);\n\n      data.nextLevel =\n          loadCoordinate(')
patch('include/Level.h', '      data.playerStart = Vector2i(0, 0);',
      '      data.playerStart = Vector2i(0, 0);\n      data.checkpoint = Vector2i(0, 0);')
patch('include/scenes/GameScene.h', '   bool gameFinished = false;',
      '   bool checkpointActive = false;\n   bool campaignLoading = true;\n   bool gameFinished = false;')
patch('src/scenes/GameScene.cpp', '   emptyCommandQueue();', '''   emptyCommandQueue();
   // Only accept a living player standing on the authored service pad. This
   // runs on the main thread, outside the asynchronous entity loader.
   if (!campaignLoading && !paused && !checkpointActive && !gameWon &&
       PlayerSystem::isInputEnabled()) {
      Entity* player = world->findFirst<PlayerComponent>();
      const Vector2i pad = getLevelData().checkpoint;
      if (player && pad.x > 0 && !player->hasComponent<DeadComponent>()) {
         auto* pos = player->getComponent<PositionComponent>();
         auto* move = player->getComponent<MovingComponent>();
         const float x = pos->getLeft() / SCALED_CUBE_SIZE;
         if (x >= pad.x && x < pad.x + 2 && move->velocity.y == 0 &&
             std::abs(pos->getBottom() - (pad.y + 1) * SCALED_CUBE_SIZE) < 1.0f) {
            checkpointActive = true;
            Entity* message = world->create();
            message->addComponent<PositionComponent>(
                Vector2f(pad.x - 3, 7) * SCALED_CUBE_SIZE, Vector2i());
            message->addComponent<TextComponent>("CHECKPOINT SAVED", 12, true, true);
            message->addComponent<FloatingTextComponent>();
         }
      }
   }''')
patch('src/scenes/GameScene.cpp', 'void GameScene::setupLevel() {',
      'void GameScene::setupLevel() {\n   campaignLoading = true;')
# Constructor and setupLevel each call loadLevel, so anchor on the setup-only
# background transition. Restarts retain the checkpoint, transitions clear it.
patch('src/scenes/GameScene.cpp', '''   loadLevel(level, subLevel);

   TextureManager::Get().SetBackgroundColor(BackgroundColor::BLACK);

   scoreSystem->showTransitionEntities();''', '''   loadLevel(level, subLevel);
   if (checkpointActive) {
      auto& data = getLevelData();
      data.playerStart = data.checkpoint;
      data.cameraStart = Vector2i(std::max(0, data.checkpoint.x - 8), 0);
   }

   TextureManager::Get().SetBackgroundColor(BackgroundColor::BLACK);

   scoreSystem->showTransitionEntities();''')
patch('src/scenes/GameScene.cpp', '          playerSystem->reset();',
      '          playerSystem->reset();\n          campaignLoading = false;')
patch('src/scenes/GameScene.cpp', '      this->level = level;',
      '      checkpointActive = false;\n      this->level = level;')
patch('src/scenes/GameScene.cpp', '         gameWon = true;', '''         gameWon = true;
         Entity* finale = world->create();
         finale->addComponent<PositionComponent>(Vector2f(100, 180), Vector2i());
         finale->addComponent<TextComponent>("RELAYFALL - SIGNAL RESTORED", 18);
         Entity* prompt = world->create();
         prompt->addComponent<PositionComponent>(Vector2f(160, 240), Vector2i());
         prompt->addComponent<TextComponent>("CONFIRM TO RETURN TO MENU", 16);''')
# The flag sequence also completes the last stage; avoid a hidden avatar and
# silent ending before Core displays its existing campaign-complete screen.
patch('src/systems/FlagSystem.cpp', '''          player->getComponent<TextureComponent>()->setVisible(false);

          inSequence = false;''', '''          if (nextLevel != Vector2i(0, 0)) {
             player->getComponent<TextureComponent>()->setVisible(false);
          } else {
             Entity* winMusic(world->create());
             winMusic->addComponent<MusicComponent>(MusicID::GAME_WON);
          }

          inSequence = false;''')
print('Installed 32 authored maps; enabled selection, service checkpoints and finale')
