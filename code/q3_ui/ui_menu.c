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

MAIN MENU

=======================================================================
*/


#include "ui_local.h"


#define ID_SINGLEPLAYER			10
#ifdef TA_SP
#define ID_ARCADE				11
#define ID_MULTIPLAYER			12
#define ID_SETUP				13
#else
#define ID_MULTIPLAYER			11
#define ID_SETUP				12
#define ID_DEMOS				13
#define ID_CINEMATICS			14
#ifndef MISSIONPACK
#define ID_TEAMARENA			15
#endif
#define ID_MODS					16
#endif
#define ID_EXIT					17

#ifdef TURTLEARENA // BANNER_IMAGE
#define ART_BANNER_IMAGE				"menu/art/titlebanner"
#else
#define MAIN_BANNER_MODEL				"models/mapobjects/banner/banner5.md3"
#endif
#define MAIN_MENU_VERTICAL_SPACING		34


typedef struct {
	menuframework_s	menu;

#ifdef TURTLEARENA // BANNER_IMAGE
	menubitmap_s	banner_image;
#endif
	menutext_s		singleplayer;
#ifdef TA_SP // ARCADE
	menutext_s		arcade;
#endif
	menutext_s		multiplayer;
	menutext_s		setup;
#ifndef TA_SP
	menutext_s		demos;
	menutext_s		cinematics;
#ifndef MISSIONPACK
	menutext_s		teamArena;
#endif
	menutext_s		mods;
#endif
	menutext_s		exit;

#ifndef TURTLEARENA // BANNER_IMAGE
	qhandle_t		bannerModel;
#endif
} mainmenu_t;


static mainmenu_t s_main;

typedef struct {
	menuframework_s menu;	
#ifdef IOQ3ZTM
	char errorMessage[256];
#else
	char errorMessage[4096];
#endif
#ifdef TURTLEARENA // BANNER_IMAGE
	menubitmap_s	banner_image;
#endif
} errorMessage_t;

static errorMessage_t s_errorMessage;

/*
=================
MainMenu_ExitAction
=================
*/
static void MainMenu_ExitAction( qboolean result ) {
	if( !result ) {
		return;
	}
	UI_PopMenu();
	UI_CreditMenu();
}



/*
=================
Main_MenuEvent
=================
*/
void Main_MenuEvent (void* ptr, int event) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_SINGLEPLAYER:
#ifdef TA_SP
		UI_SPMenu();
#else
		UI_SPLevelMenu();
#endif
		break;

#ifdef TA_SP // ARCADE
	case ID_ARCADE:
		UI_StartServerMenu(qfalse);
		break;
#endif

	case ID_MULTIPLAYER:
#ifdef TA_MISC
		UI_MultiplayerMenu();
#else
		UI_ArenaServersMenu();
#endif
		break;

	case ID_SETUP:
		UI_SetupMenu();
		break;

#ifndef TA_SP
	case ID_DEMOS:
		UI_DemosMenu();
		break;

	case ID_CINEMATICS:
		UI_CinematicsMenu();
		break;

	case ID_MODS:
		UI_ModsMenu();
		break;

#ifndef MISSIONPACK
	case ID_TEAMARENA:
		trap_Cvar_Set( "fs_game", BASETA);
		trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
		break;
#endif
#endif

	case ID_EXIT:
#ifdef TA_MISC
		UI_ConfirmMenu( "Exit Game?", 0, MainMenu_ExitAction );
#else
		UI_ConfirmMenu( "EXIT GAME?", 0, MainMenu_ExitAction );
#endif
		break;
	}
}


/*
===============
MainMenu_Cache
===============
*/
void MainMenu_Cache( void ) {
#ifndef TURTLEARENA // BANNER_IMAGE
	s_main.bannerModel = trap_R_RegisterModel( MAIN_BANNER_MODEL );
#endif
}

sfxHandle_t ErrorMessage_Key(int key)
{
	trap_Cvar_Set( "com_errorMessage", "" );
	UI_MainMenu();
	return (menu_null_sound);
}

/*
===============
Main_MenuDraw
TTimo: this function is common to the main menu and errorMessage menu
===============
*/

static void Main_MenuDraw( void ) {
#ifndef TURTLEARENA // BANNER_IMAGE
	refdef_t		refdef;
	refEntity_t		ent;
	vec3_t			origin;
	vec3_t			angles;
	float			adjust;
	float			x, y, w, h;
#endif
#ifndef TURTLEARENA
	vec4_t			color = {0.5, 0, 0, 1};
#endif

#ifndef TURTLEARENA // BANNER_IMAGE
	// setup the refdef

	memset( &refdef, 0, sizeof( refdef ) );

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	x = 0;
	y = 0;
	w = 640;
	h = 120;
	UI_AdjustFrom640( &x, &y, &w, &h );
	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	adjust = 0; // JDC: Kenneth asked me to stop this 1.0 * sin( (float)uis.realtime / 1000 );
	refdef.fov_x = 60 + adjust;
	refdef.fov_y = 19.6875 + adjust;

	refdef.time = uis.realtime;

	origin[0] = 300;
	origin[1] = 0;
	origin[2] = -32;

	trap_R_ClearScene();

	// add the model

	memset( &ent, 0, sizeof(ent) );

	adjust = 5.0 * sin( (float)uis.realtime / 5000 );
	VectorSet( angles, 0, 180 + adjust, 0 );
	AnglesToAxis( angles, ent.axis );
	ent.hModel = s_main.bannerModel;
	VectorCopy( origin, ent.origin );
	VectorCopy( origin, ent.lightingOrigin );
	ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
	VectorCopy( ent.origin, ent.oldorigin );

	trap_R_AddRefEntityToScene( &ent );

	trap_R_RenderScene( &refdef );
#endif
	
	if (strlen(s_errorMessage.errorMessage))
	{
#ifdef TURTLEARENA // BANNER_IMAGE
		// standard menu drawing
		Menu_Draw( &s_errorMessage.menu );

		UI_DrawProportionalString_AutoWrapped( 320, 224+SMALLCHAR_HEIGHT*2, 600, 20, s_errorMessage.errorMessage, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );
#else
		UI_DrawProportionalString_AutoWrapped( 320, 192, 600, 20, s_errorMessage.errorMessage, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );
#endif
#ifdef IOQ3ZTM
		UI_DrawProportionalString( 320, 480-SMALLCHAR_HEIGHT*4, "Press any key", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );
#endif
	}
	else
	{
		// standard menu drawing
		Menu_Draw( &s_main.menu );		
	}

#ifndef TURTLEARENA // Legal stuff...
	if (uis.demoversion) {
		UI_DrawProportionalString( 320, 372, "DEMO      FOR MATURE AUDIENCES      DEMO", UI_CENTER|UI_SMALLFONT, color );
		UI_DrawString( 320, 400, "Quake III Arena(c) 1999-2000, Id Software, Inc.  All Rights Reserved", UI_CENTER|UI_SMALLFONT, color );
	} else {
		UI_DrawString( 320, 450, "Quake III Arena(c) 1999-2000, Id Software, Inc.  All Rights Reserved", UI_CENTER|UI_SMALLFONT, color );
	}
#endif
}


#ifndef MISSIONPACK
/*
===============
UI_TeamArenaExists
===============
*/
static qboolean UI_TeamArenaExists( void ) {
	int		numdirs;
	char	dirlist[2048];
	char	*dirptr;
  char  *descptr;
	int		i;
	int		dirlen;

	numdirs = trap_FS_GetFileList( "$modlist", "", dirlist, sizeof(dirlist) );
	dirptr  = dirlist;
	for( i = 0; i < numdirs; i++ ) {
		dirlen = strlen( dirptr ) + 1;
    descptr = dirptr + dirlen;
		if (Q_stricmp(dirptr, BASETA) == 0) {
			return qtrue;
		}
    dirptr += dirlen + strlen(descptr) + 1;
	}
	return qfalse;
}
#endif


/*
===============
UI_MainMenu

The main menu only comes up when not in a game,
so make sure that the attract loop server is down
and that local cinematics are killed
===============
*/
void UI_MainMenu( void ) {
	int		y;
#ifndef TA_SP
#ifndef MISSIONPACK
	qboolean teamArena = qfalse;
#endif
#endif
	int		style = UI_CENTER | UI_DROPSHADOW;

	trap_Cvar_Set( "sv_killserver", "1" );
	
	memset( &s_main, 0 ,sizeof(mainmenu_t) );
	memset( &s_errorMessage, 0 ,sizeof(errorMessage_t) );

	// com_errorMessage would need that too
	MainMenu_Cache();
	
	trap_Cvar_VariableStringBuffer( "com_errorMessage", s_errorMessage.errorMessage, sizeof(s_errorMessage.errorMessage) );
	if (strlen(s_errorMessage.errorMessage))
	{	
		s_errorMessage.menu.draw = Main_MenuDraw;
		s_errorMessage.menu.key = ErrorMessage_Key;
		s_errorMessage.menu.fullscreen = qtrue;
		s_errorMessage.menu.wrapAround = qtrue;
#ifndef TA_DATA
		s_errorMessage.menu.showlogo = qtrue;		
#endif
#ifdef IOQ3ZTM
		s_errorMessage.menu.noEscape = qtrue;
#endif

#ifdef TURTLEARENA // BANNER_IMAGE
		s_errorMessage.banner_image.generic.type				= MTYPE_BITMAP;
		s_errorMessage.banner_image.generic.name				= ART_BANNER_IMAGE;
		s_errorMessage.banner_image.generic.flags				= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
		s_errorMessage.banner_image.generic.x					= 320;
		s_errorMessage.banner_image.generic.y					= 32;
		s_errorMessage.banner_image.width  						= 512;
		s_errorMessage.banner_image.height  					= 256;

		Menu_AddItem( &s_errorMessage.menu,	&s_errorMessage.banner_image );
#endif

		trap_Key_SetCatcher( KEYCATCH_UI );
		uis.menusp = 0;
		UI_PushMenu ( &s_errorMessage.menu );
		
		return;
	}

#ifdef TA_MISC
	trap_S_StopBackgroundTrack();
	trap_S_StartBackgroundTrack("music/menu.ogg", NULL);
#endif

	s_main.menu.draw = Main_MenuDraw;
	s_main.menu.fullscreen = qtrue;
	s_main.menu.wrapAround = qtrue;
#ifndef TA_DATA
	s_main.menu.showlogo = qtrue;
#endif
#ifdef IOQ3ZTM
	s_main.menu.noEscape = qtrue;
#endif

#ifdef TURTLEARENA // BANNER_IMAGE
	s_main.banner_image.generic.type				= MTYPE_BITMAP;
	s_main.banner_image.generic.name				= ART_BANNER_IMAGE;
	s_main.banner_image.generic.flags				= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_main.banner_image.generic.x					= 320;
	s_main.banner_image.generic.y					= 32;
	s_main.banner_image.width  						= 512;
	s_main.banner_image.height  					= 256;
#endif

#ifdef TA_SP
	y = 480 - (MAIN_MENU_VERTICAL_SPACING * 6);
#else
	y = 134;
#endif
	s_main.singleplayer.generic.type		= MTYPE_PTEXT;
	s_main.singleplayer.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.singleplayer.generic.x			= 320;
	s_main.singleplayer.generic.y			= y;
	s_main.singleplayer.generic.id			= ID_SINGLEPLAYER;
	s_main.singleplayer.generic.callback	= Main_MenuEvent; 
#ifdef TA_SP // Moved to MAIN GAME Menu.
	s_main.singleplayer.string				= "Main Game";
#else
	s_main.singleplayer.string				= "SINGLE PLAYER";
#endif
	s_main.singleplayer.color				= text_big_color;
	s_main.singleplayer.style				= style;

#ifdef TA_SP // ARCADE
	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.arcade.generic.type				= MTYPE_PTEXT;
	s_main.arcade.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.arcade.generic.x					= 320;
	s_main.arcade.generic.y					= y;
	s_main.arcade.generic.id				= ID_ARCADE;
	s_main.arcade.generic.callback			= Main_MenuEvent; 
	s_main.arcade.string					= "Arcade Mode";
	s_main.arcade.color						= text_big_color;
	s_main.arcade.style						= style;
#endif

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.multiplayer.generic.type			= MTYPE_PTEXT;
	s_main.multiplayer.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.multiplayer.generic.x			= 320;
	s_main.multiplayer.generic.y			= y;
	s_main.multiplayer.generic.id			= ID_MULTIPLAYER;
	s_main.multiplayer.generic.callback		= Main_MenuEvent; 
#ifdef TA_MISC
	s_main.multiplayer.string				= "Multiplayer";
#else
	s_main.multiplayer.string				= "MULTIPLAYER";
#endif
	s_main.multiplayer.color				= text_big_color;
	s_main.multiplayer.style				= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.setup.generic.type				= MTYPE_PTEXT;
	s_main.setup.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.setup.generic.x					= 320;
	s_main.setup.generic.y					= y;
	s_main.setup.generic.id					= ID_SETUP;
	s_main.setup.generic.callback			= Main_MenuEvent; 
#ifdef TA_SP // Moved to OPTIONS Menu.
	s_main.setup.string						= "Options";
#else
	s_main.setup.string						= "SETUP";
#endif
	s_main.setup.color						= text_big_color;
	s_main.setup.style						= style;

#ifndef TA_SP // Moved to OPTIONS Menu.
	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.demos.generic.type				= MTYPE_PTEXT;
	s_main.demos.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.demos.generic.x					= 320;
	s_main.demos.generic.y					= y;
	s_main.demos.generic.id					= ID_DEMOS;
	s_main.demos.generic.callback			= Main_MenuEvent; 
	s_main.demos.string						= "DEMOS";
	s_main.demos.color						= text_big_color;
	s_main.demos.style						= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.cinematics.generic.type			= MTYPE_PTEXT;
	s_main.cinematics.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.cinematics.generic.x				= 320;
	s_main.cinematics.generic.y				= y;
	s_main.cinematics.generic.id			= ID_CINEMATICS;
	s_main.cinematics.generic.callback		= Main_MenuEvent; 
	s_main.cinematics.string				= "CINEMATICS";
	s_main.cinematics.color					= text_big_color;
	s_main.cinematics.style					= style;

#ifndef MISSIONPACK
	if ( !uis.demoversion && UI_TeamArenaExists() ) {
		teamArena = qtrue;
		y += MAIN_MENU_VERTICAL_SPACING;
		s_main.teamArena.generic.type			= MTYPE_PTEXT;
		s_main.teamArena.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		s_main.teamArena.generic.x				= 320;
		s_main.teamArena.generic.y				= y;
		s_main.teamArena.generic.id				= ID_TEAMARENA;
		s_main.teamArena.generic.callback		= Main_MenuEvent; 
		s_main.teamArena.string					= "TEAM ARENA";
		s_main.teamArena.color					= text_big_color;
		s_main.teamArena.style					= style;
	}
#endif

	if ( !uis.demoversion ) {
		y += MAIN_MENU_VERTICAL_SPACING;
		s_main.mods.generic.type			= MTYPE_PTEXT;
		s_main.mods.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		s_main.mods.generic.x				= 320;
		s_main.mods.generic.y				= y;
		s_main.mods.generic.id				= ID_MODS;
		s_main.mods.generic.callback		= Main_MenuEvent; 
		s_main.mods.string					= "MODS";
		s_main.mods.color					= text_big_color;
		s_main.mods.style					= style;
	}
#endif

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.exit.generic.type				= MTYPE_PTEXT;
	s_main.exit.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.exit.generic.x					= 320;
	s_main.exit.generic.y					= y;
	s_main.exit.generic.id					= ID_EXIT;
	s_main.exit.generic.callback			= Main_MenuEvent; 
#ifdef TA_MISC
	s_main.exit.string						= "Exit";
#else
	s_main.exit.string						= "EXIT";
#endif
	s_main.exit.color						= text_big_color;
	s_main.exit.style						= style;

#ifdef TURTLEARENA // BANNER_IMAGE
	Menu_AddItem( &s_main.menu,	&s_main.banner_image );
#endif
	Menu_AddItem( &s_main.menu,	&s_main.singleplayer );
#ifdef TA_SP // ARCADE
	Menu_AddItem( &s_main.menu,	&s_main.arcade );
#endif
	Menu_AddItem( &s_main.menu,	&s_main.multiplayer );
	Menu_AddItem( &s_main.menu,	&s_main.setup );
#ifndef TA_SP // Moved to PLAY Menu.
	Menu_AddItem( &s_main.menu,	&s_main.demos );
	Menu_AddItem( &s_main.menu,	&s_main.cinematics );
#ifndef MISSIONPACK
	if (teamArena) {
		Menu_AddItem( &s_main.menu,	&s_main.teamArena );
	}
#endif
	if ( !uis.demoversion ) {
		Menu_AddItem( &s_main.menu,	&s_main.mods );
	}
#endif
	Menu_AddItem( &s_main.menu,	&s_main.exit );             

	trap_Key_SetCatcher( KEYCATCH_UI );
	uis.menusp = 0;
	UI_PushMenu ( &s_main.menu );
		
}
