// Exercise the actual patched Gold872 regex parser against every shipped stage.
#include "Level.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char** argv) {
   assert(argc == 2);
   for (int world = 1; world <= 8; ++world) {
      for (int stage = 1; stage <= 4; ++stage) {
         const std::string name = "World" + std::to_string(world) + "-" + std::to_string(stage);
         std::ifstream file(std::string(argv[1]) + "/" + name + "/" + name + ".levelproperties");
         assert(file.is_open());
         std::ostringstream buffer;
         buffer << file.rdbuf();
         Level level;
         level.loadLevelData(buffer.str());
         auto& data = level.getData();
         assert(data.playerStart == Vector2i(2, 12));
         assert(data.checkpoint == Vector2i(98, 12));
         assert(data.cameraMax == 191 + (world - 1) * 4);
         assert(data.levelType == LevelType::OVERWORLD);
         assert(data.levelBackgroundColor == BackgroundColor::BLUE);
         const Vector2i next = stage < 4 ? Vector2i(world, stage + 1)
                                        : (world < 8 ? Vector2i(world + 1, 1) : Vector2i(0, 0));
         assert(data.nextLevel == next);
         assert(data.floatingTextLocations.size() == (world == 1 && stage == 1 ? 6 : 4));
         assert(std::get<1>(data.floatingTextLocations[2]) == "SERVICE CHECKPOINT");
         assert(data.warpPipeLocations.empty());
         assert(data.platformLevelLocations.empty());
         assert(data.fireBarLocations.empty());
         assert(data.vineLocations.empty());
         for (const auto& lift : data.movingPlatformDirections) {
            assert(std::get<1>(lift) == PlatformMotionType::BACK_AND_FORTH);
            assert(std::get<2>(lift) == Direction::UP);
            assert(std::get<3>(lift) == Vector2i(7, 11));
         }
         level.clearLevelData();
         assert(level.getData().checkpoint == Vector2i(0, 0));
         assert(level.getData().movingPlatformDirections.empty());
      }
   }
   std::cout << "PASS: actual engine parser loaded all 32 campaign properties\n";
}
