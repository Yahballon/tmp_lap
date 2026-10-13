# Starbound Rebased v0.1.4 – SDL2 video driver startup compatibility

Based on hardware boot logs stopping inside `SDL_InitSubSystem(SDL_INIT_VIDEO)`, this test build bypasses **optional** remote-control, on-screen-keyboard and splash-screen initializers in the PacBrew SDL2 PS5 video startup path. The actual PS5 VideoOut backend and SDL GameController remain enabled. These wrappers are linked only for this app. This is an experimental hypothesis about unavailable/invalid PS5 service imports; only the next hardware log can confirm it.
