/*
 * Stereoscopic 3D for the Nintendo 3DS top screen.
 *
 * The mine is rendered once per eye with the engine's own eye offset support
 * (render_frame(eye_offset), originally written for 1995 VR headsets), and the
 * whole frame (3D view, cockpit, HUD) is drawn into a second 8-bit buffer for the
 * right eye. The 3D view of each eye is then shifted horizontally so that the
 * convergence plane sits a little in front of the ship: walls and robots appear
 * behind the screen, while the HUD and cockpit stay at screen depth.
 *
 * The 3D slider scales the effect; with the slider down only one eye is rendered.
 */

#ifndef _STEREO3D_H
#define _STEREO3D_H

#include "maths.h"
#include "gr.h"

#ifdef __3DS__

extern fix Stereo_eye_offset;   // camera offset along the viewer's right vector for this pass
extern int Stereo_pixel_shift;  // horizontal shift applied to this pass's 3D view

void stereo3d_init(void);                   // after the video mode is set
void stereo3d_close(void);
int  stereo3d_begin_frame(void);            // 1 if this frame should be rendered per eye
void stereo3d_set_eye(int eye);             // -1 = left, +1 = right
void stereo3d_end_frame(int right_valid);   // restore the left canvas
void stereo3d_shift_view(grs_canvas *c);    // apply Stereo_pixel_shift to the 3D view
void stereo3d_flip(void);                   // hand the right eye to SDL; call before SDL_Flip

#else

#define Stereo_eye_offset 0
#define stereo3d_shift_view(c) ((void)0)

#endif

#endif
