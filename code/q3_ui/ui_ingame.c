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

INGAME MENU

=======================================================================
*/


#include "ui_local.h"


#define INGAME_FRAME					"menu/art/addbotframe"
//#define INGAME_FRAME					"menu/art/cut_frame"
#define INGAME_MENU_VERTICAL_SPACING	28

#define ID_TEAM					10
#ifdef TA_MISC // INGAME_SERVER_MENU
#define ID_SERVER				11
#else
#define ID_ADDBOTS				11
#define ID_REMOVEBOTS			12
#endif
#define ID_SETUP				13
#define ID_SERVERINFO			14
#define ID_LEAVEARENA			15
#ifndef TA_MISC // INGAME_SERVER_MENU
#define ID_RESTART				16
#endif
#define ID_QUIT					17
#define ID_RESUME				18
#define ID_TEAMORDERS			19
#ifndef TA_MISC
#define ID_LOCALPLAYERS			20
#endif
#ifdef TA_MISC // SMART_JOIN_MENU
#define ID_JOINGAME				21
#define ID_SPECTATE				22
#endif
#ifdef TA_MISC
#define ID_CUSTOMIZEPLAYER		23
#endif


typedef struct {
	menuframework_s	menu;

	menubitmap_s	frame;
	menutext_s		team;
	menutext_s		setup;
	menutext_s		server;
	menutext_s		leave;
#ifdef TA_MISC // INGAME_SERVER_MENU
	menutext_s		inserver;
#else
	menutext_s		restart;
	menutext_s		addbots;
	menutext_s		removebots;
#endif
#ifdef TA_MISC
	menutext_s		setupplayers;
#endif
	menutext_s		teamorders;
	menutext_s		quit;
	menutext_s		resume;
	menutext_s		localPlayers;
} ingamemenu_t;

static ingamemenu_t	s_ingame;


#ifndef TA_MISC // INGAME_SERVER_MENU
/*
=================
InGame_RestartAction
=================
*/
static void InGame_RestartAction( qboolean result ) {
	if( !result ) {
		return;
	}

	UI_PopMenu();
	trap_Cmd_ExecuteText( EXEC_APPEND, "map_restart 0\n" );
}
#endif


/*
=================
InGame_QuitAction
=================
*/
static void InGame_QuitAction( qboolean result ) {
	if( !result ) {
		return;
	}
	UI_PopMenu();
	UI_CreditMenu();
}


/*
=================
UI_TogglePlayerIngame
=================
*/
void UI_TogglePlayerIngame(int localClientNum)
{
	uiClientState_t	cs;

	trap_GetClientState( &cs );

	if (cs.clientNums[localClientNum] == -1) {
		trap_Cmd_ExecuteText( EXEC_APPEND, va("%s\n", Com_LocalClientCvarName(localClientNum, "dropin")) );
	} else {
		trap_Cmd_ExecuteText( EXEC_APPEND, va("%s\n", Com_LocalClientCvarName(localClientNum, "dropout")) );
	}

	UI_ForceMenuOff ();
}

/*
=================
InGame_Event
=================
*/
void InGame_Event( void *ptr, int notification ) {
	if( notification != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_TEAM:
		InSelectPlayerMenu(UI_TeamMainMenu, "CHANGE TEAM", qtrue);
		break;

#ifdef TA_MISC // SMART_JOIN_MENU
	case ID_JOINGAME:
		trap_Cmd_ExecuteText( EXEC_APPEND, "cmd team free\n" );
		UI_ForceMenuOff();
		break;

	case ID_SPECTATE:
		trap_Cmd_ExecuteText( EXEC_APPEND, "cmd team spectator\n" );
		UI_ForceMenuOff();
		break;
#endif

	case ID_SETUP:
		UI_SetupMenu();
		break;

	case ID_LEAVEARENA:
		trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
		break;

#ifndef TA_MISC // INGAME_SERVER_MENU
	case ID_RESTART:
		UI_ConfirmMenu( "RESTART ARENA?", 0, InGame_RestartAction );
		break;
#endif

	case ID_QUIT:
		UI_ConfirmMenu( "EXIT GAME?",  0, InGame_QuitAction );
		break;

	case ID_SERVERINFO:
		UI_ServerInfoMenu();
		break;

#ifdef TA_MISC // INGAME_SERVER_MENU
	case ID_SERVER:
		UI_InServerMenu();
		break;
#else
	case ID_ADDBOTS:
		UI_AddBotsMenu();
		break;

	case ID_REMOVEBOTS:
		UI_RemoveBotsMenu();
		break;
#endif

#ifdef TA_MISC
	case ID_CUSTOMIZEPLAYER:
		UI_PlayerSetupMenu(MAX_SPLITVIEW, 0/*NULL*/, qtrue);
		break;
#endif

	case ID_TEAMORDERS:
		UI_TeamOrdersMenu();
		break;

	case ID_RESUME:
		UI_PopMenu();
		break;

#ifndef TA_MISC
	case ID_LOCALPLAYERS:
		InSelectPlayerMenu(UI_TogglePlayerIngame, "ADD OR DROP", qfalse);
		break;
#endif
	}
}


/*
=================
InGame_MenuInit
=================
*/
void InGame_MenuInit( void ) {
	int		y;
	uiClientState_t	cs;
	char	info[MAX_INFO_STRING];
	int		team;
#ifdef TA_MISC // SMART_JOIN_MENU
	int		numLocalClients;
#endif

	memset( &s_ingame, 0 ,sizeof(ingamemenu_t) );

	InGame_Cache();

	s_ingame.menu.wrapAround = qtrue;
	s_ingame.menu.fullscreen = qfalse;

	s_ingame.frame.generic.type			= MTYPE_BITMAP;
	s_ingame.frame.generic.flags		= QMF_INACTIVE;
	s_ingame.frame.generic.name			= INGAME_FRAME;
	s_ingame.frame.generic.x			= 320-233;//142;
	s_ingame.frame.generic.y			= 240-166;//118;
	s_ingame.frame.width				= 466;//359;
	s_ingame.frame.height				= 332;//256;

#ifdef TA_MISC // INGAME_SERVER_MENU
	y = 88+INGAME_MENU_VERTICAL_SPACING/2;
#else
	if (UI_MaxSplitView() > 1) {
		y = 88;
	} else {
		y = 96;
	}
#endif

#ifdef TA_MISC
	s_ingame.resume.generic.type			= MTYPE_PTEXT;
	s_ingame.resume.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.resume.generic.x				= 320;
	s_ingame.resume.generic.y				= y;
	s_ingame.resume.generic.id				= ID_RESUME;
	s_ingame.resume.generic.callback		= InGame_Event; 
	s_ingame.resume.string					= "Resume Game";
	s_ingame.resume.color					= text_big_color;
	s_ingame.resume.style					= UI_CENTER|UI_SMALLFONT;

	y += INGAME_MENU_VERTICAL_SPACING;
#endif
#ifdef TA_MISC // SMART_JOIN_MENU
	trap_GetClientState( &cs );
	trap_GetConfigString( CS_PLAYERS + cs.clientNums[0], info, MAX_INFO_STRING );
	numLocalClients = UI_NumLocalClients(&cs);

	// Force if more than one local client
	if (numLocalClients > 1 || (trap_Cvar_VariableValue( "g_gametype" ) >= GT_TEAM) ) {
		s_ingame.team.generic.type			= MTYPE_PTEXT;
		s_ingame.team.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		s_ingame.team.generic.x				= 320;
		s_ingame.team.generic.y				= y;
		s_ingame.team.generic.id			= ID_TEAM;
		s_ingame.team.generic.callback		= InGame_Event;
		if (numLocalClients > 1)
			s_ingame.team.string			= "Change Teams";
		else
			s_ingame.team.string			= "Change Team";
		s_ingame.team.color					= text_big_color;
		s_ingame.team.style					= UI_CENTER|UI_SMALLFONT;
	}
	else {
		team = atoi( Info_ValueForKey( info, "t" ) );
		if( team == TEAM_SPECTATOR ) {
			s_ingame.team.generic.type			= MTYPE_PTEXT;
			s_ingame.team.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
			s_ingame.team.generic.x				= 320;
			s_ingame.team.generic.y				= y;
			s_ingame.team.generic.id			= ID_JOINGAME;
			s_ingame.team.generic.callback		= InGame_Event;
			s_ingame.team.string				= "Join Game";
			s_ingame.team.color					= text_big_color;
			s_ingame.team.style					= UI_CENTER|UI_SMALLFONT;
		}
		else
		{
			s_ingame.team.generic.type			= MTYPE_PTEXT;
			s_ingame.team.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
			s_ingame.team.generic.x				= 320;
			s_ingame.team.generic.y				= y;
			s_ingame.team.generic.id			= ID_SPECTATE;
			s_ingame.team.generic.callback		= InGame_Event;
			s_ingame.team.string				= "Spectate";
			s_ingame.team.color					= text_big_color;
			s_ingame.team.style					= UI_CENTER|UI_SMALLFONT;

#ifdef TA_SP
			if (ui_singlePlayerActive.integer) {
				s_ingame.team.generic.flags |= QMF_GRAYED;
			}
#endif
		}
	}
#else
	s_ingame.team.generic.type			= MTYPE_PTEXT;
	s_ingame.team.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.team.generic.x				= 320;
	s_ingame.team.generic.y				= y;
	s_ingame.team.generic.id			= ID_TEAM;
	s_ingame.team.generic.callback		= InGame_Event; 
	s_ingame.team.string				= "START";
	s_ingame.team.color					= text_big_color;
	s_ingame.team.style					= UI_CENTER|UI_SMALLFONT;
#endif

#ifndef TA_MISC // INGAME_SERVER_MENU
	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.addbots.generic.type		= MTYPE_PTEXT;
	s_ingame.addbots.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.addbots.generic.x			= 320;
	s_ingame.addbots.generic.y			= y;
	s_ingame.addbots.generic.id			= ID_ADDBOTS;
	s_ingame.addbots.generic.callback	= InGame_Event; 
	s_ingame.addbots.string				= "ADD BOTS";
	s_ingame.addbots.color				= text_big_color;
	s_ingame.addbots.style				= UI_CENTER|UI_SMALLFONT;
	if( !trap_Cvar_VariableValue( "sv_running" ) || !trap_Cvar_VariableValue( "bot_enable" )
#ifdef TA_SP
	|| ui_singlePlayerActive.integer
#else
	|| (trap_Cvar_VariableValue( "g_gametype" ) == GT_SINGLE_PLAYER)
#endif
	) {
		s_ingame.addbots.generic.flags |= QMF_GRAYED;
	}

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.removebots.generic.type		= MTYPE_PTEXT;
	s_ingame.removebots.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.removebots.generic.x			= 320;
	s_ingame.removebots.generic.y			= y;
	s_ingame.removebots.generic.id			= ID_REMOVEBOTS;
	s_ingame.removebots.generic.callback	= InGame_Event; 
	s_ingame.removebots.string				= "REMOVE BOTS";
	s_ingame.removebots.color				= text_big_color;
	s_ingame.removebots.style				= UI_CENTER|UI_SMALLFONT;
	if( !trap_Cvar_VariableValue( "sv_running" ) || !trap_Cvar_VariableValue( "bot_enable" )
#ifdef TA_SP
	|| ui_singlePlayerActive.integer
#else
	|| (trap_Cvar_VariableValue( "g_gametype" ) == GT_SINGLE_PLAYER)
#endif
	) {
		s_ingame.removebots.generic.flags |= QMF_GRAYED;
	}
#endif

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.teamorders.generic.type		= MTYPE_PTEXT;
	s_ingame.teamorders.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.teamorders.generic.x			= 320;
	s_ingame.teamorders.generic.y			= y;
	s_ingame.teamorders.generic.id			= ID_TEAMORDERS;
	s_ingame.teamorders.generic.callback	= InGame_Event; 
#ifdef TA_MISC
	s_ingame.teamorders.string				= "Team Orders";
#else
	s_ingame.teamorders.string				= "TEAM ORDERS";
#endif
	s_ingame.teamorders.color				= text_big_color;
	s_ingame.teamorders.style				= UI_CENTER|UI_SMALLFONT;
	if( !(trap_Cvar_VariableValue( "g_gametype" ) >= GT_TEAM) ) {
		s_ingame.teamorders.generic.flags |= QMF_GRAYED;
	}
	else {
#ifndef TA_MISC // SMART_JOIN_MENU
		trap_GetClientState( &cs );
		trap_GetConfigString( CS_PLAYERS + cs.clientNum, info, MAX_INFO_STRING );
#endif
		team = atoi( Info_ValueForKey( info, "t" ) );
		if( team == TEAM_SPECTATOR ) {
			s_ingame.teamorders.generic.flags |= QMF_GRAYED;
		}
	}

#ifndef TA_MISC
	if (UI_MaxSplitView() > 1) {
		y += INGAME_MENU_VERTICAL_SPACING;
		s_ingame.localPlayers.generic.type		= MTYPE_PTEXT;
		s_ingame.localPlayers.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		s_ingame.localPlayers.generic.x			= 320;
		s_ingame.localPlayers.generic.y			= y;
		s_ingame.localPlayers.generic.id		= ID_LOCALPLAYERS;
		s_ingame.localPlayers.generic.callback	= InGame_Event;
#ifdef TA_MISC
		s_ingame.localPlayers.string			= "Local Players";
#else
		s_ingame.localPlayers.string			= "LOCAL PLAYERS";
#endif
		s_ingame.localPlayers.color				= text_big_color;
		s_ingame.localPlayers.style				= UI_CENTER|UI_SMALLFONT;

#ifdef TA_SP
		if (ui_singlePlayerActive.integer &&
			trap_Cvar_VariableValue( "g_gametype" ) != GT_SINGLE_PLAYER)
#else
		if (trap_Cvar_VariableValue( "g_gametype" ) == GT_SINGLE_PLAYER)
#endif
		{
			s_ingame.localPlayers.generic.flags |= QMF_GRAYED;
		}
	}
#endif

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.setup.generic.type			= MTYPE_PTEXT;
	s_ingame.setup.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.setup.generic.x			= 320;
	s_ingame.setup.generic.y			= y;
	s_ingame.setup.generic.id			= ID_SETUP;
	s_ingame.setup.generic.callback		= InGame_Event; 
#ifdef TA_SP // New menus
	s_ingame.setup.string				= "Options";
#else
	s_ingame.setup.string				= "SETUP";
#endif
	s_ingame.setup.color				= text_big_color;
	s_ingame.setup.style				= UI_CENTER|UI_SMALLFONT;

#ifdef TA_MISC
	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.setupplayers.generic.type		= MTYPE_PTEXT;
	s_ingame.setupplayers.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.setupplayers.generic.x			= 320;
	s_ingame.setupplayers.generic.y			= y;
	s_ingame.setupplayers.generic.id		= ID_CUSTOMIZEPLAYER;
	s_ingame.setupplayers.generic.callback	= InGame_Event; 
	s_ingame.setupplayers.string			= "Setup Players";
	s_ingame.setupplayers.color				= text_big_color;
	s_ingame.setupplayers.style				= UI_CENTER|UI_SMALLFONT;

	if (ui_singlePlayerActive.integer) {
		s_ingame.setupplayers.generic.flags |= QMF_GRAYED;
	}
#endif

#ifdef TA_MISC // INGAME_SERVER_MENU
	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.inserver.generic.type		= MTYPE_PTEXT;
	s_ingame.inserver.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.inserver.generic.x			= 320;
	s_ingame.inserver.generic.y			= y;
	s_ingame.inserver.generic.id		= ID_SERVER;
	s_ingame.inserver.generic.callback	= InGame_Event; 
	s_ingame.inserver.string			= "Server Options";
	s_ingame.inserver.color				= text_big_color;
	s_ingame.inserver.style				= UI_CENTER|UI_SMALLFONT;
	if( !trap_Cvar_VariableValue( "sv_running" )
		|| ui_singlePlayerActive.integer) {
		s_ingame.inserver.generic.flags |= QMF_GRAYED;
	}
#endif

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.server.generic.type		= MTYPE_PTEXT;
	s_ingame.server.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.server.generic.x			= 320;
	s_ingame.server.generic.y			= y;
	s_ingame.server.generic.id			= ID_SERVERINFO;
	s_ingame.server.generic.callback	= InGame_Event; 
#ifdef TA_MISC
	s_ingame.server.string				= "Server Info";
#else
	s_ingame.server.string				= "SERVER INFO";
#endif
	s_ingame.server.color				= text_big_color;
	s_ingame.server.style				= UI_CENTER|UI_SMALLFONT;

#ifdef TA_SP
	if (ui_singlePlayerActive.integer) {
		s_ingame.server.generic.flags |= QMF_GRAYED;
	}
#endif

#ifndef TA_MISC // INGAME_SERVER_MENU
	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.restart.generic.type		= MTYPE_PTEXT;
	s_ingame.restart.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.restart.generic.x			= 320;
	s_ingame.restart.generic.y			= y;
	s_ingame.restart.generic.id			= ID_RESTART;
	s_ingame.restart.generic.callback	= InGame_Event; 
	s_ingame.restart.string				= "RESTART ARENA";
	s_ingame.restart.color				= text_big_color;
	s_ingame.restart.style				= UI_CENTER|UI_SMALLFONT;
	if( !trap_Cvar_VariableValue( "sv_running" ) ) {
		s_ingame.restart.generic.flags |= QMF_GRAYED;
	}
#endif

#ifndef TA_MISC
	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.resume.generic.type			= MTYPE_PTEXT;
	s_ingame.resume.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.resume.generic.x				= 320;
	s_ingame.resume.generic.y				= y;
	s_ingame.resume.generic.id				= ID_RESUME;
	s_ingame.resume.generic.callback		= InGame_Event; 
	s_ingame.resume.string					= "RESUME GAME";
	s_ingame.resume.color					= text_big_color;
	s_ingame.resume.style					= UI_CENTER|UI_SMALLFONT;
#endif

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.leave.generic.type			= MTYPE_PTEXT;
	s_ingame.leave.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.leave.generic.x			= 320;
	s_ingame.leave.generic.y			= y;
	s_ingame.leave.generic.id			= ID_LEAVEARENA;
	s_ingame.leave.generic.callback		= InGame_Event; 
#ifdef TA_MISC
	s_ingame.leave.string				= "Leave Game";
#else
	s_ingame.leave.string				= "LEAVE ARENA";
#endif
	s_ingame.leave.color				= text_big_color;
	s_ingame.leave.style				= UI_CENTER|UI_SMALLFONT;

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.quit.generic.type			= MTYPE_PTEXT;
	s_ingame.quit.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.quit.generic.x				= 320;
	s_ingame.quit.generic.y				= y;
	s_ingame.quit.generic.id			= ID_QUIT;
	s_ingame.quit.generic.callback		= InGame_Event; 
#ifdef TA_MISC
	s_ingame.quit.string				= "Exit";
#else
	s_ingame.quit.string				= "EXIT GAME";
#endif
	s_ingame.quit.color					= text_big_color;
	s_ingame.quit.style					= UI_CENTER|UI_SMALLFONT;

	Menu_AddItem( &s_ingame.menu, &s_ingame.frame );
#ifdef TA_MISC
	Menu_AddItem( &s_ingame.menu, &s_ingame.resume );
#endif
	Menu_AddItem( &s_ingame.menu, &s_ingame.team );
#ifndef TA_MISC // INGAME_SERVER_MENU
	Menu_AddItem( &s_ingame.menu, &s_ingame.addbots );
	Menu_AddItem( &s_ingame.menu, &s_ingame.removebots );
#endif
	Menu_AddItem( &s_ingame.menu, &s_ingame.teamorders );
#ifndef TA_MISC
	if (UI_MaxSplitView() > 1) {
		Menu_AddItem( &s_ingame.menu, &s_ingame.localPlayers);
	}
#endif
	Menu_AddItem( &s_ingame.menu, &s_ingame.setup );
#ifdef TA_MISC
	Menu_AddItem( &s_ingame.menu, &s_ingame.setupplayers );
#endif
#ifdef TA_MISC // INGAME_SERVER_MENU
	Menu_AddItem( &s_ingame.menu, &s_ingame.inserver );
#endif
	Menu_AddItem( &s_ingame.menu, &s_ingame.server );
#ifndef TA_MISC // INGAME_SERVER_MENU
	Menu_AddItem( &s_ingame.menu, &s_ingame.restart );
#endif
#ifndef TA_MISC
	Menu_AddItem( &s_ingame.menu, &s_ingame.resume );
#endif
	Menu_AddItem( &s_ingame.menu, &s_ingame.leave );
	Menu_AddItem( &s_ingame.menu, &s_ingame.quit );
}


/*
=================
InGame_Cache
=================
*/
void InGame_Cache( void ) {
	trap_R_RegisterShaderNoMip( INGAME_FRAME );
}


/*
=================
UI_InGameMenu
=================
*/
void UI_InGameMenu( void ) {
	// force as top level menu
	uis.menusp = 0;  

	// set menu cursor to a nice location
	uis.cursorx = 319;
	uis.cursory = 80;

	InGame_MenuInit();
	UI_PushMenu( &s_ingame.menu );
}
