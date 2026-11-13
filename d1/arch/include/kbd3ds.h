/*
 * Text entry on the Nintendo 3DS through the system software keyboard, which
 * opens on the bottom screen. Used by menu input fields (newmenu.c), which
 * otherwise need a keyboard.
 */
#ifndef _KBD3DS_H
#define _KBD3DS_H

#ifdef __3DS__
// Edit text (up to maxlen characters; the buffer holds maxlen + 1) with the
// keyboard, starting from its current contents. A number pad is shown when the
// text is a number. hint is shown greyed out while the field is empty (may be
// NULL). Returns 1 if the player confirmed, 0 if they cancelled.
int kbd3ds_input(char *text, int maxlen, const char *hint);
#endif

#endif
