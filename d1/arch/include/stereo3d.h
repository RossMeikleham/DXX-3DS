/*
 * Stereoscopic 3D for the Nintendo 3DS top screen, with the OpenGL ES renderer
 * (NovaGL). See arch/ogl/stereo3d_gpu.c.
 *
 * The whole frame is rendered once per eye with the engine's own eye offset support
 * (render_frame(eye_offset), originally written for 1995 VR headsets): the left eye
 * into NovaGL's app surface, the right eye into its right-eye target. Each eye's 3D
 * view is shifted horizontally so that the convergence plane sits a little in front of
 * the ship: walls and robots appear behind the screen, while the HUD and cockpit stay
 * at screen depth. The 3D slider scales the effect; with the slider down only one eye
 * is rendered.
 *
 * With the software renderer these are no-ops and the game renders in 2D.
 */

#ifndef _STEREO3D_H
#define _STEREO3D_H

#include "maths.h"
#include "gr.h"

#if defined(__3DS__) && defined(OGL)

extern fix Stereo_eye_offset;   // camera offset along the viewer's right vector for this pass
extern int Stereo_pixel_shift;  // horizontal shift of this pass's 3D view (ogl_start_frame)

void stereo3d_init(void);
void stereo3d_close(void);
int  stereo3d_begin_frame(void);            // 1 if this frame should be rendered per eye
void stereo3d_set_eye(int eye);             // -1 = left, +1 = right
void stereo3d_end_frame(int right_valid);   // back to the left eye
void stereo3d_shift_view(grs_canvas *c);    // no-op: the shift is in the projection
void stereo3d_flip(void);                   // before the swap: 2D for frames not drawn per eye

#else

#define Stereo_eye_offset 0
#define stereo3d_shift_view(c) ((void)0)

#endif

#endif
