# Godot 4.7.2 auf PS5 – Meilensteine & Roadmap

**Projekt:** PS5-Godot-Homebrew-Port von YAHBALLON  
**Stand:** 23. Oktober 2026 · PS5 Firmware **5.10** · Entwicklungsbranch `feature/godot-ps5-engine-foundation` · [PR #207](https://github.com/Yahballon/tmp_lap/pull/207)

> **Aktueller Meilenstein: M9b REAL GODOT PHYSICS 2D AUF PS5 BESTÄTIGT (23. Oktober 2026).** Der Nutzer meldet erfolgreiche Bewegung, Wandkollision und Grünfärbung auf `PPSA99847 v0.1.3`. **Zusätzlich unabhängiger Konsolen-Logbeweis** in `bfpilot-download (21).tar`: zwei erfolgreiche externe `physics-smoke.pck`-Sessions, jeweils `GODOT_PS5_M9_REAL_COLLISION_OK x=1093.984375 normal=(-1.0, 0.0)`, `GODOT_PS5_M9_TEST_PASS physics_frames=369`, `GODOT_RETURN=0`, `NORMAL_SHUTDOWN` und `EGL_CLOSED`. Keine Dummy-Physics-Warnung innerhalb der letzten beiden Sessions. Die historischen alten Dummy-Warnungen und Fehlercodes 43 im selben append-only-TAR gehören zu zwei früheren, fehlgeschlagenen Versuchen. **Nächster Meilenstein: M10 breitere 2D-Kompatibilität (UI/Fonts, zusätzliche Physics-Features, Audio und Saves getrennt nachweisen).**

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

## Roadmap: Was als Nächstes zu tun ist

### P0 – M9b real PhysicsServer2D bestätigt; nächster Test: breitere Godot-2D-Funktionen

- [x] `PPSA99847 v0.1.2` auf realer PS5 starten: vom Nutzer bestätigt.
- [x] Pong-Bildschirm mit zwei Schlägern, bewegtem Ball, Spielfeld und Punktestand nachweisen: Video `20261022_234930_00248487(1).webm` bestätigt.
- [ ] Restliche Pong-Einzeltests bei Gelegenheit mit `/data/GodotPS5/gles3-stdout.log`, `gles3-runtime.log`, `gles3-stderr.log` erfassen: X-Aufschlag, Analog/D-Pad, Kollision/Punkte, Kreis-Reset, Options-Exit, Framezeiten und `GODOT_RETURN=0` / `EGL_CLOSED`. **Nicht mit dem jetzt vorliegenden kurzen Video gleichsetzen.**
- [x] M8c-GitHub Actions erfolgreich: Original-Godot-4.7.2-PCK erzeugt und auf Linux/headless vollständig durch zwei Szenen getestet. Run `39866584380`; PCK 4,828 Bytes, PCK format 4, engine 4.7, SHA-256 `137f4c5722e385c04c4f36a84fcd8548e5038a348f68d91ca2c3310c21527591` verified from downloaded Actions artifact.
- [x] **M8c physical PS5 test**: user reports the externally selected PCK works, both scenes switch correctly and X changes their colors, without rebuilding `PPSA99847 v0.1.2`.
- [ ] Supplement M8c with `PACK_SOURCE=CONFIG`, `GODOT_PS5_EXTERNAL_*` markers and `NORMAL_SHUTDOWN` console logs if further diagnostic evidence is needed; no logs uploaded for this run.
- [x] **M9a host acceptance:** standalone `CharacterBody2D` + `StaticBody2D` + `CollisionShape2D` real Godot 4.7.2 collision test built, executed, signed-off by CI run `39869677215`, and ready-to-copy external PCK produced.
- [x] **M9a PS5 hardware acceptance:** Die native v0.1.3 besteht den realen Kollisionstest zweimal mit `REAL_COLLISION_OK`, `TEST_PASS`, Exit-Code 0 und vollständigem EGL-Shutdown; mit v0.1.2 war der Dummy-Server das bestätigte Hindernis.
- [x] **M9b:** Original GodotPhysics2D-Modul in PS5-Engine integriert; separater Build 39872101079 grün; physische PS5-Kollision in zwei Sessions bestätigt.

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

**Kurzfassung:** Original Godot 4.7.2 GDScript, GLES3/Canvas2D, DualSense, Pong, externe PCKs und jetzt **echte native PhysicsServer2D-Kollisionen** sind auf PS5 getestet. M9b ist durch zwei reale Konsolenläufe aus `bfpilot-download (21).tar` bestätigt. **Nächste unabhängige Abnahmen:** RigidBody2D/Area2D, Fonts/UI, Audio, Savegames und Kompatibilitätsmatrix.
