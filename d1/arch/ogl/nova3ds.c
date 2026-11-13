/*
 * Nintendo 3DS: the OpenGL ES 1.1 context, through NovaGL (GLES 1.1 -> citro3d).
 * Kept apart from the game headers: <3ds.h>'s KEY_* names clash with key.h's.
 */
#if defined(__3DS__) && defined(OGL)

#include <3ds.h>
#include <citro3d.h>
#include <NovaGL.h>
#include <GLES/gl.h>

#include "nova3ds.h"

extern int n3ds_system_quit;

/*
 * SDL_Init(SDL_INIT_VIDEO) has run gfxInitDefault() and C3D_Init() with SDL's small
 * command buffer; SDL_SetVideoMode() is never called, so SDL draws nothing. Hand
 * citro3d to NovaGL with its own buffer sizes and put the text console back on the
 * bottom screen. SDL stays in use for events, joystick and audio.
 */
void nova3ds_init(void)
{
	C3D_Fini();
	nova_init();
	// nova_init() turns the top screen's 3D mode on, but the right framebuffer holds the
	// Homebrew Launcher's last frame until an eye is drawn there (speckled noise through
	// the parallax barrier). 3D mode is switched on per frame by nova3ds_set_stereo().
	gfxSet3D(false);
	// NovaGL's own stereo shifts the whole projection, HUD included, with the slider
	// (default eye depth 0.05), and stops batching draws while the slider is up. The
	// game does its own per-eye cameras and shift (stereo3d_gpu.c), so it's off.
	novaSet3DDepth(0.0f);
	// NovaGL waits for a vblank after each frame (swap interval 1) and citro3d's
	// C3D_FrameBegin(SYNCDRAW) waits for another: two refreshes a frame, a 30 FPS cap.
	// citro3d's wait alone paces the game to 60.
	novaSetSwapInterval(0);
	consoleInit(GFX_BOTTOM, NULL);
}

/*
 * GPU stereo (arch/ogl/stereo3d_gpu.c). nova3ds_init() leaves the top screen in 2D;
 * 3D mode is on only while frames are drawn per eye, so a flat frame never shows a
 * stale right eye.
 */
void nova3ds_set_stereo(int on)
{
	static int cur = 0;

	on = on != 0;
	if (on != cur)
	{
		gfxSet3D(on);
		cur = on;
	}
}

/*
 * Clear the right-eye target to black. The game's own clear (glClear after each swap)
 * only reaches the left eye's surface. A glClear in the middle of a frame becomes a
 * quad drawn through the current GL state, and on hardware neither that nor an
 * explicit black quad cleared the right eye: the exit sequence's letterbox bars and
 * sky showed stale memory (bright green). Asking for the stencil too makes NovaGL use
 * the hardware memory fill (C3D_RenderTargetClear after a frame split), the same
 * clear the left eye gets at the start of each frame.
 */
static void clear_right_eye(void)
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void nova3ds_begin_eye(int eye)
{
	novaBeginEye(eye);
	if (eye == 1)
		clear_right_eye();
}

void nova3ds_swap(void)
{
	novaSwapBuffers();
}

void nova3ds_close(void)
{
	// After HOME > Close the app has no GPU; nova_fini() would wait for it forever.
	// The system frees everything at exit.
	if (!n3ds_system_quit)
		nova_fini();
}

// Not in NovaGL; DXX only uses it for the automap's points. PICA200 has no wide points.
void glPointSize(GLfloat size) { (void)size; }

#endif
