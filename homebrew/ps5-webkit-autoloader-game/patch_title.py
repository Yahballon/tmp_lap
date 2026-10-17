#!/usr/bin/env python3
"""Rename the isolated PS5 GAME tile to 'JAILBREAK ME' on fresh and existing installs.

This patch only touches WKAG00001, never WKAL00001 or the exploit itself.
Run after patch_game.py, patch_music.py, patch_presentation.py, and patch_background.py.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path


TITLE = "JAILBREAK ME"
TITLE_ID = "WKAG00001"


def replace_once(code: str, old: str, new: str, purpose: str) -> str:
    found = code.count(old)
    if found != 1:
        raise ValueError(f"{purpose}: expected 1 insertion point, found {found}")
    return code.replace(old, new, 1)


def patch(root: Path) -> None:
    param_file = root / "assets" / "param.json.template"
    app_file = root / "src" / "app_installer.c"
    param = json.loads(param_file.read_text("utf-8"))
    if param.get("titleId") != TITLE_ID or param.get("applicationCategoryType") != 0:
        raise ValueError("Refusing to rename a title other than WKAG00001 GAME")
    params = param.get("localizedParameters", {})
    locales = [v for v in params.values() if isinstance(v, dict)]
    if not locales or any(item.get("titleName") != TITLE for item in locales):
        raise ValueError("patch_game.py must set every localized title to JAILBREAK ME")
    # Version bump from previous Game+icon+music+background build to
    # encourage Game Hub presentation cache refresh.
    param["contentVersion"] = "01.000.003"

    code = app_file.read_text("utf-8")
    if "BACKGROUND_DB_REGISTER" not in code or "MUSIC_DB_REGISTER" not in code:
        raise ValueError("Run patch_background.py and presentation patch first")
    if "WKAG_RENAME_TITLE" in code:
        raise ValueError("Game tile title already patched")

    helper = r'''
/*
 * WKAG_RENAME_TITLE: update ONLY WKAG00001 presentation database rows.
 * The system may retain previously-installed title names even after
 * sce_sys/param.json has been rewritten, so set the existing rows too.
 */
static int wkag_rename_query(sqlite3 *db, const char *sql,
                             const char *name, const char *title_id) {
  sqlite3_stmt *stmt = NULL;
  int count = 0;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    wkali_log("[WKALI] TITLE_DB_PREPARE_WARN code=%d\n", rc);
    return 0;
  }
  sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, title_id, -1, SQLITE_TRANSIENT);
  rc = sqlite3_step(stmt);
  if (rc == SQLITE_DONE) count = sqlite3_changes(db);
  else wkali_log("[WKALI] TITLE_DB_WRITE_WARN code=%d\n", rc);
  sqlite3_finalize(stmt);
  return count;
}

static void wkag_rename_installed_title(const char *title_id) {
  /* Keep the function safe if a future upstream fork changes the title ID. */
  if (strcmp(title_id, "WKAG00001") != 0) return;
  const char *new_title = "JAILBREAK ME";
  sqlite3 *db = NULL;
  int rc = sqlite3_open_v2("/system_data/priv/mms/app.db",
                           &db, SQLITE_OPEN_READWRITE, NULL);
  if (rc != SQLITE_OK) {
    wkali_log("[WKALI] TITLE_DB_OPEN_WARN code=%d\n", rc);
    if (db) sqlite3_close(db);
    return;
  }
  sqlite3_busy_timeout(db, 5000);
  int updated = wkag_rename_query(
      db, "UPDATE tbl_contentinfo SET titleName=?1 WHERE titleId=?2",
      new_title, title_id);

  sqlite3_stmt *tables = NULL;
  rc = sqlite3_prepare_v2(
      db, "SELECT name FROM sqlite_master WHERE type='table' "
          "AND name LIKE 'tbl_iconinfo_%'",
      -1, &tables, NULL);
  if (rc == SQLITE_OK) {
    while (sqlite3_step(tables) == SQLITE_ROW) {
      const char *table = (const char *)sqlite3_column_text(tables, 0);
      const char *prefix = "tbl_iconinfo_";
      if (!table || strncmp(table, prefix, strlen(prefix)) != 0) continue;
      const char *suffix = table + strlen(prefix);
      size_t n = strlen(suffix);
      if (!n || n > 64) continue;
      int safe = 1;
      for (size_t i = 0; i < n; ++i) {
        const char ch = suffix[i];
        if (!(ch >= 'a' && ch <= 'z') &&
            !(ch >= 'A' && ch <= 'Z') &&
            !(ch >= '0' && ch <= '9') && ch != '_') safe = 0;
      }
      if (!safe) continue;

      char query[512];
      snprintf(query, sizeof(query),
               "UPDATE \"%s\" SET titleName=?1 WHERE titleId=?2", table);
      updated += wkag_rename_query(db, query, new_title, title_id);

      char concept_table[112];
      snprintf(concept_table, sizeof(concept_table),
               "tbl_concepticoninfo_%s", suffix);
      snprintf(query, sizeof(query),
               "UPDATE \"%s\" SET conceptName=?1 WHERE localConceptId "
               "IN (SELECT localConceptId FROM \"%s\" WHERE titleId=?2)",
               concept_table, table);
      updated += wkag_rename_query(db, query, new_title, title_id);
    }
  } else {
    wkali_log("[WKALI] TITLE_DB_TABLES_WARN code=%d\n", rc);
  }
  if (tables) sqlite3_finalize(tables);
  sqlite3_close(db);
  wkali_log("[WKALI] TITLE_DB_REGISTER title=%s text=JAILBREAK ME rows=%d\n",
            title_id, updated);
}

'''

    code = replace_once(code, "static int needs_update(", helper + "static int needs_update(", "renaming helper")
    code = replace_once(code,
        "    (void)register_game_background(title_id);",
        "    (void)register_game_background(title_id);\n"
        "    wkag_rename_installed_title(title_id);",
        "rename already up-to-date installed tile")
    code = replace_once(code,
        '  wkali_log("[WKALI] GAME_INSTALL_DONE',
        '  wkag_rename_installed_title(title_id);\n'
        '  wkali_log("[WKALI] GAME_INSTALL_DONE',
        "rename after installation")

    app_file.write_text(code, encoding="utf-8")
    param_file.write_text(
        json.dumps(param, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    print("Game title set to JAILBREAK ME (all locales + existing PS5 app database rows)")


if __name__ == "__main__":
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("upstream", type=Path)
    patch(cli.parse_args().upstream)
