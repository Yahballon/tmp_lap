/*
 * Open Surge PS5: native launch arguments for the original engine.
 * Copyright (C) 2026 YAHBALLON - GPL-3.0-or-later.
 *
 * Not a stand-alone game, launcher, renderer or Allegro replacement.
 */
#ifndef OPENSURGE_PS5_LAUNCHER_H
#define OPENSURGE_PS5_LAUNCHER_H

#if defined(OPENSURGE_PS5)
#include <stdio.h>
#include <stdlib.h>

#define OPENSURGE_PS5_TITLE_ID "PPSA99358"
#define OPENSURGE_PS5_ASSETS "/data/homebrew/" OPENSURGE_PS5_TITLE_ID "/game"
#define OPENSURGE_PS5_USERDATA "/data/OpenSurge"

static inline void opensurge_ps5_prepare_arguments(int *argc, char ***argv)
{
    static char arg0[] = "opensurge";
    static char arg1[] = "--game-folder";
    static char arg2[] = OPENSURGE_PS5_ASSETS;
    static char arg3[] = "--fullscreen";
    static char arg4[] = "--hide-fps";
    static char *arguments[] = {arg0, arg1, arg2, arg3, arg4, NULL};

    /* Keep save data out of the read-only ShadowMount application folder. */
    if (setenv("OPENSURGE_USER_PATH", OPENSURGE_PS5_USERDATA, 0) != 0)
        fprintf(stderr, "[OpenSurgePS5] Can't configure user-data path\n");

    *argc = 5;
    *argv = arguments;
}
#endif
#endif
