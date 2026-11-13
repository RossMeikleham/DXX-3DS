// Holds the main init and de-init functions for arch-related program parts

#include <SDL/SDL.h>
#ifdef __3DS__
#include <3ds.h>
#include <sys/iosupport.h>
extern int n3ds_system_quit;
#endif
#include "songs.h"
#include "key.h"
#include "digi.h"
#include "mouse.h"
#include "joy.h"
#include "gr.h"
#include "dxxerror.h"
#include "text.h"
#include "args.h"
#include "config.h"

void arch_close(void)
{
	songs_uninit();

	gr_close();

	if (!GameArg.CtlNoJoystick)
		joy_close();

	mouse_close();

#ifdef __3DS__
	// After HOME > Close the DSP is paused, so SDL's audio thread never gets a buffer
	// back and closing the audio would wait for it forever. Just release the DSP.
	if (n3ds_system_quit)
		ndspExit();
	else
#endif
	if (!GameArg.SndNoSound)
	{
		digi_close();
	}

	key_close();

#ifdef __3DS__
	// SDL_Quit() releases the framebuffers the console draws into. Send stdout and
	// stderr to the null device first so printf()s from later atexit handlers
	// (con_close, mem leak reports) can't write into freed memory.
	consoleDebugInit(debugDevice_NULL);
	devoptab_list[STD_OUT] = devoptab_list[STD_ERR];
#endif

#ifdef __3DS__
	// After HOME > Close, SDL_Quit() joins SDL's video thread, which is stuck in
	// C3D_FrameBegin() waiting for the GPU. The system frees it all at exit.
	if (!n3ds_system_quit)
#endif
	SDL_Quit();
}

void arch_init(void)
{
	int t;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		Error("SDL library initialisation failed: %s.",SDL_GetError());

	key_init();

	digi_select_system( GameArg.SndDisableSdlMixer ? SDLAUDIO_SYSTEM : SDLMIXER_SYSTEM );

	if (!GameArg.SndNoSound)
		digi_init();

	mouse_init();

	if (!GameArg.CtlNoJoystick)
		joy_init();

	if ((t = gr_init(0)) != 0)
		Error(TXT_CANT_INIT_GFX,t);

	atexit(arch_close);
}

