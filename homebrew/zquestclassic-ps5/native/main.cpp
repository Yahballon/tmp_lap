// SPDX-License-Identifier: GPL-3.0-or-later
// ZQuest Classic PS5 - native quest-library prototype, NOT the upstream game engine.
#include "quest_index.hpp"
#include "gfx/renderer.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace {
using namespace hui::gfx;
using zcps5::QuestIndex;

constexpr const char* kDataDir = "/data/ZQuestClassic";
constexpr const char* kLogDir = "/data/ZQuestClassic/logs";
const Color kWhite = Color::rgb(0xf3f8f4);
const Color kDim = Color::rgb(0x9bb5b3);
const Color kMint = Color::rgb(0x85f3ac);
const Color kGold = Color::rgb(0xffcc76);
Color color(uint32_t c, float a = 1.f) { return Color::rgb(c, a); }
std::string read_file(const char* path) {
  FILE* f = fopen(path, "rb");
  if (!f) return {};
  if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return {}; }
  const long n = ftell(f);
  if (n <= 0 || n > 16000000) { fclose(f); return {}; }
  rewind(f);
  std::string data(static_cast<size_t>(n), '\0');
  if (fread(data.data(), 1, data.size(), f) != data.size()) data.clear();
  fclose(f);
  return data;
}
void logging(const char* text) {
  mkdir(kDataDir, 0755);
  mkdir(kLogDir, 0755);
  if (FILE* f = fopen("/data/ZQuestClassic/logs/port.log", "ab")) {
    fprintf(f, "[ZQuest PS5 0.1.1] %s\n", text);
    fclose(f);
  }
}
void txt(DrawList& d, const Font& f, unsigned atlas, const std::string& str,
         float x, float y, float size, Color c = kWhite, Align a = Align::left) {
  d.text(f, atlas, str, x, y, size, c, a);
}
void box(DrawList& d, float x, float y, float w, float h, bool active = false) {
  if (active) d.glow({x, y, w, h}, 12, 12, color(0x57ec9a, .22f));
  d.bordered_rect({x, y, w, h}, 14, color(active ? 0x103731 : 0x102126, .96f),
                  active ? 3 : 2, color(active ? 0x5bf0ae : 0x245052));
}
std::string ellipsis(const std::string& s, size_t max) {
  return s.size() <= max ? s : s.substr(0, max > 3 ? max - 3 : 0) + "...";
}
std::string human_size(uint64_t n) {
  char buf[64];
  if (n >= 1048576) snprintf(buf, sizeof(buf), "%.1f MiB", double(n) / 1048576.);
  else if (n >= 1024) snprintf(buf, sizeof(buf), "%.1f KiB", double(n) / 1024.);
  else snprintf(buf, sizeof(buf), "%llu B", static_cast<unsigned long long>(n));
  return buf;
}
void scenery(DrawList& d) {
  // Original, abstract maze motif. No Zelda/Nintendo artwork is distributed.
  d.gradient_rect({0, 0, 1920, 1080}, 0, color(0x061315), color(0x0a2225));
  for (int x = 0; x < 1920; x += 96) d.line(float(x), 0, float(x), 1080, 1, color(0x183a38, .22f));
  for (int y = 0; y < 1080; y += 96) d.line(0, float(y), 1920, float(y), 1, color(0x183a38, .22f));
  for (int i = 0; i < 12; ++i) {
    const float x = 1320.f + (i % 4) * 124.f;
    const float y = 370.f + (i / 4) * 122.f;
    d.bordered_rect({x, y, 102, 96}, 12, color(i == 5 ? 0x1c6549 : 0x153638),
                    2, color(i == 5 ? 0xb4fc86 : 0x2b5851, .7f));
    if (i == 5) {
      d.circle(x + 50, y + 48, 23, color(0x99f7c5));
      d.circle(x + 50, y + 48, 10, color(0x194e41));
    } else if (i % 3 == 0) {
      d.rounded_rect({x + 30, y + 24, 42, 48}, 7, color(0x326a58));
    }
  }
}
void render_screen(DrawList& d, const Font& f, unsigned atlas,
                   const QuestIndex& scan, size_t selected,
                   bool info, bool status, bool exiting) {
  scenery(d);
  txt(d, f, atlas, "ZQUEST CLASSIC", 88, 116, 69, kWhite);
  txt(d, f, atlas, "PS5 PORT FOUNDATION  /  by YAHBALLON", 91, 161, 22, kMint);
  txt(d, f, atlas, "v0.1.1", 1832, 112, 26, kDim, Align::right);
  d.line(88, 191, 1832, 191, 2, color(0x2c6760));
  box(d, 78, 234, 1080, 692);
  txt(d, f, atlas, "QUEST LIBRARY", 110, 307, 32, kWhite);
  const std::string found = std::to_string(scan.quests().size()) +
      (scan.done() ? " files found" : " files found  /  scanning...");
  txt(d, f, atlas, found, 1115, 306, 20, kMint, Align::right);
  d.line(112, 330, 1121, 330, 2, color(0x264a4c));

  const auto& items = scan.quests();
  if (items.empty()) {
    const std::string message = !scan.done() ? "Looking for quests..." :
        scan.opened() == 0 ? "Quest folders inaccessible" : "No .qst files found";
    txt(d, f, atlas, message, 123, 432, 36,
        scan.opened() == 0 && scan.done() ? kGold : kWhite);
    if (scan.done() && scan.opened() == 0) {
      txt(d, f, atlas, "OPTIONS shows scan errors; view port.log for details",
          124, 699, 19, kGold);
    }
    txt(d, f, atlas, "Copy legally obtained .qst quests to:", 124, 497, 23, kDim);
    txt(d, f, atlas, "/data/ZQuestClassic/quests", 124, 541, 25, kMint);
    txt(d, f, atlas, "or USB: /mnt/usb0/ZQuestClassic/quests", 124, 581, 23, kMint);
    txt(d, f, atlas, "SQUARE rescans after copying files.", 124, 657, 21, kDim);
  } else {
    const size_t first = selected > 3 ? selected - 3 : 0;
    for (size_t i = first; i < items.size() && i < first + 8; ++i) {
      float y = 351.f + float(i - first) * 68.f;
      bool active = i == selected;
      box(d, 112, y, 1005, 59, active);
      txt(d, f, atlas, active ? ">" : "+", 138, y + 39, 27, active ? kGold : kMint);
      txt(d, f, atlas, ellipsis(items[i].name, 37), 188, y + 40, 25, kWhite);
      txt(d, f, atlas, items[i].size_known ? human_size(items[i].bytes) : "size unknown", 1089, y + 40, 20, kDim,
          Align::right);
    }
  }
  box(d, 1192, 234, 650, 692);
  txt(d, f, atlas, "ADVENTURE AWAITS", 1230, 317, 30, kMint);
  if (status) {
    txt(d, f, atlas, "PORTING STATUS", 1230, 798, 29, kWhite);
    txt(d, f, atlas, "Kernel getdents quest scanner v0.1.1", 1230, 759, 19, kMint);
    txt(d, f, atlas, "Folders opened: " + std::to_string(scan.opened()), 1230, 801, 18);
    txt(d, f, atlas, "Errors: " + std::to_string(scan.failed()), 1230, 836, 18,
        scan.failed() ? kGold : kMint);
    if (scan.failed())
      txt(d, f, atlas, ellipsis(scan.first_error(), 49), 1230, 869, 16, kGold);
    txt(d, f, atlas, "Gameplay engine: pending", 1230, 898, 19, kGold);
  } else if (info && !items.empty()) {
    const auto& q = items[std::min(selected, items.size() - 1)];
    txt(d, f, atlas, "SELECTED QUEST", 1230, 783, 23, kMint);
    txt(d, f, atlas, ellipsis(q.name, 29), 1230, 823, 27, kWhite);
    txt(d, f, atlas, q.size_known ? human_size(q.bytes) : "File size unavailable", 1230, 861, 20, kDim);
    txt(d, f, atlas, "Preview only; engine integration pending", 1230, 895, 18, kGold);
  } else {
    txt(d, f, atlas, "NATIVE PS5 FRONTEND", 1230, 786, 25, kWhite);
    txt(d, f, atlas, "Folders opened: " + std::to_string(scan.opened()), 1230, 834, 21, kMint);
    txt(d, f, atlas, "Gameplay is NOT implemented yet.", 1230, 878, 21, kGold);
  }
  txt(d, f, atlas, "D-PAD  Select    X  Details    SQUARE  Rescan    OPTIONS  Port status    O  Exit",
      960, 1007, 21, kWhite, Align::center);
  if (exiting) {
    d.rounded_rect({0, 0, 1920, 1080}, 0, color(0x010607, .66f));
    box(d, 576, 330, 760, 346, true);
    txt(d, f, atlas, "EXIT ZQUEST CLASSIC?", 956, 425, 38, kWhite, Align::center);
    txt(d, f, atlas, "X  Confirm exit        O  Cancel", 956, 535, 25,
        kMint, Align::center);
  }
}
}  // namespace

int main() {
  logging("BOOT native quest library");
  hui::ps5::Display display;
  if (!display.open()) hui::sys::park();
  Renderer renderer;
  if (!renderer.init()) hui::sys::park();
  std::string font_bytes = read_file("/app0/assets/fonts/inter-semibold.huifont");
  if (font_bytes.empty())
    font_bytes = read_file("/data/homebrew/PPSA99127/assets/fonts/inter-semibold.huifont");
  Font font;
  if (!font.load(font_bytes)) {
    logging("FONT unavailable");
    hui::sys::park();
  }
  unsigned atlas = renderer.batch().create_font_texture(font);
  hui::ps5::Pad pad;
  pad.open();

  QuestIndex scan;
  scan.set_log(logging);
  const std::vector<std::string> roots = {
      "/data/ZQuestClassic/quests", "/data/homebrew/PPSA99127/quests",
      "/mnt/usb0/ZQuestClassic/quests", "/mnt/usb1/ZQuestClassic/quests",
      "/mnt/usb0/quests", "/mnt/usb1/quests"};
  scan.begin(roots);
  size_t selected = 0;
  bool show_info = false, show_status = false, exiting = false;
  uint32_t previous_buttons = 0;
  uint64_t next_repeat = 0;
  for (;;) {
    scan.step(16);
    std::array<hui::PadSample, 64> samples{};
    const size_t count = pad.read(samples);
    uint32_t pressed = 0, held = 0;
    bool connected = false;
    for (size_t i = 0; i < count; ++i) {
      if (!samples[i].connected) continue;
      connected = true;
      uint32_t buttons = samples[i].buttons;
      if (samples[i].left_y < 65) buttons |= hui::pad_bits::kUp;
      if (samples[i].left_y > 190) buttons |= hui::pad_bits::kDown;
      pressed |= buttons & ~previous_buttons;
      previous_buttons = buttons;
      held = buttons;
    }
    if (!connected) { held = 0; previous_buttons = 0; }
    const uint64_t now = static_cast<uint64_t>(hui::sys::monotonic_us());
    const uint32_t dirs = held & (hui::pad_bits::kUp | hui::pad_bits::kDown);
    uint32_t nav = pressed & dirs;
    if (nav) next_repeat = now + 250000;
    else if (dirs && now >= next_repeat) {
      nav = dirs;
      next_repeat = now + 140000;
    }
    if (!dirs) next_repeat = 0;
    if (exiting) {
      if (pressed & hui::pad_bits::kCircle) exiting = false;
      if (pressed & hui::pad_bits::kCross) {
        logging("EXIT");
        pad.close();
        renderer.release();
        display.close();
        hui::sys::quit();
      }
    } else {
      if ((nav & hui::pad_bits::kUp) && selected > 0) --selected;
      if ((nav & hui::pad_bits::kDown) && selected + 1 < scan.quests().size()) ++selected;
      if (pressed & hui::pad_bits::kCross) {
        show_info = !show_info;
        show_status = false;
      }
      if (pressed & hui::pad_bits::kSquare) {
        logging("RESCAN quests");
        scan.begin(roots);
        selected = 0;
        show_info = false;
      }
      if (pressed & hui::pad_bits::kOptions) {
        show_status = !show_status;
        show_info = false;
      }
      if (pressed & hui::pad_bits::kCircle) {
        if (show_info || show_status) { show_info = false; show_status = false; }
        else exiting = true;
      }
    }
    DrawList ui;
    render_screen(ui, font, atlas, scan, selected, show_info, show_status, exiting);
    renderer.begin();
    renderer.draw(ui);
    renderer.present(0, display.width(), display.height());
    display.swap();
    hui::sys::hide_splash_screen();
    hui::sys::sleep_us(16000);
  }
}
