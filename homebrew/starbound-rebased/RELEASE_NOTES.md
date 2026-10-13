# Starbound Rebased v0.1.3 — SDL subsystem isolation

Hardware evidence confirms a null-address SIGSEGV inside `SDL_Init` (last checkpoint TM01). This update initializes SDL core, timer, events and video separately, persisting checkpoints `TM01a`–`TM01g` and `TM02` to `/data/starbound-rebased/boot.log`. This narrows which PS5 SDL2 subsystem triggers the crash and does **not** claim a hardware-verified fix.
