# Godot 4.7.2 auf PS5 – Meilensteine & Roadmap

**Projekt:** PS5-Godot-Homebrew-Port von YAHBALLON  
**Stand:** 23. Oktober 2026 · PS5 Firmware **5.10** · Entwicklungsbranch `feature/godot-ps5-engine-foundation` · [PR #207](https://github.com/Yahballon/tmp_lap/pull/207)

> **Aktueller Meilenstein: M10a ECHTE DYNAMISCHE GODOT-2D-PHYSIK AUF PS5 BESTÄTIGT (23. Oktober 2026).** `PPSA99847 v0.1.3` lädt das externe `dynamics-area-smoke.pck` ohne neue native Kompilierung. Nutzer bestätigt, dass die gelbe Kugel durch den cyanfarbenen Sensor fällt und auf der pinken Plattform landet. **Zwei vollständige PS5-Logläufe in `bfpilot-download (22).tar`** belegen `GODOT_PS5_M10A_AREA_ENTER_OK`, `GODOT_PS5_M10A_AREA_EXIT_OK`, `GODOT_PS5_M10A_RIGID_CONTACT_OK`, `GODOT_PS5_M10A_TEST_PASS ticks=143 rest_y=777.299987792969 velocity=(0.0, 0.0)`, `GODOT_RETURN=0`, `NORMAL_SHUTDOWN`, `EGL_CLOSED`. **M10a Hardware-Abnahme bestanden.** **M10b a/b jetzt im originalen Godot-4.7.2-Host erfolgreich:** TileMapLayer-Kollision und externe TrueType/Label-UI; [CI #39878279649](https://github.com/Yahballon/tmp_lap/actions/runs/39878279649) grün, zwei getrennte PCK-Test-ZIPs erstellt. **PS5-Hardwaretests für beide M10b-Funktionen noch offen.**

## Bedeutung der Statusangaben

- **✅ Auf PS5 bestätigt:** physischer Bildschirmtest und/oder konkrete Konsolenlogs liegen vor.
- **🟡 CI/Release erfolgreich:** der Code hat Tests/Kompilierung/Signierung bestanden, aber die jeweilige neue Funktion **noch keinen Hardware-Nachweis**.
- **⬜ Offen:** noch nicht umgesetzt oder ohne ausreichenden Funktionsnachweis.

Diese Unterscheidung ist wichtig: Ein erfolgreich signiertes ELF/ZIP ist **kein** Beweis, dass ein Spiel auf PS5 sichtbar oder spielbar ist.

## Erreichte Meilensteine

| Phase | Meilenstein | Ergebnis / belastbarer Nachweis | Status |
| --- | --- | --- | --- |
| M0 | Godot 4.7.2 nativ für PS5 bauen | `x86_64-sie-ps5`, Clang 18, native statische Engine-Bibliotheken, PS5-Installationspakete | ✅ Build |
| M1 | Echte Godot-VM auf der PS5 starten | `PPSA99843`: `GODOT_PS5_SMOKE_READY`, echte PCK/GDScript-Ausführung, `NATIVE_GODOT_RETURN=0` | ✅ Auf PS5 |
| M2 | DualSense an Godot Input anbinden | `PPSA99843 v0.3.2`: **38** `GODOT_PS5_DUALSENSE_CROSS_PRESSED` in GDScript | ✅ Auf PS5 |
| M3 | PS5-GPU initialisieren | `PPSA99844 v0.1.5`: EGL, Mesa **OpenGL 4.6**, **GLSL 4.60**, `PS5 AGC`, 1920×1080; sichtbare Farbwechsel, **1.800 Frames** | ✅ Auf PS5 |
| M4 | Native Shader/Geometrie | `PPSA99844 v0.1.6`: farbiges Dreieck, GLSL-Shader, VAO/VBO, `GL_FIRST_DRAW_ERROR=0`; Screenshot | ✅ Auf PS5 |
| M5 | 2D-Texturen und Blending | `PPSA99844 v0.1.7`: sichtbare RGBA-Schachbretttextur, UV-Mapping, Alpha-Blending | ✅ Auf PS5 |
| M6 | Godot + GPU in **einem** Prozess | `PPSA99845 v0.1.1`: echte Godot-GDScript-Szene steuert native OpenGL-Brücke; **24 X-Eingaben**, sichtbares Schachbrett, sauberes Ende in späterem Log | ✅ Auf PS5 |
| M7 | **Godots eigener GLES3-/Canvas2D-Renderer** | `PPSA99847 v0.1.0`: `DisplayServerPS5`, `RasterizerGLES3`, `Sprite2D`, `ColorRect`, Controller; **4.388 Godot-Swaps über vier Sitzungen**, **223 X-Meldungen**, Screenshot und sauberer Exit | ✅ Auf PS5 |
| M8a | Fremde Godot-PCK explizit auswählen | `PPSA99847 v0.1.2`: Externes `external-smoke.pck` über `launch.txt` auf PS5 geladen; Nutzer bestätigt Szenen und X-Interaktion | ✅ Auf PS5 vom Nutzer bestätigt (M8c) |
| M8b | Erstes Godot-Spiel als Hardwaretest | `PPSA99847 v0.1.2`: **Pong** läuft sichtbar auf echter PS5; Video zeigt Ballbewegung, Schläger und Punktestand. Individuelle Eingaben/Punkte/sauberer Exit ohne neue Pong-Logs nicht unabhängig nachgewiesen. | ✅ Auf PS5 sichtbar/laufend; 🟡 Detailabnahme offen |
| M8c | Eigenes externes Godot-4.7.2-PCK ohne eboot.bin-Neubau | Zwei Szenen, `res://palette.json`, GDScript, Canvas2D, DualSense-X; grüner CI-Lauf **39866584380** und Nutzerbericht „geht auch alles wie es soll“ | ✅ PS5-Funktion vom Nutzer bestätigt; 🟡 Hardware-Logs noch ausstehend |
| M9a | **Real PhysicsServer2D collision PCK** | Original-Godot-4.7.2-PCK mit `CharacterBody2D`, `StaticBody2D`, `CollisionShape2D`, `move_and_collide()`; Host-CI 39869677215 erfolgreich; auf PS5 mit v0.1.2 zunächst Dummy-Physics/Timeout, mit v0.1.3 echter Kontakt | ✅ PCK auf PS5 mit M9b erfolgreich; v0.1.2 ursprünglich fehlgeschlagen |
| M9b | **Native GodotPhysics2D / PPSA99847 v0.1.3** | Build [#39872101079](https://github.com/Yahballon/tmp_lap/actions/runs/39872101079) erfolgreich; `bfpilot-download (21).tar`: **2×** `REAL_COLLISION_OK`, `TEST_PASS physics_frames=369`, `GODOT_RETURN=0`, `EGL_CLOSED`, ohne Dummy-Warnung in den erfolgreichen Sitzungen | ✅ **Auf PS5 mit Logs bestätigt** |
| M10a | **Real RigidBody2D-Schwerkraft + Area2D-Signale (externes PCK)** | Echte PS5, Nutzerbericht und **2 vollständige Logläufe** aus `bfpilot-download (22).tar`: Eintritt y=409.2584, Austritt y=646.6695, Bodenkontakt y=777.4374; `TEST_PASS ticks=143`, Resthöhe 777.3000, Geschwindigkeit (0,0), `GODOT_RETURN=0`, `EGL_CLOSED`. Host CI [#39876290256](https://github.com/Yahballon/tmp_lap/actions/runs/39876290256) ebenfalls grün | ✅ **PS5 Hardware bestätigt (2×)** |
| M10b-a | **TileMapLayer + TileSetAtlasSource / echte Level-Kollision** | Godot 4.7.2 Host: neun Atlas-Tiles, echte `TileData`-Physikpolygone, `CharacterBody2D`-Kollision mit **TileMapLayer** selbst, `normal=(0,-1)`, y=742.984375, `TILE_TEST_PASS tick=139`; [CI #39878279649](https://github.com/Yahballon/tmp_lap/actions/runs/39878279649) grün | 🟡 Host bestanden, **PS5 offen** |
| M10b-b | **Control/Label + TrueType-Font** | Host: echtes externes Press Start 2P `.ttf` 118.204 Byte inkl. SIL OFL/FONTLOG im PCK; `FONT_TTF_LOADED`, `FONT_METRICS_OK size=(392,28)`, fünf Labels, `FONT_TEST_FINISHED frames=480`; [CI #39878279649](https://github.com/Yahballon/tmp_lap/actions/runs/39878279649) grün. Sichtbarer Text kann nur auf PS5 bestätigt werden | 🟡 Host bestanden, **PS5 offen** |

**Wichtig zu M6/M7:** M6 benutzte noch einen **eigenen nativen OpenGL-Zeichner**. **M7 ist der eigentliche Renderer-Durchbruch:** Das Schachbrett wird dort vom **originalen Godot-Canvas2D** gezeichnet, nicht von unserem Test-Shader. Die native Hülle besitzt nur EGL-Kontext, GL-Prozedurauflösung und Präsentation.

### Wichtige Hardware- und Build-Nachweise

| Nachweis | Quelle |
| --- | --- |
| Headless Engine + Pad | `bfpilot-download (14).tar`, `engine-stdout.log` / `engine-stderr.log`, `PPSA99843 v0.3.2` |
| Shader-Dreieck und Textur | PS5-Screenshots und `graphics-probe.log`, `PPSA99844 v0.1.6–0.1.7` |
| Szene → native OpenGL-Brücke | `bfpilot-download (16).tar`, `scene-bridge.log`, `PPSA99845 v0.1.1` |
| **Echtes Godot GLES3/Canvas2D** | PS5-Screenshot, `bfpilot-download (18).tar`, `gles3-runtime.log`, `gles3-stdout.log` |
| Pong v0.1.2 kompiliert/signiert | [Erfolgreicher GitHub Actions Build #39853780399](https://github.com/Yahballon/tmp_lap/actions/runs/39853780399) |
| **Pong v0.1.2 auf PS5 sichtbar und laufend** | Nutzer-Video `20261022_234930_00248487(1).webm` (13,88 s, 1920×1088, VP9), hier im Chat geprüft; dynamischer Ball und unterschiedliche Paddle-Positionen, kein Schwarzbild. Aussage des Nutzers: „geht bereits“. Keine neuen Pong-Logs eingereicht. |
| **M9b echte PhysicsServer2D-Kollision auf PS5** | Nutzer-TAR `bfpilot-download (21).tar`, jeweils 2× `GODOT_PS5_M9_REAL_COLLISION_OK`, `GODOT_PS5_M9_TEST_PASS physics_frames=369`, `GODOT_RETURN=0` und `EGL_CLOSED`; keine Dummy-Warnung in den letzten beiden stderr-Sessions |
| **M10a RigidBody2D und Area2D auf echter PS5** | Nutzer-TAR `bfpilot-download (22).tar`: zwei Sessions mit `GODOT_PS5_M10A_AREA_ENTER_OK`, `GODOT_PS5_M10A_AREA_EXIT_OK`, `GODOT_PS5_M10A_RIGID_CONTACT_OK`, `GODOT_PS5_M10A_TEST_PASS ticks=143`, plus `PACK_SOURCE=CONFIG`, `GODOT_RETURN=0`, `NORMAL_SHUTDOWN`, `EGL_CLOSED` |
| Echter GLES3-Test-Quellcode | [`gles3-canvas/README.md`](gles3-canvas/README.md), [`display_server_ps5.cpp`](engine/platform/ps5/display_server_ps5.cpp) |

Die vier geprüften GLES3-Sitzungen aus `bfpilot-download (18).tar` enthalten jeweils **1001 / 1145 / 1136 / 1106** Swap-Aufrufe, `GODOT_RETURN=0`, `NORMAL_SHUTDOWN` und `EGL_CLOSED`. Insgesamt wurden **223** `GODOT_PS5_REAL_2D_CROSS_PRESSED`-Meldungen protokolliert. Diese Werte gehören zur **bestätigten GLES3-Checkerboard-Version v0.1.0**, nicht zum neuen Pong.

## Aktuelle TESTBUILD-Titel

| Title-ID | App / Version | Zweck | Hardwarestand |
| --- | --- | --- | --- |
| `PPSA99842` | Godot PS5 Probe v0.1.0 | Frühes Native-Build-Grundgerüst | Diagnose/Legacy |
| `PPSA99843` | [Engine Smoke v0.3.2](https://github.com/Yahballon/tmp_lap/releases/tag/godot-ps5-engine-smoke-v0.3.2) | Original Godot VM, PCK, GDScript, DualSense | ✅ bestätigt |
| `PPSA99844` | [GPU Probe v0.1.7](https://github.com/Yahballon/tmp_lap/releases/tag/godot-ps5-gpu-probe-v0.1.7) | EGL/GL, Triangle, Textur, Alpha | ✅ bestätigt |
| `PPSA99845` | [Scene Bridge v0.1.2](https://github.com/Yahballon/tmp_lap/releases/tag/godot-ps5-scene-bridge-v0.1.2) | Godot-Szene → **eigener** OpenGL-Renderer, Performance-Messung | ✅ **v0.1.1** bestätigt; v0.1.2 Diagnoseänderung |
| **`PPSA99847`** | **[GLES3 Canvas v0.1.2 + M9b Physics2D v0.1.3](https://github.com/Yahballon/tmp_lap/releases/tag/godot-ps5-gles3-canvas-v0.1.2)** | **Echter Godot-Renderer mit neuem Pong-PCK** | ✅ Pong v0.1.2 sowie Physik v0.1.3 auf PS5 bestätigt (2 Log-Sessions) |

Alle genannten Versionen wurden mit dem `main/version.json`-Katalog abgeglichen. **Installierbare Builds kommen nur über erfolgreiche signierte GitHub Releases in TESTBUILD.** Bestehende Title-IDs und die bestätigten älteren Releases bleiben für Regressionstests erhalten.

## Wo wir aktuell stehen

```text
PS5 / Firmware 5.10
  └─ PPSA99847: Godot 4.7.2 / echtes GLES3 Canvas2D ✅
       ├─ PS5-Payload-SDK + Godot Main Loop ✅
       ├─ GDScript + SceneTree + Resource/PCK ✅ (eingeschränkt getestet)
       ├─ DualSense scePad → Godot Input ✅ (X sicher bestätigt)
       ├─ DisplayServerPS5 + RasterizerGLES3 ✅
       ├─ PS5 AGC / EGL / OpenGL 4.6 / GLAD ✅
       ├─ Shader/Texture/Sprite2D/ColorRect ✅
       ├─ Externe PCK-Auswahl ✅ (M8c auf PS5 vom Nutzer bestätigt)
       ├─ Pong Godot 2D ✅ (PS5-Video: sichtbar und laufend; Detail-Logs noch offen)
       └─ Fonts / Audio / Physik / Saves / 3D / allgemeine Spiele ⬜
```

**Der Status ist „funktionierender experimenteller Godot-2D-Renderer“, nicht „vollständige oder universelle Godot-PS5-Engine“.** Insbesondere darf aus der selbstgeschriebenen Pong-Kollision nicht auf einen funktionierenden Godot-PhysicsServer2D geschlossen werden.

## M8c – Independent external PCK acceptance fixture (23 October 2026)

**Status: ✅ Physical PS5 M8c behavior confirmed by user on 23 October 2026; 🟡 diagnostic logs/video not provided for this fixture.** The user tested the standalone `external-smoke.pck` with the existing `PPSA99847 v0.1.2` and reported that both Godot scenes and X-triggered color changes work as intended. This is real **user-reported console acceptance**, not just a host simulation. Official Godot 4.7.2 PCK generation, two-scene/asset runtime acceptance, header/ZIP/checksum verification and artifact publication all passed [CI run #39866584380](https://github.com/Yahballon/tmp_lap/actions/runs/39866584380). [PR #235](https://github.com/Yahballon/tmp_lap/pull/235) merged at `7f3fe710f9d11e399b49f96092ab0461b43dade3`. Do not claim measured FPS, independent shutdown traces or generalized third-party game support.

- New standalone project: [`samples/external-pack-smoke/`](samples/external-pack-smoke/README.md), deliberately **not** linked into the native PPSA99847 binary.
- New [`build-godot-ps5-external-pck.yml`](../../.github/workflows/build-godot-ps5-external-pck.yml) produces a genuine `external-smoke.pck` with official Godot 4.7.2, verifies markers in a Linux/headless game run, then runs strict PCK-header/ZIP checks. A passing workflow uploads artifact **`Godot-PS5-M8c-External-PCK-PS5-READY-HOST-TESTED`**, containing `external-smoke.pck` and nested **`GodotPS5-M8c-External-PCK-PS5-TEST.zip`**. CI is now **confirmed successful** in [run #39866584380](https://github.com/Yahballon/tmp_lap/actions/runs/39866584380), artifact `Godot-PS5-M8c-External-PCK-PS5-READY-HOST-TESTED` (artifact ID `12464481166`).
- The nested PS5-ready ZIP maps `GodotPS5/launch.txt`, `GodotPS5/games/external-smoke.pck`, instructions and SHA-256 so the user can copy its `GodotPS5` contents to `/data/GodotPS5` without modifying the known-good native runner. Packager [`make_ps5_test_bundle.py`](samples/external-pack-smoke/make_ps5_test_bundle.py) and mock regression [`test_external_pck_bundle.py`](tests/test_external_pck_bundle.py) are committed. Branch pushes invoke the workflow automatically; workflow_dispatch may not appear until the YAML is on the default branch.
- The PCK contains two `Node2D` scenes, their GDScripts and a `res://palette.json` resource. It tests **real** `FileAccess.open`, `JSON.parse_string`, scene switching, CanvasItem primitive rendering, X input and clean engine exit.
- Use `PPSA99847 v0.1.2` unchanged: copy artifact `external-smoke.pck` to `/data/GodotPS5/games/external-smoke.pck`; create `/data/GodotPS5/launch.txt` containing `external-smoke.pck`. Remove `launch.txt` afterward to restore the bundled Pong game.
- Expected `gles3-runtime.log`: `PACK_SOURCE=CONFIG`, `GODOT_SWAP_FIRST`, `GODOT_RETURN=0`, `NORMAL_SHUTDOWN`, `EGL_CLOSED`.
- Expected `gles3-stdout.log`: `GODOT_PS5_EXTERNAL_MAIN_READY`, `GODOT_PS5_EXTERNAL_ASSET_OK`, `GODOT_PS5_EXTERNAL_SCENE_CHANGE_REQUESTED`, `GODOT_PS5_EXTERNAL_SECOND_SCENE_READY`, `GODOT_PS5_EXTERNAL_FINISHED`; optional pad proof `GODOT_PS5_EXTERNAL_GAMEPAD_X`.
- **Acceptance result:** user confirms PS5 display, independent PCK loading, scenes and X behavior. **Hardware logs and independent video remain optional follow-up evidence** for accurate frame-time, resource-open and exit diagnostics. Do not relabel those measurements as proven without logs.
- No claims yet for broad Godot compatibility, saving, text rendering, PhysicsServer2D, audio or 3D.

## M9a – Real upstream GodotPhysics2D collision acceptance (23 October 2026)

**Status: 🟡 Real Godot 4.7.2 host collision verified; PS5 physics hardware test required.**
Host Actions run [39869677215](https://github.com/Yahballon/tmp_lap/actions/runs/39869677215) is **SUCCESS**. [PR #236](https://github.com/Yahballon/tmp_lap/pull/236) merged to this branch as `83fa6e4a6f2018ee04e4df659886c9fb1a7ad761`. Existing known-good `PPSA99847 v0.1.2` native executable was **not changed**.

- New [`samples/physics-smoke/`](samples/physics-smoke/README.md) contains a true Godot `CharacterBody2D` moving with `move_and_collide()`, a `StaticBody2D` wall, and engine `CollisionShape2D/RectangleShape2D` geometry. There is **no hand-coded contact/bounce**.
- Desktop Godot reported `GODOT_PS5_M9_REAL_COLLISION_OK x=1093.984375 normal=(-1.0, 0.0)` and `GODOT_PS5_M9_TEST_PASS physics_frames=369`, then exited successfully. Earlier first run [39869575617](https://github.com/Yahballon/tmp_lap/actions/runs/39869575617) correctly failed because its `--quit-after` cap was too short after actual contact; [39869677215](https://github.com/Yahballon/tmp_lap/actions/runs/39869677215) corrected the cap and passed all host steps.
- Download [M9a test artifact](https://github.com/Yahballon/tmp_lap/actions/runs/39869677215/artifacts/12466240152) named `Godot-PS5-M9a-REAL-PHYSICS-Host-Validated-PS5-UNTESTED`; extract the nested `GodotPS5-M9a-REAL-PHYSICS-TEST.zip`. The `physics-smoke.pck` is 5,440 bytes, **SHA-256 `414583c3e2cd79de23d9344df0b8b44c1cd15aaa76961327e919214add2c5ea1`**. The ZIP includes `GodotPS5/launch.txt` with `physics-smoke.pck`.
- Copy the nested ZIP's `GodotPS5/` contents under `/data/GodotPS5/`. This **replaces the selected launch.txt**, but leaves Pong, M8c, and native PS5 title unchanged. Open `PPSA99847 v0.1.2`.
- **Success criterion on PS5:** cyan CharacterBody2D moves right and physically stops at magenta StaticBody2D wall; visual body turns green only after real `move_and_collide()` returns matching collider and normal; logs contain `GODOT_PS5_M9_REAL_COLLISION_OK` and `GODOT_PS5_M9_TEST_PASS`. Also check native `gles3-runtime.log` for `PACK_SOURCE=CONFIG`, `GODOT_RETURN=0`, `NORMAL_SHUTDOWN`, and `EGL_CLOSED`.
- **Failure diagnostic:** if the square passes through the wall or prints `GODOT_PS5_M9_NO_COLLISION_PHYSICS_UNAVAILABLE`, the PS5 executable still needs native PhysicsServer2D support. **Do not mark M9 passed** simply because the desktop workflow or visible squares succeed. Capture `gles3-stdout.log`, `gles3-stderr.log`, and `gles3-runtime.log` before deciding whether to rebuild the native engine.

## M9b – Native GodotPhysics2D integration, PS5 v0.1.3 (23 October 2026)

**Status: ✅ auf echter PS5 erfolgreich, durch den Nutzer bestätigt und durch zwei vollständige Hardware-Log-Sessions unabhängig belegt.** Die native `GodotPhysics2D`-Implementierung ist geladen, echte Kollisionen und sauberer Exit funktionieren. Andere PhysicsServer2D-Funktionen, Performance und breite Spielekompatibilität müssen weiterhin separat getestet werden.

- Root cause observed on the user's PS5 M9a test: `Falling back to dummy PhysicsServer2D` and `GODOT_PS5_M9_TIMEOUT physics_frames=720`, resulting in `GODOT_RETURN=43`. The PCK and display rendered correctly, but the native engine had been compiled without the actual physics module.
- Fix in `scripts/build-engine.sh`: opt-in `GODOT_PS5_PHYSICS2D=1` enables the genuine Godot 4.7.2 `module_godot_physics_2d_enabled=yes` while retaining default behavior for previously verified versions.
- Dedicated [native M9b build #39872101079](https://github.com/Yahballon/tmp_lap/actions/runs/39872101079) **SUCCESS**: real GodotPhysics2D archive present, native PS5 engine compiled and linked, signed `eboot.bin` generated, `PPSA99847.zip` passed ZIP/integrity/title/version validation.
- PS5 package `PPSA99847`, version `01.000.004` (project v0.1.3), with default Pong and additional bundled `physics-smoke.pck`; explicit `launch.txt` still selects `/data/GodotPS5/games/physics-smoke.pck`. Build artifact `Godot-PS5-M9b-NATIVE-PHYSICS-v0.1.3-PS5-HARDWARE-UNTESTED`, artifact ID `12467676123`.
- [PR #238](https://github.com/Yahballon/tmp_lap/pull/238) merged to the engine foundation branch as `94bb83919c112e7ecafeff02a21b463b67d6f677`. Previous `PPSA99847 v0.1.2` remains available as regression fallback.
- **Hardware-Abnahme BESTANDEN (23.10.2026):** `bfpilot-download (21).tar` enthält zwei vollständige `physics-smoke.pck`-Läufe unter dem neuen `PPSA99847 v0.1.3`: je `GODOT_PS5_M9_BODIES_READY`, `GODOT_PS5_M9_REAL_COLLISION_OK x=1093.984375 normal=(-1.0, 0.0)` und `GODOT_PS5_M9_TEST_PASS physics_frames=369`; dazu `PACK_SOURCE=CONFIG`, `GODOT_RETURN=0`, `NORMAL_SHUTDOWN` und `EGL_CLOSED` im Runtime-Log. Zuvor im selben TAR enthalten: zwei alte Fehlversuche mit `GODOT_RETURN=43`, Dummy-Warnung und Timeout. Die beiden letzten Sessions enthalten **keine** Dummy-Warnung mehr.
- **Restwarnungen:** `Couldn't load file res://project.binary` und `Could not load global script cache` sind weiterhin im stderr der erfolgreichen Sitzungen vorhanden, blockieren hier nicht. Nur die getesteten Physics-Basics beanspruchen; noch keine Bestätigung für RigidBody2D, TileMaps, Areas, Joints, Audio, Fonts oder Savegames.

## M10a – Real rigid bodies, gravity and Area2D signals (23 October 2026)

**Status: ✅ HOST UND PHYSISCHE PS5 BESTANDEN.** Das unveränderte, bereits auf Hardware bestätigte `PPSA99847 v0.1.3` lädt das externe PCK und führt originale `RigidBody2D`-/`Area2D`-Physik aus. Zwei komplette aufeinanderfolgende Konsolenläufe durch `bfpilot-download (22).tar` bestätigt; keine neue native ELF-Datei erforderlich.

- Source and runbook: [`samples/dynamics-area-smoke/`](samples/dynamics-area-smoke/README.md). New [`build-godot-ps5-m10a-dynamics-area.yml`](../../.github/workflows/build-godot-ps5-m10a-dynamics-area.yml) builds a **genuine Godot 4.7.2 PCK** and starts it independently on a Linux host, then checks real engine physics signals and generates an installable PS5-ready *data-only* ZIP.
- CI **SUCCESS**: [run 39876290256](https://github.com/Yahballon/tmp_lap/actions/runs/39876290256), artifact `Godot-PS5-M10a-RIGIDBODY-AREA-HOST-PASS-PS5-UNTESTED` (ID `12468183558`). Standalone real Godot PCK SHA-256: `afec68be20df28c71119d8e8d23c3b19c8e81ace54739decbea2d810d8a908ff`.
- Host proof: `GODOT_PS5_M10A_AREA_ENTER_OK y=409.2584`, `GODOT_PS5_M10A_AREA_EXIT_OK y=646.6695`, `GODOT_PS5_M10A_RIGID_CONTACT_OK y=777.4374`, and `GODOT_PS5_M10A_TEST_PASS ticks=143 rest_y=777.3000 velocity=(0.0, 0.0)`. These are genuine Godot `Area2D.body_entered`, `Area2D.body_exited` and `RigidBody2D.body_entered` events with Godot-managed gravity and body position. No scripted movement or synthetic collisions.
- To test on PS5: extract **`GodotPS5-M10a-RIGIDBODY-AREA-TEST.zip`** inside the Actions artifact; copy `GodotPS5/` contents to `/data/GodotPS5/`. It adds `games/dynamics-area-smoke.pck` and writes `launch.txt` containing `dynamics-area-smoke.pck` (previous PCKs remain installed). Run existing `PPSA99847 v0.1.3`.
- Expected visual result: yellow ball falls through cyan trigger and lands on magenta platform, three signal lights turn green. Required console proof: `gles3-stdout.log` contains `GODOT_PS5_M10A_AREA_ENTER_OK`, `GODOT_PS5_M10A_AREA_EXIT_OK`, `GODOT_PS5_M10A_RIGID_CONTACT_OK`, `GODOT_PS5_M10A_TEST_PASS`; `gles3-runtime.log` contains `PACK_SOURCE=CONFIG`, `GODOT_RETURN=0`, `NORMAL_SHUTDOWN`, `EGL_CLOSED`.
- **Hardware acceptance COMPLETE:** User confirms the yellow ball falls through the cyan trigger and lands on the magenta platform. Two physical-console `gles3-stdout.log` sessions contain all four markers: `AREA_ENTER_OK y=409.258422851563`, `AREA_EXIT_OK y=646.669494628906`, `RIGID_CONTACT_OK y=777.437438964844`, `TEST_PASS ticks=143 rest_y=777.299987792969 velocity=(0.0,0.0)`. Corresponding `gles3-runtime.log` sessions show `PACK_SOURCE=CONFIG` with `/data/GodotPS5/games/dynamics-area-smoke.pck`, `TOTAL_GODOT_SWAP_COUNT=114/119`, `GODOT_RETURN=0`, `EGL_CLOSED`, `NORMAL_SHUTDOWN`. No M10a FAIL markers in either session.
- **Non-blocking diagnostics:** `gles3-stderr.log` still contains `Couldn't load file res://project.binary` and `Could not load global script cache`, yet M10a completes and exits normally. Follow up separately as a resource/cache compatibility issue. Still no verified support for `TileMapLayer`, `Label`/fonts, joints, audio, saves or arbitrary Godot projects.

## M10b – TileMapLayer-Kollision und echte Godot-UI/TrueType (23 October 2026)

**Status: 🟡 beide externen PCK-Projekte auf offizieller Desktop-Godot-4.7.2-Engine erfolgreich; beide physischen PS5-Hardwaretests stehen noch aus.** GitHub Actions [#39878279649](https://github.com/Yahballon/tmp_lap/actions/runs/39878279649) vollständig grün. [PR #242](https://github.com/Yahballon/tmp_lap/pull/242) dokumentiert Source, Host-Tests und separate Testpakete.

- **M10b-a TileMapLayer:** Eigenständiges `samples/m10b-tilemap/` erzeugt echte Textur, `TileSet`-Physikebene, `TileSetAtlasSource`, ein Kollisionspolygon pro Tile, neun `TileMapLayer`-Zellen. `CharacterBody2D.move_and_collide()` meldet im Host `GODOT_PS5_M10B_TILE_CONTACT_OK tilemap=RealTileMapPlatform normal=(0.0, -1.0) y=742.984375` und `GODOT_PS5_M10B_TILE_TEST_PASS tick=139`. **Kein vorgetäuschter Boden oder eigenes Kollisionssystem.**
- **M10b-b Text/Fonts:** Eigenständiges `samples/m10b-font-ui/` erstellt fünf echte Godot-`Label`-Nodes mit `FontFile.load_dynamic_font()`. Original Press Start 2P aus `google/fonts` auf Revision `92345ac0dbb28d27dbd32f3a782e84c55eaac214`, License `SIL OFL 1.1`, `OFL.txt` und `FONTLOG.txt` in PCK mitgeliefert. Host-Marker: `GODOT_PS5_M10B_FONT_RESOURCE_OK size=118204`, `GODOT_PS5_M10B_FONT_METRICS_OK size=(392.0, 28.0)`, `GODOT_PS5_M10B_LABEL_TREE_READY count=5`, `GODOT_PS5_M10B_FONT_TEST_FINISHED frames=480`. **Headless kann sichtbare GPU-Glyphen nicht bestätigen.**
- Host-erzeugte echte PCK-SHA256: **TileMap `9372acb6f978775b5378424427bb3e766df47ed95c42f623f1a63890e843c1a5`**, **Font `c8925cfe52dba5b128dd12aba47cd9ae07623919ae15899610a8942a293d0cc7`**.
- Download-Artefakt `Godot-PS5-M10b-TILEMAP-FONT-Host-Validated-PS5-UNTESTED` (ID `12469481706`). Es enthält **zwei getrennte ZIPs** `GodotPS5-M10b-TILEMAP-PS5-TEST.zip` und `GodotPS5-M10b-FONT-UI-PS5-TEST.zip`. Jeweils nur die Inhalte von `GodotPS5/` nach `/data/GodotPS5/` kopieren. Jede ZIP schreibt `launch.txt` auf ihr eigenes PCK; alle bisherigen PCKs und das **bekannt funktionierende PPSA99847 v0.1.3** bleiben unverändert.
- **PS5-Abnahme separat:** erst TileMap-Test starten, Screenshot/Video sowie `gles3-stdout.log` und `gles3-runtime.log` mit `TILE_CONTACT_OK`, `TILE_TEST_PASS`, `GODOT_RETURN=0`, `EGL_CLOSED` erfassen. Danach die Font-ZIP kopieren und **visuell** prüfen, dass alle fünf Godot-Labels samt Umlauten sichtbar sind. `FONT_TTF_LOADED` und `LABEL_TREE_READY` allein beweisen noch keine gerenderten Zeichen. Optional **X** für Textfarbe.
- Die beiden Testprojekte sind unabhängig: Tile-Kollision kann PASS sein, während der Font-Renderer FAIL ist. Keine Behauptungen zu Audio, Savegames, 3D, TextServer-Full, vollständigen TileMap-Funktionen oder allgemeinen Godot-Spielen.

## Roadmap: Was als Nächstes zu tun ist

### P0 – M10b TileMapLayer / Label-Glyphen auf echter PS5 getrennt abnehmen

- [x] `PPSA99847 v0.1.2` auf realer PS5 starten: vom Nutzer bestätigt.
- [x] Pong-Bildschirm mit zwei Schlägern, bewegtem Ball, Spielfeld und Punktestand nachweisen: Video `20261022_234930_00248487(1).webm` bestätigt.
- [ ] Restliche Pong-Einzeltests bei Gelegenheit mit `/data/GodotPS5/gles3-stdout.log`, `gles3-runtime.log`, `gles3-stderr.log` erfassen: X-Aufschlag, Analog/D-Pad, Kollision/Punkte, Kreis-Reset, Options-Exit, Framezeiten und `GODOT_RETURN=0` / `EGL_CLOSED`. **Nicht mit dem jetzt vorliegenden kurzen Video gleichsetzen.**
- [x] M8c-GitHub Actions erfolgreich: Original-Godot-4.7.2-PCK erzeugt und auf Linux/headless vollständig durch zwei Szenen getestet. Run `39866584380`; PCK 4,828 Bytes, PCK format 4, engine 4.7, SHA-256 `137f4c5722e385c04c4f36a84fcd8548e5038a348f68d91ca2c3310c21527591` verified from downloaded Actions artifact.
- [x] **M8c physical PS5 test**: user reports the externally selected PCK works, both scenes switch correctly and X changes their colors, without rebuilding `PPSA99847 v0.1.2`.
- [ ] Supplement M8c with `PACK_SOURCE=CONFIG`, `GODOT_PS5_EXTERNAL_*` markers and `NORMAL_SHUTDOWN` console logs if further diagnostic evidence is needed; no logs uploaded for this run.
- [x] **M9a host acceptance:** standalone `CharacterBody2D` + `StaticBody2D` + `CollisionShape2D` real Godot 4.7.2 collision test built, executed, signed-off by CI run `39869677215`, and ready-to-copy external PCK produced.
- [x] **M9a PS5 hardware acceptance:** Die native v0.1.3 besteht den realen Kollisionstest zweimal mit `REAL_COLLISION_OK`, `TEST_PASS`, Exit-Code 0 und vollständigem EGL-Shutdown; mit v0.1.2 war der Dummy-Server das bestätigte Hindernis.
- [x] **M9b:** Original GodotPhysics2D-Modul in PS5-Engine integriert; separater Build 39872101079 grün; physische PS5-Kollision in zwei Sessions bestätigt.
- [x] **M10a host:** official Godot 4.7.2 PhysicsServer2D RigidBody2D + Area2D PCK complete, real collision/trigger markers and rest-state verified; CI 39876290256 green and install ZIP available.
- [x] **M10a PS5:** zwei erfolgreiche Hardware-Sessions mit realer Schwerkraft, Area-Eintritt/-Austritt, RigidBody-Bodenkontakt, stillstehendem Körper und sauberem Exit aus `bfpilot-download (22).tar`.
- [x] **M10b Host CI:** zwei unabhängige Godot-4.7.2-PCKs mit echten TileMapLayer-Kollisionen und FontFile/Label-Objekten erzeugt, ausgeführt und strukturell validiert. GitHub Actions 39878279649 vollständig grün.
- [ ] **M10b-a PS5:** cyanfarbenes Quadrat muss auf pinken echten TileMapLayer-Tiles stoppen; stdout `GODOT_PS5_M10B_TILE_CONTACT_OK` / `...TILE_TEST_PASS`; runtime Exit-Code 0.
- [ ] **M10b-b PS5:** fünf sichtbare Godot-Label-Zeilen mit echter Press Start 2P Schrift, Zahlen und Umlauten; stdout `FONT_TTF_LOADED`, `LABEL_TREE_READY`, `FONT_TEST_FINISHED`; zusätzlich **sichtbaren Text bestätigen**, Exit-Code 0.

### P1 – Echte eigene Godot-4-PCKs laden

- [ ] Ein kleines selbst erstelltes **Godot 4.7.2**-Projekt als `.pck` testen, ohne nativen Neu-Build.
- [ ] `/data/GodotPS5/games/MyGame.pck` mit `/data/GodotPS5/launch.txt` auswählen; alternativ ausdrücklich freigegebene USB-Pfade.
- [ ] Auswahl im Log `PACK_SOURCE=CONFIG` und Start einer **fremden** `main.tscn` auf PS5 bestätigen.
- [ ] Ungültige PCK, fehlende Assets, Szenenwechsel, Neustart und Rückfall aufs interne Test-PCK prüfen.
- [ ] Mehrere echte Open-Source-Demos sukzessive testen; **nicht** jede Godot-4-Datei als automatisch kompatibel bezeichnen.

### P2 – Fehlende Grundlagen zum echten Spielen

- [ ] **2D-Physik:** Godots echten `PhysicsServer2D` anbinden; `CharacterBody2D`, `CollisionShape2D`, Kollisionen und TileMaps auf PS5 nachweisen.
- [ ] **Text/Fonts:** FreeType und TextServer-Build reparieren; `Label`, Umlaute/Unicode, UI-Schrift und dynamische Fonts testen. (Im alten Scene Bridge v0.1.0 verursachte ein Label den schwarzen Bildschirm.)
- [ ] **Audio:** einen PS5-Audiotreiber mit PCM-Playback, niedriger Latenz und Godot-`AudioStreamPlayer` testen.
- [ ] **Eingaben:** Achsen/Trigger, mehrere Controller, Controller-Trennung/-Wiederverbindung, D-Pad und geringe Verzögerung getrennt bestätigen.
- [ ] **Dateisystem/Saves:** `user://`, Schreibrechte, Speichern/Laden nach Neustart, externes PCK mit Ressourcen und unterschiedliche USB-Mounts validieren.
- [ ] **Lebenszyklus:** mehrfacher Start/Exit ohne Hänger, sichere GL/EGL-Zerstörung, Logs, Fehlerdialoge und Absturzdiagnose.

### P3 – Kompatibilität, Leistung und Distribution

- [ ] Rendering-Features erweitern: Canvas2D-Shader, Viewports, UI-Controls, Animationen, Bildformate, Transparenz und Performance unter Last.
- [ ] **3D** getrennt evaluieren; mit einer einfachen `MeshInstance3D`-Szene beginnen. Kein pauschaler Vulkan- oder 3D-Support ohne Hardwaretest.
- [ ] Eingabe-Latenz, Framezeiten, GL-Shader-Cache, RAM/VRAM-Verbrauch und längere Sessions messen.
- [ ] Mehrere exportierte Open-Source-Godot-Spiele mit definierter Kompatibilitätsmatrix prüfen.
- [ ] **Lizenz/Release-Audit:** Original-Godot MIT; PS5-GPU-Glüe/DevBridge GPL-3.0-or-later; Herkunft und Lizenz des inoffiziellen OpenGL-SDK-Mirrors einschließlich vollständiger korrespondierender Quellen und Notices prüfen.
- [ ] Dokumentation, Screenshots, Installationsanleitung, eindeutige Releases und automatische TESTBUILD-Katalogpflege vervollständigen.

## Wann gilt der Godot-PS5-Port als „spielbar“?

**Nicht nur ein grüner GitHub Actions Build.** Für den ersten **spielbaren 2D-Port** müssen auf echter PS5 mindestens ein vollständiges Godot-Spiel (Pong zählt als erste interne Referenz), reale Controllersteuerung, Szene/Renderer, reproduzierbare Frameausgabe und sauberer Exit demonstriert sein. Für einen **breiter nutzbaren Godot-4-2D-Port** kommen externe PCKs, Physik, Fonts/UI, Audio und Savegames hinzu. **3D und Vulkan** sind danach eigenständige Meilensteine.

## Technische Ankerpunkte

- **Godot-Plattform:** [`engine/platform/ps5/`](engine/platform/ps5/), insbesondere `os_ps5.cpp`, `display_server_ps5.cpp`, `pad_ps5.cpp`, `detect.py`.
- **Grafik-Einstieg:** [`native/gles3_canvas_main.cpp`](native/gles3_canvas_main.cpp) – EGL/Swap-Wrapper; **keine eigene Sprite-Rasterisierung**.
- **Godot-Testprojekte:** [`samples/visual-smoke/`](samples/visual-smoke/) und [`samples/pong-smoke/`](samples/pong-smoke/).
- **GLES3-Build:** [`build-godot-ps5-gles3-canvas.yml`](../../.github/workflows/build-godot-ps5-gles3-canvas.yml) (im Repo-Root `.github/workflows/`).
- **Testapp-Dokumentation:** [`gles3-canvas/README.md`](gles3-canvas/README.md).
- **Ältere Detailanalysen:** [`GRAPHICS_BRINGUP.md`](GRAPHICS_BRINGUP.md), [`docs/DISPLAYSERVER_INTEGRATION.md`](docs/DISPLAYSERVER_INTEGRATION.md), [`docs/PS5_GPU_HARDWARE_ACCEPTANCE.md`](docs/PS5_GPU_HARDWARE_ACCEPTANCE.md).

## Dokumentation nach jedem neuen Konsolentest aktualisieren

1. Neue Version/Title-ID, Commit und Actions-Build eintragen.
2. Unterscheiden: **nur gebaut**, **auf PS5 sichtbar**, **vollständig spielbar**.
3. Screenshot/Video und exakte `/data/GodotPS5/`-Logmarker dokumentieren; keine Erfolgsbehauptung allein aus CI ableiten.
4. Neue Fehler mit reproduzierbaren Schritten und Fix-Commit festhalten.
5. Das jeweils **nächste offene P0-Ziel** hier ändern und alte Erfolgstests für Regression erhalten.

---

**Kurzfassung:** Echte Godot 4.7.2 GDScript-/Canvas2D-Pipeline, DualSense, Pong, externe PCKs, CharacterBody2D-Kollisionen und jetzt **RigidBody2D-Schwerkraft plus Area2D-Trigger** auf PS5 bestätigt. M10a: **2×** vollständig erfolgreich inklusive sauberem Exit (`bfpilot-download (22).tar`). **M10b TileMapLayer und FontFile/Label haben beide echte Desktop-Host-CI bestanden**, **PS5 noch nicht getestet**. Danach weitere unabhängige Abnahmen: Audio, Savegames und allgemeine Spiel-Kompatibilität.
