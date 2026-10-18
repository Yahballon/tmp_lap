#!/usr/bin/env bash
# Fetch verified, redistributable Piper voice models for PS5's *native* ONNX runtime.
# The German Thorsten model is installed by the previous workflow step.
set -Eeuo pipefail
OUT="${1:-/tmp/ps5-piper-voices}"
mkdir -p "$OUT"
BASE=https://huggingface.co/rhasspy/piper-voices/resolve/main
fetch_one() {
  local lang="$1" dir="$2" model="$3" sha="$4" locale="$5"
  local dest="$OUT/$lang"
  mkdir -p "$dest"
  echo "[PIPER_MODEL] Downloading $model and validating SHA256"
  curl -fL --retry 3 --connect-timeout 25 --max-time 600 \
    "$BASE/$dir/$model.onnx" -o "$dest/$model.onnx"
  echo "$sha  $dest/$model.onnx" | sha256sum --check --status
  curl -fL --retry 3 --connect-timeout 25 --max-time 60 \
    "$BASE/$dir/$model.onnx.json" -o "$dest/$model.onnx.json"
  curl -fL --retry 3 --connect-timeout 25 --max-time 60 \
    "$BASE/$dir/MODEL_CARD" -o "$dest/MODEL_CARD"
  python3 - "$dest/$model.onnx.json" "$locale" <<'PY'
import json,sys
from pathlib import Path
c=json.loads(Path(sys.argv[1]).read_text())
assert c["language"]["code"]==sys.argv[2],(c["language"]["code"],sys.argv[2])
assert c["audio"]["sample_rate"]==22050
assert c["phoneme_type"]=="espeak"
assert c["num_speakers"]==1
assert len(c["phoneme_id_map"])>=140
assert c["phoneme_id_map"]["^"]==[1]
assert c["phoneme_id_map"]["$"]==[2]
print("Verified Piper config:",c["language"]["code"])
PY
}
# EN Kristin is based on LibriVox public-domain speech. FR Siwis
# has CC BY 4.0 source data; ES davefx source data is CC0.
fetch_one en en/en_US/kristin/medium en_US-kristin-medium \
  5849957f929cbf720c258f8458692d6103fff2f0e3d3b19c8259474bb06a18d4 en_US
fetch_one fr fr/fr_FR/siwis/medium fr_FR-siwis-medium \
  641d1ab097da2b81128c076810edb052b385decc8be3381814802a64a73baf99 fr_FR
fetch_one es es/es_ES/davefx/medium es_ES-davefx-medium \
  6658b03b1a6c316ee4c265a9896abc1393353c2d9e1bca7d66c2c442e222a917 es_ES
# Russian Denis medium is trained on CC0 voice-datasets, unlike the
# noncommercial Ruslan dataset; include the original model card in the ZIP.
fetch_one ru ru/ru_RU/denis/medium ru_RU-denis-medium \
  15fab56e11a097858ee115545d0f697fc2a316c41a291a5362349fb870411b0a ru_RU
echo "[PIPER_MODEL] All 4 additional language models downloaded and SHA256 verified."
