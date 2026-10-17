#!/usr/bin/env python3
"""Repair WKAL v0.4.0 GAME presentation staging and PS5 selection music.

Run AFTER patch_game.py and patch_music.py, before make/build_release.sh.
Only affects the GAME title WKAG00001, never upstream WKAL00001.
"""
from pathlib import Path
import argparse


def replace_once(text: str, old: str, new: str, label: str) -> str:
    n = text.count(old)
    if n != 1:
        raise RuntimeError(f"{label}: expected one anchor, found {n}")
    return text.replace(old, new, 1)


def patch(root: Path) -> None:
    f = root / "src" / "app_installer.c"
    make = root / "Makefile"
    code = f.read_text()
    if 'INCASSET(snd0_at9, "assets/snd0.at9");' not in code:
        raise RuntimeError("Run patch_music.py before patch_presentation.py")
    if 'WKALI_GAME_PRESENTATION_FIX' in code:
        raise RuntimeError("Presentation patch has already been applied")

    code = replace_once(code,
        '#include <sys/stat.h>',
        '#include <sys/stat.h>\n#include <sqlite3.h>',
        "sqlite header")

    code = replace_once(code,
        'static int needs_update(',
        r'''/*
 * WKALI_GAME_PRESENTATION_FIX
 * The PS5 Game homescreen reads metadata from /user/appmeta/TITLE_ID.
 * Updating only /user/app/TITLE_ID/sce_sys leaves icons / selection audio
 * stale. Also, tbl_contentinfo.snd0info points to appmeta/snd0.at9.
 * Only update OUR title's sound path; never alter other database records.
 */
static int update_sound_index(const char *title_id) {
  const char *db_path = "/system_data/priv/mms/app.db";
  const char *sql =
      "UPDATE tbl_contentinfo "
      "SET snd0info = '/user/appmeta/' || ?1 || '/snd0.at9' "
      "WHERE titleId = ?1;";
  sqlite3 *db = NULL;
  sqlite3_stmt *stmt = NULL;
  int result = -1;
  int rc = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE, NULL);
  if (rc != SQLITE_OK) {
    wkali_log("[WKALI] MUSIC_DB_OPEN failed (rc=%d)\n", rc);
    if (db) sqlite3_close(db);
    return -1;
  }
  sqlite3_busy_timeout(db, 5000);
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    wkali_log("[WKALI] MUSIC_DB_PREPARE failed (%s)\n", sqlite3_errmsg(db));
    goto done;
  }
  rc = sqlite3_bind_text(stmt, 1, title_id, -1, SQLITE_TRANSIENT);
  if (rc != SQLITE_OK) {
    wkali_log("[WKALI] MUSIC_DB_BIND failed (rc=%d)\n", rc);
    goto done;
  }
  rc = sqlite3_step(stmt);
  if (rc == SQLITE_DONE) {
    result = sqlite3_changes(db);
    wkali_log("[WKALI] MUSIC_DB_REGISTER title=%s rows=%d\n",
              title_id, result);
    if (result == 0) {
      wkali_log("[WKALI] MUSIC_DB_WARNING no title record found\n");
    }
  } else {
    wkali_log("[WKALI] MUSIC_DB_WRITE failed (%s)\n", sqlite3_errmsg(db));
  }
done:
  if (stmt) sqlite3_finalize(stmt);
  sqlite3_close(db);
  return result;
}

static int needs_update(''',
        "sound db helper")

    code = replace_once(code,
        '  char sound_path[256];',
        '''  char sound_path[256];
  char root_icon_path[256];
  char appmeta_dir[256];
  char meta_param_path[256];
  char meta_icon_path[256];
  char meta_sound_path[256];''',
        "appmeta path buffers")

    code = replace_once(code,
        '  int update_needed = 0;',
        '''  snprintf(root_icon_path, sizeof(root_icon_path),
           "/user/app/%s/icon0.png", title_id);
  snprintf(appmeta_dir, sizeof(appmeta_dir),
           "/user/appmeta/%s", title_id);
  snprintf(meta_param_path, sizeof(meta_param_path),
           "/user/appmeta/%s/param.json", title_id);
  snprintf(meta_icon_path, sizeof(meta_icon_path),
           "/user/appmeta/%s/icon0.png", title_id);
  snprintf(meta_sound_path, sizeof(meta_sound_path),
           "/user/appmeta/%s/snd0.at9", title_id);

  int update_needed = 0;''',
        "appmeta full paths")

    code = replace_once(code,
        '    if (needs_update(sound_path, snd0_at9, snd0_at9_size))\n      update_needed = 1;',
        '''    if (needs_update(sound_path, snd0_at9, snd0_at9_size))
      update_needed = 1;
    if (needs_update(root_icon_path, icon0_png, icon0_png_size))
      update_needed = 1;
    if (needs_update(meta_param_path, param_json, param_json_size))
      update_needed = 1;
    if (needs_update(meta_icon_path, icon0_png, icon0_png_size))
      update_needed = 1;
    if (needs_update(meta_sound_path, snd0_at9, snd0_at9_size))
      update_needed = 1;''',
        "appmeta update detection")

    code = replace_once(code,
        '    return 0; /* Already installed and up to date */',
        r'''    wkali_log("[WKALI] GAME_ASSETS already current; checking sound index\n");
    (void)update_sound_index(title_id);
    return 0; /* Already installed and up to date */''',
        "recheck registration")

    code = replace_once(code,
        '  if ((err = install_app(title_id, "/user/app/"))) {',
        r'''  /* GAME homescreen can read the tile icon from the app root. */
  if (install_file(root_icon_path, icon0_png, icon0_png_size)) {
    wkali_log("[WKALI] ICON_ROOT_WRITE failed (%s)\n", root_icon_path);
    sceAppInstUtilTerminate();
    return -1;
  }

  if ((err = install_app(title_id, "/user/app/")) &&
      (uint32_t)err != 0x80990002u) {''',
        "install/restore support")

    code = replace_once(code,
        '  wkali_log("[WKALI] Launcher app installed successfully.\\n");',
        r'''  /* App installation may replace appmeta, so restage afterwards.
   * Do not delete/uninstall an existing title: users keep their old Game
   * launcher while this installer refreshes its presentation metadata. */
  if (mkdir_p(appmeta_dir, 0755) != 0 ||
      install_file(meta_param_path, param_json, param_json_size) != 0 ||
      install_file(meta_icon_path, icon0_png, icon0_png_size) != 0 ||
      install_file(meta_sound_path, snd0_at9, snd0_at9_size) != 0) {
    wkali_log("[WKALI] APPMETA_WRITE_FAILED path=%s errno=%d\\n",
              appmeta_dir, errno);
    sceAppInstUtilTerminate();
    return -1;
  }
  wkali_log("[WKALI] ICON_REGISTERED %s (%zu bytes)\\n",
            meta_icon_path, icon0_png_size);
  wkali_log("[WKALI] MUSIC_FILE_REGISTERED %s (%zu bytes)\\n",
            meta_sound_path, snd0_at9_size);
  int registered = update_sound_index(title_id);
  if (registered < 1) {
    wkali_log("[WKALI] MUSIC_INDEX_WARNING rc=%d; inspect PS5 app.db\\n",
              registered);
  }
  wkali_log("[WKALI] GAME_INSTALL_DONE title=%s\\n", title_id);
  wkali_log("[WKALI] Launcher app installed successfully.\\n");''',
        "appmeta staging")

    mk = make.read_text()
    mk = replace_once(mk, '-lSceAppInstUtil', '-lSceAppInstUtil -lsqlite3', "sqlite linker")
    f.write_text(code)
    make.write_text(mk)
    print("Game presentation fix: app root icon + appmeta icon/music + SQLite snd0info update")


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("upstream", type=Path)
    patch(p.parse_args().upstream)
