/*
 * Nintendo 3DS bottom-screen panel: a touch panel drawn on the bottom-screen text
 * console (40x30 characters of 8x8 pixels) while a level is being played.
 *
 *   row  0      shield, energy, lives, keys
 *   rows 1-11   primary weapons   (tap one to select it)
 *   rows 12-22  secondary weapons (tap one to select it)
 *   rows 23-26  buttons: Map, Rear view, Save/Load (D1) or Headlight/Energy->Shield (D2), Menu
 *   rows 27-29  log: everything else printed to the console scrolls here
 *
 * The same file is used by D1 and D2; D2-only parts check PLAYER_FLAGS_HEADLIGHT.
 */

#ifdef __3DS__

#include <stdio.h>
#include <string.h>
#include <3ds.h>

#include "panel3ds.h"
#include "game.h"
#include "player.h"
#include "weapon.h"
#include "laser.h"
#include "text.h"
#include "kconfig.h"
#include "window.h"
#include "gameseq.h"
#include "newdemo.h"
#include "automap.h"

extern int game_menu_3ds(void);
extern void game_save_3ds(void);
extern void game_load_3ds(void);

#define COLS		40
#define PANEL_ROWS	27
#define LOG_ROW		27
#define LOG_ROWS	3
#define PRIM_ROW	2	// first weapon row; two text rows per weapon
#define SEC_ROW		13
#define BTN_ROW		23
#define BTN_ROWS	4
#define N_BTN		5
#define BTN_W		(COLS / N_BTN)

#define ESC_RESET	"\x1b[0m"
#define ESC_REV		"\x1b[7m"
#define ESC_BRIGHT	"\x1b[37;1m"
#define ESC_DIM		"\x1b[37;2m"
#define ESC_LOW		"\x1b[33m"
#define ESC_HEAD	"\x1b[36;1m"

enum { BTN_MAP, BTN_REAR, BTN_3, BTN_4, BTN_MENU };

typedef struct {
	int shields, energy, lives, flags, laser_level, primary, secondary, rear;
	int has[2][10];
	int ammo[2][10];
} panel_state;

static PrintConsole panel_con;
static PrintConsole *log_con = NULL;
static void *log_fb = NULL;
static panel_state last;
static int drawn = 0;
static int touch_held = 0;
static int converter_held = 0;

static int n_weapons(int secondary)
{
	return secondary ? MAX_SECONDARY_WEAPONS : MAX_PRIMARY_WEAPONS;
}

static const char *weapon_name(int secondary, int w)
{
	return secondary ? SECONDARY_WEAPON_NAMES_SHORT(w) : PRIMARY_WEAPON_NAMES_SHORT(w);
}

static void read_state(panel_state *s)
{
	player *p = &Players[Player_num];
	int sec, w;

	memset(s, 0, sizeof(*s));
	s->shields = f2ir(p->shields);
	s->energy = f2ir(p->energy);
	s->lives = p->lives;
	s->flags = p->flags;
	s->laser_level = p->laser_level;
	s->primary = Primary_weapon;
	s->secondary = Secondary_weapon;
	s->rear = Rear_view;
	for (sec = 0; sec < 2; sec++)
		for (w = 0; w < n_weapons(sec); w++)
		{
			s->has[sec][w] = player_has_weapon(w, sec);
			s->ammo[sec][w] = sec ? p->secondary_ammo[w]
			                      : (w == VULCAN_INDEX
#ifdef GAUSS_INDEX
			                         || w == GAUSS_INDEX
#endif
			                        ) ? f2i((unsigned int)p->primary_ammo[VULCAN_INDEX] * VULCAN_AMMO_SCALE) : 0;
		}
}

/* Make sure the log console is confined to the bottom rows. SDL creates the console
   with each video mode set, so redo it (and redraw everything) when that happens. */
static int setup_consoles(void)
{
	PrintConsole *cur = consoleSelect(NULL);

	consoleSelect(cur);
	if (!cur || cur == &panel_con)
		return 0;
	if (cur != log_con || cur->frameBuffer != log_fb || cur->windowY != LOG_ROW)
	{
		log_con = cur;
		log_fb = cur->frameBuffer;
		consoleSetWindow(log_con, 0, LOG_ROW, COLS, LOG_ROWS);
		drawn = 0;
	}
	panel_con = *log_con;
	consoleSetWindow(&panel_con, 0, 0, COLS, PANEL_ROWS);
	return 1;
}

static void at(int row, int col)
{
	printf("\x1b[%d;%dH", row + 1, col + 1);
}

// Print exactly `width` characters at (row, col).
static void field(int row, int col, int width, const char *style, const char *text)
{
	char buf[COLS + 1];

	snprintf(buf, sizeof(buf), "%-*.*s", width, width, text);
	at(row, col);
	printf("%s%s" ESC_RESET, style, buf);
}

static void draw_weapon(const panel_state *s, int sec, int w)
{
	int per_col = 5, width = n_weapons(sec) > per_col ? COLS / 2 : COLS;
	int row = (sec ? SEC_ROW : PRIM_ROW) + 2 * (w % per_col);
	int col = (w / per_col) * width;
	int has = s->has[sec][w], selected = (sec ? s->secondary : s->primary) == w;
	char text[COLS + 1], info[16] = "";
	char style[24];

	if (!sec && w == LASER_INDEX)
		snprintf(info, sizeof(info), "L%d%s", (s->laser_level > MAX_LASER_LEVEL ? MAX_LASER_LEVEL : s->laser_level) + 1,
		         (s->flags & PLAYER_FLAGS_QUAD_LASERS) ? " Q" : "");
#ifdef SUPER_LASER_INDEX
	else if (!sec && w == SUPER_LASER_INDEX && s->laser_level > MAX_LASER_LEVEL)
		snprintf(info, sizeof(info), "L%d%s", s->laser_level + 1, (s->flags & PLAYER_FLAGS_QUAD_LASERS) ? " Q" : "");
#endif
	else if ((has & HAS_WEAPON_FLAG) && s->ammo[sec][w])
		snprintf(info, sizeof(info), "%d", s->ammo[sec][w]);

	snprintf(text, sizeof(text), " %d %-10.10s %s", (w + 1) % 10, weapon_name(sec, w), info);
	snprintf(style, sizeof(style), "%s%s", selected ? ESC_REV : "",
	         !(has & HAS_WEAPON_FLAG) ? ESC_DIM : (has == HAS_ALL ? ESC_BRIGHT : ESC_LOW));
	field(row, col, width - 1, style, text);
	field(row + 1, col, width - 1, "", "");
}

static const char *button_label(const panel_state *s, int b)
{
	switch (b)
	{
		case BTN_MAP:  return "MAP";
		case BTN_REAR: return s->rear ? "FRONT" : "REAR";
#ifdef PLAYER_FLAGS_HEADLIGHT
		case BTN_3:    return (s->flags & PLAYER_FLAGS_HEADLIGHT_ON) ? "LIGHT*" : "LIGHT";
		case BTN_4:    return "E>SHLD";
#else
		case BTN_3:    return "SAVE";
		case BTN_4:    return "LOAD";
#endif
		default:       return "MENU";
	}
}

static int button_enabled(const panel_state *s, int b)
{
#ifdef PLAYER_FLAGS_HEADLIGHT
	if (b == BTN_3)
		return (s->flags & PLAYER_FLAGS_HEADLIGHT) != 0;
	if (b == BTN_4)
		return (s->flags & PLAYER_FLAGS_CONVERTER) != 0;
#endif
	return 1;
}

static void draw_buttons(const panel_state *s)
{
	static const char *bg[N_BTN] = { "\x1b[44m", "\x1b[42m", "\x1b[45m", "\x1b[46m", "\x1b[41m" };
	int b, r;

	for (b = 0; b < N_BTN; b++)
	{
		char style[24], label[BTN_W + 1];
		const char *l = button_label(s, b);
		int pad = (BTN_W - 1 - (int)strlen(l)) / 2;

		snprintf(style, sizeof(style), "%s%s", bg[b], button_enabled(s, b) ? ESC_BRIGHT : "\x1b[30m");
		snprintf(label, sizeof(label), "%*s%s", pad, "", l);
		for (r = 0; r < BTN_ROWS; r++)
			// last panel row: one column short, so the cursor never wraps into a scroll
			field(BTN_ROW + r, b * BTN_W, BTN_W - 1 - (r == BTN_ROWS - 1 && b == N_BTN - 1),
			      style, r == BTN_ROWS / 2 - 1 ? label : "");
	}
}

static void draw(const panel_state *s)
{
	char text[COLS + 1];
	int sec, w;

	snprintf(text, sizeof(text), " SHIELD %3d  ENERGY %3d  LIVES %d", s->shields, s->energy, s->lives);
	field(0, 0, COLS - 9, ESC_REV ESC_BRIGHT, text);
	field(0, COLS - 9, 3, (s->flags & PLAYER_FLAGS_BLUE_KEY) ? "\x1b[44;37;1m" : ESC_REV, " B ");
	field(0, COLS - 6, 3, (s->flags & PLAYER_FLAGS_GOLD_KEY) ? "\x1b[43;30m" : ESC_REV, " Y ");
	field(0, COLS - 3, 3, (s->flags & PLAYER_FLAGS_RED_KEY) ? "\x1b[41;37;1m" : ESC_REV, " R ");

	field(PRIM_ROW - 1, 0, COLS, ESC_HEAD, " PRIMARY            (D-pad left/right)");
	field(SEC_ROW - 1, 0, COLS, ESC_HEAD, " SECONDARY          (D-pad up/down)");
	for (sec = 0; sec < 2; sec++)
		for (w = 0; w < n_weapons(sec); w++)
			draw_weapon(s, sec, w);
	draw_buttons(s);
}

// Returns the weapon under (row, col) as sec * 16 + w, or -1.
static int weapon_at(int row, int col)
{
	int sec;

	for (sec = 0; sec < 2; sec++)
	{
		int top = sec ? SEC_ROW : PRIM_ROW, n = n_weapons(sec);
		int width = n > 5 ? COLS / 2 : COLS;
		int w;

		if (row < top || row >= top + 10)
			continue;
		w = (row - top) / 2 + 5 * (col / width);
		if (w < n)
			return sec * 16 + w;
	}
	return -1;
}

// Returns 1 if the game window was closed.
static int tap(int row, int col, panel_state *s)
{
	int hit;

	if ((hit = weapon_at(row, col)) >= 0)
	{
		int sec = hit / 16, w = hit % 16;

		if (!Player_is_dead && player_has_weapon(w, sec) == HAS_ALL)
			select_weapon(w, sec, 1, 1);
		return 0;
	}
	if (row < BTN_ROW || row >= BTN_ROW + BTN_ROWS)
		return 0;

	switch (col / BTN_W)
	{
		case BTN_MAP:
			Controls.automap_count++;
			break;
		case BTN_REAR:
			Controls.rear_view_count++;
			break;
#ifdef PLAYER_FLAGS_HEADLIGHT
		case BTN_3:
			Controls.headlight_count++;
			break;
		case BTN_4:
			break;	// held: see panel3ds_frame()
#else
		case BTN_3:
			game_save_3ds();
			drawn = 0;
			break;
		case BTN_4:
			game_load_3ds();
			drawn = 0;
			break;
#endif
		case BTN_MENU:
			drawn = 0;
			return game_menu_3ds();
	}
	return 0;
}

int panel3ds_frame(void)
{
	panel_state s;
	int held, closed = 0;
	touchPosition t;

	if (window_get_front() != Game_wind || Newdemo_state == ND_STATE_PLAYBACK)
		return 0;
	if (!setup_consoles())
		return 0;

	read_state(&s);

	// Touch: react on touch-down. SDL scans the HID several times a frame, so
	// hidKeysDown() can miss it; track the held state instead.
	held = (hidKeysHeld() & KEY_TOUCH) != 0;
	if (held)
	{
		hidTouchRead(&t);
#ifdef PLAYER_FLAGS_HEADLIGHT
		{
			int on_converter = t.py / 8 >= BTN_ROW && t.py / 8 < BTN_ROW + BTN_ROWS && t.px / 8 / BTN_W == BTN_4;
			if (on_converter != converter_held)
			{
				converter_held = on_converter;
				Controls.energy_to_shield_state = on_converter;
			}
		}
#endif
		if (!touch_held)
			closed = tap(t.py / 8, t.px / 8, &s);
	}
#ifdef PLAYER_FLAGS_HEADLIGHT
	else if (converter_held)
	{
		converter_held = 0;
		Controls.energy_to_shield_state = 0;
	}
#endif
	touch_held = held;
	if (closed)
		return 1;

	if (!drawn || memcmp(&s, &last, sizeof(s)))
	{
		if (!setup_consoles())
			return 0;
		consoleSelect(&panel_con);
		draw(&s);
		consoleSelect(log_con);
		last = s;
		drawn = 1;
	}
	return 0;
}

// Leaving the game: give the whole screen back to the console.
void panel3ds_close(void)
{
	PrintConsole *cur = consoleSelect(NULL);

	consoleSelect(cur);
	if (cur && cur == log_con && cur->frameBuffer == log_fb)
	{
		consoleSetWindow(cur, 0, 0, COLS, 30);
		printf("\x1b[2J");
	}
	log_con = NULL;
	log_fb = NULL;
	drawn = 0;
	touch_held = 0;
#ifdef PLAYER_FLAGS_HEADLIGHT
	if (converter_held)
		Controls.energy_to_shield_state = 0;
#endif
	converter_held = 0;
}

#endif
