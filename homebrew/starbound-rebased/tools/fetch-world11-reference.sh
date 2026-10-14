#!/usr/bin/env bash
set -euo pipefail
revision=609c5af217abf6eb55d3eb594df4db76cfb0c90c
target="${1:-build/gold872-reference}"
git clone --quiet --filter=blob:none --no-checkout https://github.com/Gold872/Super-Mario-Bros.git "$target"
git -C "$target" sparse-checkout init --no-cone
printf '%s\n' \
  '/res/data/World1-1/' \
  '/res/data/MenuBackground/MenuBackground_Background.csv' \
  '/res/sprites/blocks/BlockTileSheet.png' \
  '/res/sprites/blocks/IrregularReferences.blockmap' \
  '/res/sprites/characters/PlayerSpriteSheet.png' \
  '/res/sprites/characters/EnemySpriteSheet.png' \
  | git -C "$target" sparse-checkout set --stdin
git -C "$target" checkout --quiet "$revision"
test "$(git -C "$target" rev-parse HEAD)" = "$revision"
for file in \
 'res/sprites/blocks/BlockTileSheet.png' \
 'res/sprites/blocks/IrregularReferences.blockmap' \
 'res/sprites/characters/PlayerSpriteSheet.png' \
 'res/sprites/characters/EnemySpriteSheet.png' \
 'res/data/World1-1/World1-1.levelproperties' \
 'res/data/World1-1/World1-1_Foreground.csv' \
 'res/data/World1-1/World1-1_Background.csv' \
 'res/data/World1-1/World1-1_Underground.csv' \
 'res/data/World1-1/World1-1_Enemies.csv' \
 'res/data/World1-1/World1-1_Above_Foreground.csv' \
 'res/data/World1-1/World1-1_Collectibles.csv' \
 'res/data/MenuBackground/MenuBackground_Background.csv'; do
 test -s "$target/$file" || { echo "MISSING UPSTREAM $file" >&2; exit 1; }
done
echo "Pinned Gold872 world 1-1 + atlas source copied separately for temporary compatibility validation"
