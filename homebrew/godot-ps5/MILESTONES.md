# Godot 4.7.2 auf PS5 – Meilensteine & Roadmap

**Projekt:** PS5-Godot-Homebrew-Port von YAHBALLON  
**Stand:** 23. Oktober 2026 · PS5 Firmware **5.10** · Entwicklungsbranch `feature/godot-ps5-engine-foundation` · [PR #207](https://github.com/Yahballon/tmp_lap/pull/207)

> **Aktueller Meilenstein: M8c AUF PS5 BESTÄTIGT (Nutzerbericht, 23.10.2026).** `PPSA99847 v0.1.2` lädt ein unabhängiges, auf dem PC erstelltes Godot-4.7.2-PCK **ohne neues eboot.bin**. Der Nutzer hat nach dem M8c-Hardwaretest ausdrücklich bestätigt, dass beide Szenen und der X-Farbwechsel „alles wie es soll“ funktionieren. Host-CI [#39866584380](https://github.com/Yahballon/tmp_lap/actions/runs/39866584380) hatte zuvor PCK, Asset und Szenenwechsel validiert. **Kein neues Konsolenlog oder M8c-Video eingereicht**, daher Logmarker, FPS und Exit nicht unabhängig gemessen. **Nächster Meilenstein: M9 – native Godot 2D-Physik.**

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
| Echter GLES3-Test-Quellcode | [`gles3-canvas/README.md`](gles3-canvas/README.md), [`display_server_ps5.cpp`](engine/platform/ps5/display_server_ps5.cpp) |

Die vier geprüften GLES3-Sitzungen aus `bfpilot-download (18).tar` enthalten jeweils **1001 / 1145 / 1136 / 1106** Swap-Aufrufe, `GODOT_RETURN=0`, `NORMAL_SHUTDOWN` und `EGL_CLOSED`. Insgesamt wurden **223** `GODOT_PS5_REAL_2D_CROSS_PRESSED`-Meldungen protokolliert. Diese Werte gehören zur **bestätigten GLES3-Checkerboard-Version v0.1.0**, nicht zum neuen Pong.

## Aktuelle TESTBUILD-Titel

| Title-ID | App / Version | Zweck | Hardwarestand |
| --- | --- | --- | --- |
| `PPSA99842` | Godot PS5 Probe v0.1.0 | Frühes Native-Build-Grundgerüst | Diagnose/Legacy |
| `PPSA99843` | [Engine Smoke v0.3.2](https://github.com/Yahballon/tmp_lap/releases/tag/godot-ps5-engine-smoke-v0.3.2) | Original Godot VM, PCK, GDScript, DualSense | ✅ bestätigt |
| `PPSA99844` | [GPU Probe v0.1.7](https://github.com/Yahballon/tmp_lap/releases/tag/godot-ps5-gpu-probe-v0.1.7) | EGL/GL, Triangle, Textur, Alpha | ✅ bestätigt |
| `PPSA99845` | [Scene Bridge v0.1.2](https://github.com/Yahballon/tmp_lap/releases/tag/godot-ps5-scene-bridge-v0.1.2) | Godot-Szene → **eigener** OpenGL-Renderer, Performance-Messung | ✅ **v0.1.1** bestätigt; v0.1.2 Diagnoseänderung |
| **`PPSA99847`** | **[GLES3 Canvas v0.1.2](https://github.com/Yahballon/tmp_lap/releases/tag/godot-ps5-gles3-canvas-v0.1.2)** | **Echter Godot-Renderer mit neuem Pong-PCK** | ✅ Renderer v0.1.0 und Pong v0.1.2 auf PS5 sichtbar/laufend; 🟡 Detailabnahme |

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

## Roadmap: Was als Nächstes zu tun ist

### P0 – M8c bestätigt; M9 Godot-PhysicsServer2D prüfen (nächster Schritt)

- [x] `PPSA99847 v0.1.2` auf realer PS5 starten: vom Nutzer bestätigt.
- [x] Pong-Bildschirm mit zwei Schlägern, bewegtem Ball, Spielfeld und Punktestand nachweisen: Video `20261022_234930_00248487(1).webm` bestätigt.
- [ ] Restliche Pong-Einzeltests bei Gelegenheit mit `/data/GodotPS5/gles3-stdout.log`, `gles3-runtime.log`, `gles3-stderr.log` erfassen: X-Aufschlag, Analog/D-Pad, Kollision/Punkte, Kreis-Reset, Options-Exit, Framezeiten und `GODOT_RETURN=0` / `EGL_CLOSED`. **Nicht mit dem jetzt vorliegenden kurzen Video gleichsetzen.**
- [x] M8c-GitHub Actions erfolgreich: Original-Godot-4.7.2-PCK erzeugt und auf Linux/headless vollständig durch zwei Szenen getestet. Run `39866584380`; PCK 4,828 Bytes, PCK format 4, engine 4.7, SHA-256 `137f4c5722e385c04c4f36a84fcd8548e5038a348f68d91ca2c3310c21527591` verified from downloaded Actions artifact.
- [x] **M8c physical PS5 test**: user reports the externally selected PCK works, both scenes switch correctly and X changes their colors, without rebuilding `PPSA99847 v0.1.2`.
- [ ] Supplement M8c with `PACK_SOURCE=CONFIG`, `GODOT_PS5_EXTERNAL_*` markers and `NORMAL_SHUTDOWN` console logs if further diagnostic evidence is needed; no logs uploaded for this run.
- [ ] **M9a:** Design a separate `CharacterBody2D` + `StaticBody2D` + `CollisionShape2D` acceptance project and validate real `PhysicsServer2D` collisions on device; a host-only build or scripted Pong bounce is insufficient.
- [ ] **M9b:** If physics is unavailable in current `PPSA99847`, enable and link the original Godot 2D physics backend in the PS5 engine, build a separate version and repeat the exact scene with logs.

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

**Kurzfassung:** Echte Godot-4.7.2-Canvas2D-Pipeline, Pong v0.1.2 und **ein separat erstelltes externes Godot-PCK** sind nun auf echter PS5 demonstriert bzw. **durch Nutzerbericht bestätigt**. Zusätzliche Logs für Pong/M8c-Exit fehlen noch. **Als Nächstes: PhysicsServer2D / native Kollisionstest**, danach Fonts, Audio und Savegames.
