# Starbound Rebased v0.1.2 – SDL initialization crash investigation

User-confirmed v0.1.1 crash happens inside Core::init() after entering it, before finishing TextureManager::Init(). The update adds precise persisted TM01–TM12 checkpoints around SDL_Init, IMG_Init, TTF_Init, window creation, and renderer creation, and falls back to SDL software rendering if acceleration fails. Starts video separately from audio to reduce failure coupling. Early trace in `/data/starbound-rebased/boot.log` is essential for diagnosing the remaining PS5 runtime issue; **hardware boot is not yet confirmed**.
