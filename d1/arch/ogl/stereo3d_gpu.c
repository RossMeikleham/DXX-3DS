/*
 * GPU spike: stereoscopic 3D through NovaGL. Same interface (stereo3d.h), tuning
 * (stereo3d.cfg) and depth model as the software version in arch/sdl/stereo3d.c:
 *
 * - each eye renders the whole frame with render_frame(+/-eye offset); the left eye
 *   draws into NovaGL's app surface, the right eye into its right-eye LCD target
 *   (novaBeginEye)
 * - the convergence shift is a clip-space translation on the 3D view's projection
 *   (ogl_start_frame), so the mine sits behind the screen while the HUD and cockpit,
 *   drawn with the 2D projection, stay at screen depth
 *
 * Tuning (optional) - "stereo3d.cfg" next to the game data:
 *   Separation=0.25    half the distance between the eyes, in mine units, at full slider
 *   Convergence=14     distance (mine units) that appears at screen depth
 *   Enabled=1          0 turns stereo off even with the slider up
 */
#if defined(__3DS__) && defined(OGL)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include "stereo3d.h"
#include "gr.h"
#include "physfsx.h"
#include "strutil.h"
#include "window.h"
#include "screens.h"
#include "game.h"
#include "nova3ds.h"

extern fix Render_zoom;
extern int Automap_active;

fix Stereo_eye_offset = 0;
int Stereo_pixel_shift = 0;	// read by ogl_start_frame(): + moves this eye's 3D view right

static int in_stereo_frame = 0;
static int eyes_drawn = 0;	// this frame was drawn per eye (set until the next flip)
static int half_shift = 0;

static float cfg_separation = 0.25f;
static float cfg_convergence = 14.0f;
static int cfg_enabled = 1;
static int cfg_loaded = 0;

static int last_status = -1;
static void status(int code, const char *why)
{
	if (code == last_status)
		return;
	last_status = code;
	printf("3D: %s\n", why);
}

static void load_config(void)
{
	PHYSFS_file *fp;
	char line[80];

	cfg_loaded = 1;
	if (!(fp = PHYSFSX_openReadBuffered("stereo3d.cfg")))
		return;
	while (PHYSFSX_fgets(line, sizeof(line), fp))
	{
		char *eq = strchr(line, '=');
		float v;

		if (!eq)
			continue;
		*eq = '\0';
		v = (float)atof(eq + 1);
		if (!d_stricmp(line, "Separation") && v >= 0.0f && v <= 4.0f)
			cfg_separation = v;
		else if (!d_stricmp(line, "Convergence") && v >= 1.0f && v <= 1000.0f)
			cfg_convergence = v;
		else if (!d_stricmp(line, "Enabled"))
			cfg_enabled = (v != 0.0f);
	}
	PHYSFS_close(fp);
}

void stereo3d_init(void)
{
	if (!cfg_loaded)
		load_config();
}

void stereo3d_close(void)
{
	nova3ds_set_stereo(0);
}

static int flat(int code, const char *why)
{
	status(code, why);
	nova3ds_set_stereo(0);	// the right framebuffer would keep a stale eye
	return 0;
}

int stereo3d_begin_frame(void)
{
	float slider, half_sep, focal;

	if (!cfg_loaded)
		load_config();
	if (!cfg_enabled)
		return flat(3, "off - Enabled=0 in stereo3d.cfg");
	if (window_get_front() != Game_wind)
		return flat(5, "flat - a menu is open");
	if (Automap_active)
		return flat(6, "flat - automap");
	slider = osGet3DSliderState();
	if (slider < 0.02f)
		return flat(7, "flat - slider down");

	half_sep = cfg_separation * slider;
	// Horizontal focal length of the mine view in pixels (Render_zoom 0x9000 is ~60 deg).
	focal = (Screen_3d_window.cv_bitmap.bm_w * 0.5f) / f2fl(Render_zoom);
	Stereo_eye_offset = fl2f(half_sep);
	half_shift = (int)(focal * half_sep / cfg_convergence + 0.5f);

	if (last_status != 0)
	{
		last_status = 0;
		printf("3D: on (GPU) - eye %.2f units, shift %d px, view %dx%d\n",
		       half_sep, half_shift, Screen_3d_window.cv_bitmap.bm_w, Screen_3d_window.cv_bitmap.bm_h);
	}
	nova3ds_set_stereo(1);
	in_stereo_frame = 1;
	return 1;
}

void stereo3d_set_eye(int eye)
{
	if (!in_stereo_frame)
		return;
	Stereo_eye_offset = eye < 0 ? -abs(Stereo_eye_offset) : abs(Stereo_eye_offset);
	Stereo_pixel_shift = eye < 0 ? -half_shift : half_shift;
	nova3ds_begin_eye(eye < 0 ? 0 : 1);
}

void stereo3d_end_frame(int right_valid)
{
	if (!in_stereo_frame)
		return;
	if (right_valid)
		eyes_drawn = 1;
	else
		nova3ds_set_stereo(0);	// the right eye wasn't drawn this frame
	nova3ds_begin_eye(0);	// back to the left eye / app surface for the rest of the frame
	Stereo_eye_offset = 0;
	Stereo_pixel_shift = 0;
	in_stereo_frame = 0;
}

void stereo3d_shift_view(grs_canvas *c) { (void)c; }	// done in the projection
// Called by gr_flip() before the swap. Any frame not drawn per eye (loading screen,
// briefings, menus drawn outside game_render_frame) is shown in 2D, so the right
// framebuffer's stale or never-written contents can't show through.
void stereo3d_flip(void)
{
	if (!eyes_drawn)
		nova3ds_set_stereo(0);
	eyes_drawn = 0;
}

#endif
