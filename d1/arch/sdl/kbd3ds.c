/* System software keyboard for menu text entry. See kbd3ds.h. Kept apart from the
   game code because <3ds.h>'s KEY_* names clash with key.h. */
#ifdef __3DS__
#include <3ds.h>
#include <ctype.h>
#include <string.h>
#include "kbd3ds.h"

int kbd3ds_input(char *text, int maxlen, const char *hint)
{
	SwkbdState swkbd;
	SwkbdButton button;
	char buf[256];
	int numeric = text[0] != '\0', i;

	if (maxlen <= 0)
		return 0;
	if (maxlen > (int)sizeof(buf) - 1)
		maxlen = sizeof(buf) - 1;
	for (i = 0; text[i]; i++)
		if (!isdigit((unsigned char)text[i]))
			numeric = 0;

	swkbdInit(&swkbd, numeric ? SWKBD_TYPE_NUMPAD : SWKBD_TYPE_WESTERN, 2, maxlen);
	if (numeric)
		swkbdSetNumpadKeys(&swkbd, '-', 0);	// minus for secret levels
	swkbdSetInitialText(&swkbd, text);
	if (hint && hint[0])
		swkbdSetHintText(&swkbd, hint);
	swkbdSetValidation(&swkbd, SWKBD_ANYTHING, 0, 0);

	buf[0] = '\0';
	button = swkbdInputText(&swkbd, buf, sizeof(buf));
	if (button != SWKBD_BUTTON_CONFIRM)
		return 0;

	strncpy(text, buf, maxlen);
	text[maxlen] = '\0';
	return 1;
}
#endif
