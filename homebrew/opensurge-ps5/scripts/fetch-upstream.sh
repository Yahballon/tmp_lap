#!/usr/bin/env bash
# Fetch precisely the audited upstream releases for Open Surge's engine port.
set -euo pipefail
project=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
destination=${1:-"$project/.work"}
mkdir -p "$destination"
for tool in git python3; do command -v "$tool" >/dev/null || { echo "Missing $tool" >&2; exit 2; }; done

while IFS='|' read -r name repository tag commit; do
  target="$destination/$name"
  if [[ ! -d "$target/.git" ]]; then
    git clone --quiet --depth=1 --branch "$tag" "https://github.com/$repository.git" "$target"
  fi
  actual=$(git -C "$target" rev-parse HEAD)
  if [[ "$actual" != "$commit" ]]; then
    echo "Source mismatch for $name: expected $commit, got $actual. Delete $target and rerun." >&2
    exit 3
  fi
  if [[ -n $(git -C "$target" status --porcelain) ]]; then
    echo "Dirty upstream checkout: $target" >&2
    exit 4
  fi
  printf '%s %s %s\n' "$name" "$tag" "$commit"
done < <(python3 - "$project/upstream.lock.json" <<'PY'
import json,sys
lock=json.load(open(sys.argv[1], encoding="utf-8"))
assert lock["version"] == 1
for name in ("opensurge","surgescript","allegro","physfs"):
    x=lock[name]
    assert len(x["commit"]) == 40 and len(x["repository"].split("/")) == 2
    print("|".join((name,x["repository"],x["tag"],x["commit"])))
PY
)
