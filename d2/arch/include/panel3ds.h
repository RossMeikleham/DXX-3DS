/* Nintendo 3DS bottom-screen touch panel. See arch/sdl/panel3ds.c. */
#ifndef _PANEL3DS_H
#define _PANEL3DS_H

#ifdef __3DS__
int panel3ds_frame(void);	// once per game frame; returns 1 if the game window was closed
void panel3ds_close(void);	// leaving the game
#endif

#endif
