/*
===========================================================================
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
Copyright (C) 2010-2011 Zack Middleton

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

INGAME SERVER MENU

Server only controls

=======================================================================
*/


#include "ui_local.h"

#ifdef TA_MISC // INGAME_SERVER_MENU

//#define INGAME_FRAME					"menu/art/addbotframe"
#define INGAME_FRAME					"menu/art/cut_frame"
#define INGAME_MENU_VERTICAL_SPACING	28

#define ID_CHANGEMAP			10
#define ID_ADDBOTS				11
#define ID_REMOVEBOTS			12
#define ID_RESTART				13
#define ID_BACK					14



typedef struct {
	menuframework_s	menu;

	menubitmap_s	frame;
	menutext_s		map;
	menutext_s		restart;
	menutext_s		addbots;
	menutext_s		removebots;

	menutext_s		back;
} inservermenu_t;

static inservermenu_t	s_inserver;


/*
=================
InServer_RestartAction
=================
*/
static void InServer_RestartAction( qboolean result ) {
	if( !result ) {
		return;
	}

	// We are a submenu so force exit menu on Pop.
	uis.menusp = 1;

	UI_PopMenu();
	trap_Cmd_ExecuteText( EXEC_APPEND, "map_restart 0\n" );
}


/*
=================
InServer_Event
=================
*/
void InServer_Event( void *ptr, int notification ) {
	if( notification != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_CHANGEMAP:
#ifdef TA_SP // ARCADE
		UI_StartServerMenu( !ui_singlePlayerActive.integer );
#else
		UI_StartServerMenu( trap_Cvar_VariableValue("g_gametype") != GT_SINGLE_PLAYER );
#endif
		break;

	case ID_RESTART:
		UI_ConfirmMenu( "Restart Level?", 0, InServer_RestartAction );
		break;

	case ID_ADDBOTS:
		UI_AddBotsMenu();
		break;

	case ID_REMOVEBOTS:
		UI_RemoveBotsMenu();
		break;


	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
=================
UI_InServerMenu_Draw
=================
*/
static void UI_InServerMenu_Draw( void ) {
	UI_DrawBannerString( 320, 16, "SERVER OPTIONS", UI_CENTER, text_banner_color );

	// standard menu drawing
	Menu_Draw( &s_inserver.menu );
}

/*
=================
InServer_MenuInit
=================
*/
void InServer_MenuInit( void ) {
	int		y;

	memset( &s_inserver, 0 ,sizeof(inservermenu_t) );

	InServer_Cache();

	s_inserver.menu.draw = UI_InServerMenu_Draw;
	s_inserver.menu.wrapAround = qtrue;
	s_inserver.menu.fullscreen = qfalse;

	s_inserver.frame.generic.type		= MTYPE_BITMAP;
	s_inserver.frame.generic.flags		= QMF_INACTIVE;
	s_inserver.frame.generic.name		= INGAME_FRAME;
	s_inserver.frame.generic.x			= 320-233;//142;
	s_inserver.frame.generic.y			= 240-166;//118;
	s_inserver.frame.width				= 466;//359;
	s_inserver.frame.height				= 332;//256;

	y = 96;
	//y = 88;

	y += INGAME_MENU_VERTICAL_SPACING*2;

	s_inserver.map.generic.type			= MTYPE_PTEXT;
	s_inserver.map.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_inserver.map.generic.x			= 320;
	s_inserver.map.generic.y			= y;
	s_inserver.map.generic.id			= ID_CHANGEMAP;
	s_inserver.map.generic.callback		= InServer_Event;
	s_inserver.map.string				= "Change Map";
	s_inserver.map.color				= text_big_color;
	s_inserver.map.style				= UI_CENTER|UI_SMALLFONT;
	if( !trap_Cvar_VariableValue( "sv_running" ) ) {
		s_inserver.map.generic.flags |= QMF_GRAYED;
	}

	y += INGAME_MENU_VERTICAL_SPACING;
	s_inserver.addbots.generic.type			= MTYPE_PTEXT;
	s_inserver.addbots.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_inserver.addbots.generic.x			= 320;
	s_inserver.addbots.generic.y			= y;
	s_inserver.addbots.generic.id			= ID_ADDBOTS;
	s_inserver.addbots.generic.callback		= InServer_Event; 
	s_inserver.addbots.string				= "Add Bots";
	s_inserver.addbots.color				= text_big_color;
	s_inserver.addbots.style				= UI_CENTER|UI_SMALLFONT;
	if( !trap_Cvar_VariableValue( "bot_enable" )
#ifdef TA_SP
	|| ui_singlePlayerActive.integer
#else
	|| (trap_Cvar_VariableValue( "g_gametype" ) == GT_SINGLE_PLAYER)
#endif
	) {
		s_inserver.addbots.generic.flags |= QMF_GRAYED;
	}

	y += INGAME_MENU_VERTICAL_SPACING;
	s_inserver.removebots.generic.type		= MTYPE_PTEXT;
	s_inserver.removebots.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_inserver.removebots.generic.x			= 320;
	s_inserver.removebots.generic.y			= y;
	s_inserver.removebots.generic.id		= ID_REMOVEBOTS;
	s_inserver.removebots.generic.callback	= InServer_Event; 
	s_inserver.removebots.string			= "Remove Bots";
	s_inserver.removebots.color				= text_big_color;
	s_inserver.removebots.style				= UI_CENTER|UI_SMALLFONT;
	if( !trap_Cvar_VariableValue( "bot_enable" )
#ifdef TA_SP
	|| ui_singlePlayerActive.integer
#else
	|| (trap_Cvar_VariableValue( "g_gametype" ) == GT_SINGLE_PLAYER)
#endif
	) {
		s_inserver.removebots.generic.flags |= QMF_GRAYED;
	}

	y += INGAME_MENU_VERTICAL_SPACING;
	s_inserver.restart.generic.type			= MTYPE_PTEXT;
	s_inserver.restart.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_inserver.restart.generic.x			= 320;
	s_inserver.restart.generic.y			= y;
	s_inserver.restart.generic.id			= ID_RESTART;
	s_inserver.restart.generic.callback		= InServer_Event; 
	s_inserver.restart.string				= "Restart Level";
	s_inserver.restart.color				= text_big_color;
	s_inserver.restart.style				= UI_CENTER|UI_SMALLFONT;


	y += INGAME_MENU_VERTICAL_SPACING*2;
	s_inserver.back.generic.type		= MTYPE_PTEXT;
	s_inserver.back.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_inserver.back.generic.x			= 320;
	s_inserver.back.generic.y			= y;
	s_inserver.back.generic.id			= ID_BACK;
	s_inserver.back.generic.callback	= InServer_Event; 
	s_inserver.back.string				= "Back";
	s_inserver.back.color				= text_big_color;
	s_inserver.back.style				= UI_CENTER|UI_SMALLFONT;

	Menu_AddItem( &s_inserver.menu, &s_inserver.frame );
	Menu_AddItem( &s_inserver.menu, &s_inserver.map );
	Menu_AddItem( &s_inserver.menu, &s_inserver.addbots );
	Menu_AddItem( &s_inserver.menu, &s_inserver.removebots );
	Menu_AddItem( &s_inserver.menu, &s_inserver.restart );
	Menu_AddItem( &s_inserver.menu, &s_inserver.back );
}


/*
=================
InServer_Cache
=================
*/
void InServer_Cache( void ) {
	trap_R_RegisterShaderNoMip( INGAME_FRAME );
}


/*
=================
UI_InServerMenu
=================
*/
void UI_InServerMenu( void ) {
	InServer_MenuInit();
	UI_PushMenu( &s_inserver.menu );
}
#endif

