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

MULTIPLAYER MENU

=======================================================================
*/

#include "ui_local.h"

#ifdef TA_MISC

#define SETUP_MENU_VERTICAL_SPACING		34

#define ART_BACK0		"menu/art/back_0"
#define ART_BACK1		"menu/art/back_1"
#define ART_FRAMEL		"menu/art/frame2_l"
#define ART_FRAMER		"menu/art/frame1_r"

#define ID_CREATEGAME			10
#define ID_JOINSEARCH			11
#define ID_JOINSPECIFY			12
#define ID_BACK					13


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menutext_s		creategame;
	menutext_s		joinsearch;
	menutext_s		joinspecify;
	menutext_s		setupplayers;

	menubitmap_s	back;
} multiplayerMenu_t;

static multiplayerMenu_t	multiplayerMenu;

/*
===============
UI_JoinSpecify
===============
*/
static void UI_JoinSpecifyMenu( void ) {
	UI_SpecifyServerMenu(qfalse);
}

/*
===============
UI_MultiplayerMenu_Event
===============
*/
static void UI_MultiplayerMenu_Event( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {

	case ID_CREATEGAME:
		UI_StartServerMenu( qtrue );
		break;

	case ID_JOINSEARCH:
		UI_PlayerSetupMenu(MAX_SPLITVIEW, UI_ArenaServersMenu, qfalse);
		break;

	case ID_JOINSPECIFY:
		UI_PlayerSetupMenu(MAX_SPLITVIEW, UI_JoinSpecifyMenu, qfalse);
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
===============
UI_Multiplayer_MenuInit
===============
*/
static void UI_Multiplayer_MenuInit( void ) {
	int				y;

	UI_Multiplayer_Cache();

	memset( &multiplayerMenu, 0, sizeof(multiplayerMenu) );
	multiplayerMenu.menu.wrapAround = qtrue;
	multiplayerMenu.menu.fullscreen = qtrue;

	multiplayerMenu.banner.generic.type				= MTYPE_BTEXT;
	multiplayerMenu.banner.generic.x				= 320;
	multiplayerMenu.banner.generic.y				= 16;
	multiplayerMenu.banner.string					= "MULTIPLAYER";
	multiplayerMenu.banner.color					= text_banner_color;
	multiplayerMenu.banner.style					= UI_CENTER;

	multiplayerMenu.framel.generic.type				= MTYPE_BITMAP;
	multiplayerMenu.framel.generic.name				= ART_FRAMEL;
	multiplayerMenu.framel.generic.flags			= QMF_INACTIVE;
	multiplayerMenu.framel.generic.x				= 0;
	multiplayerMenu.framel.generic.y				= 78;
	multiplayerMenu.framel.width  					= 256;
	multiplayerMenu.framel.height  					= 329;

	multiplayerMenu.framer.generic.type				= MTYPE_BITMAP;
	multiplayerMenu.framer.generic.name				= ART_FRAMER;
	multiplayerMenu.framer.generic.flags			= QMF_INACTIVE;
	multiplayerMenu.framer.generic.x				= 376;
	multiplayerMenu.framer.generic.y				= 76;
	multiplayerMenu.framer.width  					= 256;
	multiplayerMenu.framer.height  					= 334;

	y = (SCREEN_HEIGHT - 3*SETUP_MENU_VERTICAL_SPACING) * 0.5f;
	multiplayerMenu.creategame.generic.type			= MTYPE_PTEXT;
	multiplayerMenu.creategame.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	multiplayerMenu.creategame.generic.x			= 320;
	multiplayerMenu.creategame.generic.y			= y;
	multiplayerMenu.creategame.generic.id			= ID_CREATEGAME;
	multiplayerMenu.creategame.generic.callback		= UI_MultiplayerMenu_Event;
	multiplayerMenu.creategame.string				= "Start Server";
	multiplayerMenu.creategame.color				= text_big_color;
	multiplayerMenu.creategame.style				= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	multiplayerMenu.joinsearch.generic.type			= MTYPE_PTEXT;
	multiplayerMenu.joinsearch.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	multiplayerMenu.joinsearch.generic.x			= 320;
	multiplayerMenu.joinsearch.generic.y			= y;
	multiplayerMenu.joinsearch.generic.id			= ID_JOINSEARCH;
	multiplayerMenu.joinsearch.generic.callback		= UI_MultiplayerMenu_Event;
	multiplayerMenu.joinsearch.string				= "Join Server (Browse)";
	multiplayerMenu.joinsearch.color				= text_big_color;
	multiplayerMenu.joinsearch.style				= UI_CENTER;

	y += SETUP_MENU_VERTICAL_SPACING;
	multiplayerMenu.joinspecify.generic.type		= MTYPE_PTEXT;
	multiplayerMenu.joinspecify.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	multiplayerMenu.joinspecify.generic.x			= 320;
	multiplayerMenu.joinspecify.generic.y			= y;
	multiplayerMenu.joinspecify.generic.id			= ID_JOINSPECIFY;
	multiplayerMenu.joinspecify.generic.callback	= UI_MultiplayerMenu_Event;
	multiplayerMenu.joinspecify.string				= "Join Server (Specify)";
	multiplayerMenu.joinspecify.color				= text_big_color;
	multiplayerMenu.joinspecify.style				= UI_CENTER;

	multiplayerMenu.back.generic.type				= MTYPE_BITMAP;
	multiplayerMenu.back.generic.name				= ART_BACK0;
	multiplayerMenu.back.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	multiplayerMenu.back.generic.id					= ID_BACK;
	multiplayerMenu.back.generic.callback			= UI_MultiplayerMenu_Event;
	multiplayerMenu.back.generic.x					= 0;
	multiplayerMenu.back.generic.y					= 480-64;
	multiplayerMenu.back.width						= 128;
	multiplayerMenu.back.height						= 64;
	multiplayerMenu.back.focuspic					= ART_BACK1;

	Menu_AddItem( &multiplayerMenu.menu, &multiplayerMenu.banner );
	Menu_AddItem( &multiplayerMenu.menu, &multiplayerMenu.framel );
	Menu_AddItem( &multiplayerMenu.menu, &multiplayerMenu.framer );

	Menu_AddItem( &multiplayerMenu.menu, &multiplayerMenu.creategame );
	Menu_AddItem( &multiplayerMenu.menu, &multiplayerMenu.joinsearch );
	Menu_AddItem( &multiplayerMenu.menu, &multiplayerMenu.joinspecify );

	Menu_AddItem( &multiplayerMenu.menu, &multiplayerMenu.back );
}

/*
=================
UI_Multiplayer_Cache
=================
*/
void UI_Multiplayer_Cache( void )
{
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
}

/*
===============
UI_MultiplayerMenu
===============
*/
void UI_MultiplayerMenu( void )
{
	UI_Multiplayer_MenuInit();
	UI_PushMenu( &multiplayerMenu.menu );
}

#endif // TA_MISC
