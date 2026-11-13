/* 3DS OpenGL ES context through NovaGL. See nova3ds.c. */
#ifndef _NOVA3DS_H
#define _NOVA3DS_H
void nova3ds_init(void);
void nova3ds_swap(void);
void nova3ds_close(void);
void nova3ds_set_stereo(int on);	// top screen 3D mode on/off
void nova3ds_begin_eye(int eye);	// 0 = left (app surface), 1 = right-eye target
#endif
