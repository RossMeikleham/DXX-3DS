/*
 * Stereoscopic 3D for the Nintendo 3DS top screen. See stereo3d.h.
 *
 * Tuning (optional) - put a "stereo3d.cfg" next to the game data with any of:
 *   Separation=0.25    half the distance between the eyes, in mine units, at full slider
 *   Convergence=14     distance (mine units) that appears at screen depth
 *   Enabled=1          0 turns stereo off even with the slider up
 */

#ifdef __3DS__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include <SDL/SDL.h>

#include "stereo3d.h"
#include "gr.h"
#include "u_mem.h"
#include "physfsx.h"
#include "strutil.h"
#include "window.h"
#include "screens.h"
#include "game.h"

/* From the stereo patch to SDL's 3DS video driver (3ds/sdl-n3ds-stereo.patch). Weak, so
   the game still links against an unpatched SDL and just stays 2D. */
extern int SDL_N3DS_SetStereo(int enable) __attribute__((weak));
extern void SDL_N3DS_SetRightEyeFrame(const Uint8 *pixels, int pitch) __attribute__((weak));

extern fix Render_zoom;
extern int Automap_active;
extern SDL_Surface *canvas;   // arch/sdl/gr.c: the 8-bit surface the screen canvas draws into
extern SDL_Surface *screen;   // arch/sdl/gr.c: the video surface (32bpp, or 16 with -16bpp)

fix Stereo_eye_offset = 0;
int Stereo_pixel_shift = 0;

static ubyte *right_buf = NULL;     // 8-bit right-eye frame, same layout as the screen canvas
static void *right_out = NULL;      // right_buf converted to the video surface's format
static int right_out_bpp = 0;
static int right_w = 0, right_h = 0;
static int sdl_stereo = 0;
static int right_ready = 0;
static int in_stereo_frame = 0;

static float cfg_separation = 0.25f;
static float cfg_convergence = 14.0f;
static int cfg_enabled = 1;
static int cfg_loaded = 0;

/* Prints to the bottom-screen console when the reason stereo is on/off changes,
   so a hardware test shows why the top screen is flat. */
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

	if (!SDL_N3DS_SetStereo || !SDL_N3DS_SetRightEyeFrame)
	{
		sdl_stereo = 0;
		return;
	}
	sdl_stereo = (SDL_N3DS_SetStereo(1) == 0);
	if (!sdl_stereo)
		printf("3D: video driver refused stereo (%s)\n", SDL_GetError());
	if (!sdl_stereo || !canvas || !screen)
		return;

	if (!right_buf || right_w != canvas->w || right_h != canvas->h ||
	    right_out_bpp != screen->format->BytesPerPixel)
	{
		stereo3d_close();
		right_w = canvas->w;
		right_h = canvas->h;
		right_out_bpp = screen->format->BytesPerPixel;
		if (right_out_bpp != 1 && right_out_bpp != 2 && right_out_bpp != 4)
			return;
		MALLOC(right_buf, ubyte, right_w * right_h);
		if (!right_buf)
			return;
		memset(right_buf, 0, right_w * right_h);
		if (right_out_bpp > 1)
		{
			MALLOC(right_out, ubyte, right_w * right_h * right_out_bpp);
			if (!right_out)
				stereo3d_close();
		}
	}
}

void stereo3d_close(void)
{
	if (right_buf)
		d_free(right_buf);
	if (right_out)
		d_free(right_out);
	right_buf = NULL;
	right_out = NULL;
	right_ready = 0;
}

// Point the screen canvas (and the 3D view inside it) at a different pixel buffer.
static void retarget(ubyte *base)
{
	ubyte *cur = grd_curscreen->sc_canvas.cv_bitmap.bm_data;
	ptrdiff_t off = Screen_3d_window.cv_bitmap.bm_data - cur;

	grd_curscreen->sc_canvas.cv_bitmap.bm_data = base;
	Screen_3d_window.cv_bitmap.bm_data = base + off;
}

int stereo3d_begin_frame(void)
{
	float slider, half_sep, focal;

	if (!sdl_stereo)
		return status(1, "off - video driver has no stereo"), 0;
	if (!right_buf)
		return status(2, "off - no right-eye buffer"), 0;
	if (!cfg_enabled)
		return status(3, "off - Enabled=0 in stereo3d.cfg"), 0;
	if (grd_curscreen->sc_canvas.cv_bitmap.bm_data != (ubyte *)canvas->pixels ||
	    canvas->w != right_w || canvas->h != right_h)
		return status(4, "off - screen canvas changed"), 0;
	// Only when nothing is drawn over the game: menus and the automap draw to the
	// left buffer only, so those frames are shown flat (left image to both eyes).
	if (window_get_front() != Game_wind)
		return status(5, "flat - a menu is open"), 0;
	if (Automap_active)
		return status(6, "flat - automap"), 0;

	slider = osGet3DSliderState();
	if (slider < 0.02f)
		return status(7, "flat - slider down"), 0;

	half_sep = cfg_separation * slider;
	// Horizontal focal length of the mine view in pixels (Render_zoom 0x9000 is ~60 deg).
	focal = (Screen_3d_window.cv_bitmap.bm_w * 0.5f) / f2fl(Render_zoom);

	Stereo_eye_offset = fl2f(half_sep);
	Stereo_pixel_shift = (int)(focal * half_sep / cfg_convergence + 0.5f);
	if (last_status != 0)
	{
		last_status = 0;
		printf("3D: on - eye %.2f units, shift %d px, view %dx%d\n",
		       half_sep, Stereo_pixel_shift, Screen_3d_window.cv_bitmap.bm_w, Screen_3d_window.cv_bitmap.bm_h);
	}
	in_stereo_frame = 1;
	return 1;
}

void stereo3d_set_eye(int eye)
{
	if (!in_stereo_frame)
		return;

	if (eye < 0)
	{
		retarget((ubyte *)canvas->pixels);
		Stereo_eye_offset = -abs(Stereo_eye_offset);
		Stereo_pixel_shift = -abs(Stereo_pixel_shift);
	}
	else
	{
		// Start from the finished left frame so regions that aren't redrawn every
		// frame (letterbox bars, untouched borders) match in both eyes.
		memcpy(right_buf, canvas->pixels, right_w * right_h);
		retarget(right_buf);
		Stereo_eye_offset = abs(Stereo_eye_offset);
		Stereo_pixel_shift = abs(Stereo_pixel_shift);
	}
}

void stereo3d_end_frame(int right_valid)
{
	if (!in_stereo_frame)
		return;
	retarget((ubyte *)canvas->pixels);
	Stereo_eye_offset = 0;
	Stereo_pixel_shift = 0;
	in_stereo_frame = 0;
	right_ready = right_valid;
}

/*
 * Parallel eye cameras put infinity at screen depth and everything else in front of
 * it. Shifting the left view left and the right view right moves the convergence plane
 * out to cfg_convergence, so the mine sits behind the screen. Columns uncovered by the
 * shift are filled with black.
 */
void stereo3d_shift_view(grs_canvas *c)
{
	int s = Stereo_pixel_shift;
	int w = c->cv_bitmap.bm_w, h = c->cv_bitmap.bm_h, rs = c->cv_bitmap.bm_rowsize;
	ubyte *row = c->cv_bitmap.bm_data;
	ubyte black;
	int y;

	if (!s || abs(s) >= w)
		return;

	black = BM_XRGB(0, 0, 0);
	for (y = 0; y < h; y++, row += rs)
	{
		if (s > 0)
		{
			memmove(row + s, row, w - s);
			memset(row, black, s);
		}
		else
		{
			memmove(row, row - s, w + s);
			memset(row + w + s, black, -s);
		}
	}
}

// Convert the 8-bit right eye to the video surface's format with the canvas palette,
// exactly as SDL_BlitSurface(canvas, screen) does for the left eye, and hand it over.
void stereo3d_flip(void)
{
	SDL_Palette *pal;
	Uint32 lut[256];
	int i, n = right_w * right_h;

	if (!right_ready || !right_buf || !screen)
	{
		right_ready = 0;
		return;
	}
	right_ready = 0;

	if (right_out_bpp == 1)
	{
		SDL_N3DS_SetRightEyeFrame(right_buf, right_w);
		return;
	}

	pal = canvas->format->palette;
	if (!pal || !right_out)
		return;
	for (i = 0; i < 256; i++)
	{
		SDL_Color c = (i < pal->ncolors) ? pal->colors[i] : (SDL_Color){0, 0, 0, 0};
		lut[i] = SDL_MapRGB(screen->format, c.r, c.g, c.b);
	}

	if (right_out_bpp == 4)
	{
		Uint32 *d = right_out;
		for (i = 0; i < n; i++)
			d[i] = lut[right_buf[i]];
	}
	else
	{
		Uint16 *d = right_out;
		for (i = 0; i < n; i++)
			d[i] = (Uint16)lut[right_buf[i]];
	}
	SDL_N3DS_SetRightEyeFrame(right_out, right_w * right_out_bpp);
}

#endif
