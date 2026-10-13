// PS5 SDL2 compatibility shim: disable only OPTIONAL remote-control and OSK
// initialization while retaining SDL2's real PS5 VideoOut backend.
// The ps5-payload-dev SDL port unconditionally opens a remote and keyboard
// during PS5_CreateDevice; unavailable service imports can null-call before
// VideoInit is reached. The ordinary DualSense SDL2 GameController path remains.
// These wrappers are activated only in the Rebased native app link.
extern "C" {
int __wrap_PS5_Remote_Init(void) { return 0; }
int __wrap_PS5_Keyboard_Init(void) { return 0; }
int __wrap_PS5_Keyboard_Open(void) { return 0; }
int __wrap_sceSystemServiceHideSplashScreen(void) { return 0; }
}
