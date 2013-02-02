/*
===========================================================================
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of Spearmint Source Code.

Spearmint Source Code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Spearmint Source Code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Spearmint Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, Spearmint Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following
the terms and conditions of the GNU General Public License.  If not, please
request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional
terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/
//
/*
=======================================================================

CREDITS

=======================================================================
*/


#include "ui_local.h"


#ifdef TURTLEARENA
typedef struct {
	menuframework_s	menu;
	int				startTime;
	qboolean		quitAfter;
} creditsmenu_t;

static creditsmenu_t	s_credits;

typedef struct
{
	const char *header;
	const char *lines[20];
} creditSection_t;

creditSection_t credits[] = {
	{"Turtle Arena", { NULL } },
	{"Designed by", { "Zack Middleton", NULL } },
	{"Art by", { "Zack Middleton", NULL } },
	{"Fonts by", { "M+ Fonts Project", NULL } },
	{"Sound Effects by", { "Zack Middleton", NULL } },
	{"Music by", {
		"Neil Crowe",
		"zero-project",
		NULL
	} },
	{"Programming by", {
		"Zack Middleton",
		"",
		"::id Software::",
		"John Carmack",
		"Robert A. Duffy",
		"Jim Dose'",
		"",
		"::ioquake3 contributors::",
		"Tim Angus",
		"Vincent Cojot",
		"Ryan C. Gordon",
		"Aaron Gyes",
		//"Zack Middleton",
		"Ludwig Nussel",
		"Julian Priestley",
		"Scirocco Six",
		"Thilo Schulz",
		"Zachary J. Slater",
		"Tony J. White",
		"...and many, many others!",  // keep this one last.
		NULL
	} },
	{"Additional Code from", {
		"OpenArena",
		"Xreal",
		"Smokin' Guns",
		"ZEQ2-lite",
		"Mercenaries Guild Client for Tremulous",
		NULL
	} },
	{"Additional Scripts from", {
		"OpenArena",
		NULL
	} },

	// End of credits
	{NULL, { NULL } }
};

/*
=================
UI_CreditMenu_Key
=================
*/
static sfxHandle_t UI_CreditMenu_Key( int key ) {
	if( key & K_CHAR_FLAG ) {
		return 0;
	}

	if (s_credits.quitAfter) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
	} else {
		UI_PopMenu();
	}
	return 0;
}


/*
===============
UI_CreditMenu_Draw
===============
*/
static void UI_CreditMenu_Draw( void ) {
	int i, j, y;

	y = 480 - (uis.realtime - s_credits.startTime) / 50.0f;

	for (i = 0; credits[i].header != NULL; i++) {
		if (i != 0) {
			y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
		}

		UI_DrawProportionalString( 64, y, credits[i].header, UI_BIGFONT, color_white );
		y += 1.42 * PROP_HEIGHT;

		for (j = 0; credits[i].lines[j]; j++) {
			UI_DrawProportionalString( 320, y, credits[i].lines[j], UI_CENTER|UI_SMALLFONT, color_white );
			y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;

			if (y > 480)
				break;
		}

		if (y > 480)
			break;
	}

	if (y < 0) {
		// It's... over.
		UI_CreditMenu_Key(0);
	}
}

/*
===============
UI_CreditMenu_Draw_QuitMessage
===============
*/
static void UI_CreditMenu_Draw_QuitMessage( void ) {
	int		y;
	int		numItems = 6; // 5 text lines, 1 blank line

	// Force exit after 20 seconds
	if (uis.realtime > s_credits.startTime + 20000) {
		UI_CreditMenu_Key(0);
	}

	y = (SCREEN_HEIGHT - numItems * (1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE)) / 2;

	// This is a unoffial game
	UI_DrawProportionalString( 320, y, "Turtle Arena developer(s) are in", UI_CENTER|UI_SMALLFONT, color_white );
	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "no way affiliated with", UI_CENTER|UI_SMALLFONT, color_white );
	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Viacom, Nickelodeon, or Ubisoft.", UI_CENTER|UI_SMALLFONT, color_white );

	// Copyright
	y += (1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE) * 2;
	UI_DrawProportionalString( 320, y, "TMNT(c) Viacom", UI_CENTER|UI_SMALLFONT, color_white );
	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Turtle Arena(c) 2009-2013 Zack Middleton", UI_CENTER|UI_SMALLFONT, color_white );

	// Website
	UI_DrawString( 320, 459, "http://turtlearena.googlecode.com/", UI_CENTER|UI_SMALLFONT, color_red );
}

/*
===============
UI_CreditMenu

Exit game message/credits
===============
*/
void UI_CreditMenu( void ) {
	memset( &s_credits, 0 ,sizeof(s_credits) );

	s_credits.startTime = uis.realtime;
	s_credits.quitAfter = qtrue;
//#ifdef TURTLEARENA // LONG_CREDITS
	s_credits.menu.draw = UI_CreditMenu_Draw_QuitMessage;
//#else
//	s_credits.menu.draw = UI_CreditMenu_Draw;
//#endif
	s_credits.menu.key = UI_CreditMenu_Key;
	s_credits.menu.fullscreen = qtrue;
	UI_PushMenu ( &s_credits.menu );
}

//#ifdef TURTLEARENA // LONG_CREDITS
/*
===============
UI_LongCreditMenu

Long credit screen
===============
*/
void UI_LongCreditMenu( void )
{
	memset( &s_credits, 0 ,sizeof(s_credits) );

	s_credits.startTime = uis.realtime;
	s_credits.quitAfter = qfalse;
	s_credits.menu.draw = UI_CreditMenu_Draw;
	s_credits.menu.key = UI_CreditMenu_Key;
	s_credits.menu.fullscreen = qtrue;
	UI_PushMenu ( &s_credits.menu );
}
//#endif
#else
typedef struct {
	menuframework_s	menu;
	int frame;
} creditsmenu_t;

static creditsmenu_t	s_credits;


/*
===============
UI_CreditMenu_Draw_ioq3
===============
*/
static void UI_CreditMenu_Draw_ioq3( void ) {
	int		y;
	int		i;

	// These are all people that have made commits to Subversion, and thus
	//  probably incomplete.
	// (These are in alphabetical order, for the defense of everyone's egos.)
	static const char *names[] = {
		"Tim Angus",
		"James Canete",
		"Vincent Cojot",
		"Ryan C. Gordon",
		"Aaron Gyes",
		"Zack Middleton",
		"Ludwig Nussel",
		"Julian Priestley",
		"Scirocco Six",
		"Thilo Schulz",
		"Zachary J. Slater",
		"Tony J. White",
		"...and many, many others!",  // keep this one last.
		NULL
	};

	// Center text vertically on the screen
	y = (SCREEN_HEIGHT - ARRAY_LEN(names) * (1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE)) / 2;

	UI_DrawProportionalString( 320, y, "ioquake3 contributors:", UI_CENTER|UI_SMALLFONT, color_white );
	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;

	for (i = 0; names[i]; i++) {
		UI_DrawProportionalString( 320, y, names[i], UI_CENTER|UI_SMALLFONT, color_white );
		y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	}

	UI_DrawString( 320, 459, "http://www.ioquake3.org/", UI_CENTER|UI_SMALLFONT, color_red );
}


/*
=================
UI_CreditMenu_Key
=================
*/
static sfxHandle_t UI_CreditMenu_Key( int key ) {
	if( key & K_CHAR_FLAG ) {
		return 0;
	}

	s_credits.frame++;
	if (s_credits.frame == 1) {
		s_credits.menu.draw = UI_CreditMenu_Draw_ioq3;
	} else {
		trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
	}
	return 0;
}


/*
===============
UI_CreditMenu_Draw
===============
*/
static void UI_CreditMenu_Draw( void ) {
	int		y;

	y = 12;
	UI_DrawProportionalString( 320, y, "id Software is:", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Programming", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "John Carmack, Robert A. Duffy, Jim Dose'", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Art", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Adrian Carmack, Kevin Cloud,", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Kenneth Scott, Seneca Menard, Fred Nilsson", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Game Designer", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Graeme Devine", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Level Design", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Tim Willits, Christian Antkow, Paul Jaquays", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "CEO", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Todd Hollenshead", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Director of Business Development", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Marty Stratton", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Biz Assist and id Mom", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Donna Jackson", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Development Assistance", UI_CENTER|UI_SMALLFONT, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Eric Webb", UI_CENTER|UI_SMALLFONT, color_white );

	y += 1.35 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawString( 320, y, "To order: 1-800-idgames     www.quake3arena.com     www.idsoftware.com", UI_CENTER|UI_SMALLFONT, color_red );
	y += SMALLCHAR_HEIGHT;
	UI_DrawString( 320, y, "Quake III Arena(c) 1999-2000, Id Software, Inc.  All Rights Reserved", UI_CENTER|UI_SMALLFONT, color_red );
}


/*
===============
UI_CreditMenu
===============
*/
void UI_CreditMenu( void ) {
	memset( &s_credits, 0 ,sizeof(s_credits) );

	s_credits.menu.draw = UI_CreditMenu_Draw;
	s_credits.menu.key = UI_CreditMenu_Key;
	s_credits.menu.fullscreen = qtrue;
	UI_PushMenu ( &s_credits.menu );
}
#endif
