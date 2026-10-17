#!/usr/bin/env python3
"""Add PS5 4K Game Hub background artwork to WKAL00001's isolated GAME fork.

Must run after patch_game.py, patch_music.py and patch_presentation.py.
The PS5 browser exploit and source assets remain unchanged.
"""
from pathlib import Path
import argparse

EXPECTED_LENGTH = 148 + (3840 // 4) * (2160 // 4) * 16


def replace_once(src: str, original: str, replacement: str, label: str) -> str:
    n = src.count(original)
    if n != 1:
        raise RuntimeError(f"{label}: expected one upstream anchor, found {n}")
    return src.replace(original, replacement, 1)


def patch(root: Path) -> None:
    from prepare_background import verify

    bg = root / "assets/pic0.dds"
    if not bg.is_file():
        raise FileNotFoundError("Create assets/pic0.dds using the BC7 encoder before patching")
    verify(bg)
    path = root / "src/app_installer.c"
    code = path.read_text(encoding="utf-8")
    if "INCASSET(pic0_dds," in code:
        raise RuntimeError("Background already patched into installer")
    if "MUSIC_DB_REGISTER" not in code or "ICON_REGISTERED" not in code:
        raise RuntimeError("Run patch_music.py and patch_presentation.py first")

    code = replace_once(code,
        'INCASSET(snd0_at9, "assets/snd0.at9");',
        'INCASSET(snd0_at9, "assets/snd0.at9");\n'
        'INCASSET(pic0_dds, "assets/pic0.dds");',
        "embed BC7 background")

    # Query app.db by the GAME title ID only, resolving profile-specific
    # icon/concept tables via the SQLite schema. Never modify global artwork.
    helper = r'''
/* WKALI_GAME_BACKGROUND_FIX
 * The PS5 Home Screen reads its selection background from concept pic0Info.
 * Files alone can be ignored when a title was registered without artwork.
 * Update per-profile concept rows linked to WKAG00001, nothing else.
 */
static int register_game_background(const char *title_id) {
  const char *db_path = "/system_data/priv/mms/app.db";
  const char *image_uri = "/user/appmeta/WKAG00001/pic0.dds?v=2";
  sqlite3 *db = NULL;
  sqlite3_stmt *tables = NULL;
  int updated = 0;
  int rc = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE, NULL);
  if (rc != SQLITE_OK) {
    wkali_log("[WKALI] BACKGROUND_DB_OPEN_FAILED code=%d\n", rc);
    if (db) sqlite3_close(db);
    return -1;
  }
  sqlite3_busy_timeout(db, 5000);
  rc = sqlite3_prepare_v2(db,
      "SELECT name FROM sqlite_master WHERE type='table' AND name LIKE 'tbl_iconinfo_%'",
      -1, &tables, NULL);
  if (rc != SQLITE_OK) {
    wkali_log("[WKALI] BACKGROUND_DB_TABLES_FAILED code=%d\n", rc);
    sqlite3_close(db);
    return -1;
  }
  while ((rc = sqlite3_step(tables)) == SQLITE_ROW) {
    const char *icon_table = (const char *)sqlite3_column_text(tables, 0);
    const char *prefix = "tbl_iconinfo_";
    size_t prefix_len = strlen(prefix);
    if (!icon_table || strncmp(icon_table, prefix, prefix_len) != 0)
      continue;
    const char *suffix = icon_table + prefix_len;
    if (!suffix[0]) continue;
    int safe = 1;
    for (const char *s = suffix; *s; s++) {
      if (!((*s >= '0' && *s <= '9') ||
            (*s >= 'a' && *s <= 'z') ||
            (*s >= 'A' && *s <= 'Z') || *s == '_')) safe = 0;
    }
    if (!safe || strlen(suffix) > 64) continue;
    char concept_table[112];
    snprintf(concept_table, sizeof(concept_table),
             "tbl_concepticoninfo_%s", suffix);
    char sql[512];
    snprintf(sql, sizeof(sql),
             "UPDATE \"%s\" SET pic0Info=?1 "
             "WHERE localConceptId IN "
             "(SELECT localConceptId FROM \"%s\" WHERE titleId=?2)",
             concept_table, icon_table);
    sqlite3_stmt *st = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) == SQLITE_OK) {
      sqlite3_bind_text(st, 1, image_uri, -1, SQLITE_STATIC);
      sqlite3_bind_text(st, 2, title_id, -1, SQLITE_TRANSIENT);
      int step_rc = sqlite3_step(st);
      if (step_rc == SQLITE_DONE) {
        updated += sqlite3_changes(db);
      } else {
        wkali_log("[WKALI] BACKGROUND_DB_WRITE_FAILED code=%d\n", step_rc);
      }
    }
    if (st) sqlite3_finalize(st);
  }
  if (rc != SQLITE_DONE) {
    wkali_log("[WKALI] BACKGROUND_DB_SCAN_WARNING code=%d\n", rc);
  }
  sqlite3_finalize(tables);
  sqlite3_close(db);
  wkali_log("[WKALI] BACKGROUND_DB_REGISTER title=%s rows=%d\n",
            title_id, updated);
  return updated;
}

'''
    code = replace_once(code, 'static int needs_update(', helper + 'static int needs_update(', "database registration helper")

    code = replace_once(code,
        '  char meta_sound_path[256];',
        '  char meta_sound_path[256];\n'
        '  char game_bg0_path[256];\n'
        '  char game_bg1_path[256];\n'
        '  char meta_bg0_path[256];\n'
        '  char meta_bg1_path[256];',
        "background path buffers")

    code = replace_once(code,
        '  int update_needed = 0;',
        '''  snprintf(game_bg0_path, sizeof(game_bg0_path),
           "/user/app/%s/sce_sys/pic0.dds", title_id);
  snprintf(game_bg1_path, sizeof(game_bg1_path),
           "/user/app/%s/sce_sys/pic1.dds", title_id);
  snprintf(meta_bg0_path, sizeof(meta_bg0_path),
           "/user/appmeta/%s/pic0.dds", title_id);
  snprintf(meta_bg1_path, sizeof(meta_bg1_path),
           "/user/appmeta/%s/pic1.dds", title_id);

  int update_needed = 0;''',
        "background file paths")

    code = replace_once(code,
        '    if (needs_update(meta_sound_path, snd0_at9, snd0_at9_size))\n'
        '      update_needed = 1;',
        '''    if (needs_update(meta_sound_path, snd0_at9, snd0_at9_size))
      update_needed = 1;
    if (needs_update(game_bg0_path, pic0_dds, pic0_dds_size))
      update_needed = 1;
    if (needs_update(game_bg1_path, pic0_dds, pic0_dds_size))
      update_needed = 1;
    if (needs_update(meta_bg0_path, pic0_dds, pic0_dds_size))
      update_needed = 1;
    if (needs_update(meta_bg1_path, pic0_dds, pic0_dds_size))
      update_needed = 1;''',
        "background asset update checks")

    code = replace_once(code,
        '    (void)update_sound_index(title_id);',
        '    (void)update_sound_index(title_id);\n'
        '    (void)register_game_background(title_id);',
        "refresh old title's background registration")

    code = replace_once(code,
        '  /* GAME homescreen can read the tile icon from the app root. */',
        r'''  if (install_file(game_bg0_path, pic0_dds, pic0_dds_size) != 0 ||
      install_file(game_bg1_path, pic0_dds, pic0_dds_size) != 0) {
    wkali_log("[WKALI] BACKGROUND_GAME_FILES_FAILED errno=%d\n", errno);
    sceAppInstUtilTerminate();
    return -1;
  }
  /* GAME homescreen can read the tile icon from the app root. */''',
        "write title's sce_sys background")

    code = replace_once(code,
        '      install_file(meta_sound_path, snd0_at9, snd0_at9_size) != 0)',
        '      install_file(meta_sound_path, snd0_at9, snd0_at9_size) != 0 ||\n'
        '      install_file(meta_bg0_path, pic0_dds, pic0_dds_size) != 0 ||\n'
        '      install_file(meta_bg1_path, pic0_dds, pic0_dds_size) != 0)',
        "write appmeta background after installation")

    code = replace_once(code,
        '  int registered = update_sound_index(title_id);',
        '''  wkali_log("[WKALI] BACKGROUND_FILES_REGISTERED %s and %s (%zu bytes each)\\n",
            meta_bg0_path, meta_bg1_path, pic0_dds_size);
  int bg_registered = register_game_background(title_id);
  if (bg_registered < 1) {
    wkali_log("[WKALI] BACKGROUND_INDEX_WARNING rc=%d; inspect app.db\\n",
              bg_registered);
  }
  int registered = update_sound_index(title_id);''',
        "register artwork and report result")

    path.write_text(code, encoding="utf-8")
    print("PS5 Game Hub background embedded, staged and registered (BC7, 4K)")

if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("upstream", type=Path)
    patch(p.parse_args().upstream.resolve())
