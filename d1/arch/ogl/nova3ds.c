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
 * only reaches the left eye's surface, and NovaGL turns a glClear in the middle of a
 * frame into a quad drawn through the current GL state, which on hardware left the
 * right eye's letterbox bars and sky (exit sequence) showing stale memory: bright
 * green. Draw the quad explicitly: whole screen, identity matrices, no depth, blend,
 * alpha test, texture or culling.
 */
static void clear_right_eye(void)
{
	static const GLfloat quad[] = { -1, -1, 0,  1, -1, 0,  1, 1, 0,  -1, -1, 0,  1, 1, 0,  -1, 1, 0 };
	static const GLenum caps[] = { GL_DEPTH_TEST, GL_BLEND, GL_ALPHA_TEST, GL_TEXTURE_2D, GL_CULL_FACE, GL_SCISSOR_TEST, GL_FOG };
	GLboolean was[sizeof(caps) / sizeof(caps[0])];
	GLboolean va = glIsEnabled(GL_VERTEX_ARRAY), ca = glIsEnabled(GL_COLOR_ARRAY), ta = glIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLint vp[4];
	unsigned i;

	glGetIntegerv(GL_VIEWPORT, vp);
	for (i = 0; i < sizeof(caps) / sizeof(caps[0]); i++)
	{
		was[i] = glIsEnabled(caps[i]);
		if (was[i])
			glDisable(caps[i]);
	}
	glViewport(0, 0, 400, 240);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glColor4f(0, 0, 0, 1);
	glVertexPointer(3, GL_FLOAT, 0, quad);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glColor4f(1, 1, 1, 1);
	if (!va) glDisableClientState(GL_VERTEX_ARRAY);
	if (ca) glEnableClientState(GL_COLOR_ARRAY);
	if (ta) glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glViewport(vp[0], vp[1], vp[2], vp[3]);
	for (i = 0; i < sizeof(caps) / sizeof(caps[0]); i++)
		if (was[i])
			glEnable(caps[i]);
	glClear(GL_DEPTH_BUFFER_BIT);	// the game clears depth per 3D view too; this covers the HUD pass
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
