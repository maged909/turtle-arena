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
#include "ui_local.h"

/*********************************************************************************
	SPECIFY SERVER
*********************************************************************************/

#define SPECIFYSERVER_FRAMEL	"menu/art/frame2_l"
#define SPECIFYSERVER_FRAMER	"menu/art/frame1_r"
#define SPECIFYSERVER_BACK0		"menu/art/back_0"
#define SPECIFYSERVER_BACK1		"menu/art/back_1"
#ifdef TA_DATA // NO_MENU_FIGHT
#define SPECIFYSERVER_FIGHT0	"menu/art/join_0"
#define SPECIFYSERVER_FIGHT1	"menu/art/join_1"
#else
#define SPECIFYSERVER_FIGHT0	"menu/art/fight_0"
#define SPECIFYSERVER_FIGHT1	"menu/art/fight_1"
#endif
#ifdef IOQ3ZTM // SPECIFY_FAV
#define SPECIFYSERVER_ADD0		"menu/art/accept_0"
#define SPECIFYSERVER_ADD1		"menu/art/accept_1"
#endif


#define ID_SPECIFYSERVERBACK	102
#define ID_SPECIFYSERVERGO		103
#ifdef IOQ3ZTM // SPECIFY_FAV
#define ID_SPECIFYSERVERADD		104

void ArenaServers_LoadFavorites( void );
void ArenaServers_StartRefresh( void );
#endif

static char* specifyserver_artlist[] =
{
	SPECIFYSERVER_FRAMEL,
	SPECIFYSERVER_FRAMER,
	SPECIFYSERVER_BACK0,	
	SPECIFYSERVER_BACK1,	
	SPECIFYSERVER_FIGHT0,
	SPECIFYSERVER_FIGHT1,
#ifdef IOQ3ZTM // SPECIFY_FAV
	SPECIFYSERVER_ADD0,
	SPECIFYSERVER_ADD1,
#endif
	NULL
};

typedef struct
{
	menuframework_s	menu;
	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;
	menufield_s		domain;
	menufield_s		port;
#ifdef IOQ3ZTM // SPECIFY_FAV
	menubitmap_s	add;
#endif
	menubitmap_s	go;
	menubitmap_s	back;
} specifyserver_t;

static specifyserver_t	s_specifyserver;
#ifdef IOQ3ZTM
static qboolean s_specifyserverFav;
#endif

/*
=================
SpecifyServer_Event
=================
*/
static void SpecifyServer_Event( void* ptr, int event )
{
	char	buff[256];

	switch (((menucommon_s*)ptr)->id)
	{
		case ID_SPECIFYSERVERGO:
			if (event != QM_ACTIVATED)
				break;

			if (s_specifyserver.domain.field.buffer[0])
			{
				strcpy(buff,s_specifyserver.domain.field.buffer);
				if (s_specifyserver.port.field.buffer[0])
					Com_sprintf( buff+strlen(buff), 128, ":%s", s_specifyserver.port.field.buffer );

				trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", buff ) );
			}
			break;

#ifdef IOQ3ZTM // SPECIFY_FAV
		case ID_SPECIFYSERVERADD:
			if (event != QM_ACTIVATED)
				break;

			if (s_specifyserver.domain.field.buffer[0])
			{
				strcpy(buff,s_specifyserver.domain.field.buffer);
				if (s_specifyserver.port.field.buffer[0])
					Com_sprintf( buff+strlen(buff), 128, ":%s", s_specifyserver.port.field.buffer );

				// From serverinfo
				{
					int		i;
					int		best;
					char adrstr[256];

					best = 0;
					for (i=0; i<MAX_FAVORITESERVERS; i++)
					{
						trap_Cvar_VariableStringBuffer( va("server%d",i+1), adrstr, sizeof(adrstr) );
						if (!Q_stricmp(buff,adrstr))
						{
							// Already in list
							UI_PopMenu();
							ArenaServers_StartRefresh();
							return;
						}

						// use first empty or non-numeric available slot
						if ((adrstr[0]  < '0' || adrstr[0] > '9' ) && !best)
							best = i+1;
					}

					if (best)
					{
						trap_Cvar_Set( va("server%d",best), buff);
						ArenaServers_LoadFavorites();
						UI_PopMenu();
						ArenaServers_StartRefresh();
					}
					else
					{
						trap_Cvar_Set( "com_errorMessage", "Can't add fav, all fav slots full.");
						UI_MainMenu();
						return;
					}
				}
			}
			break;
#endif

		case ID_SPECIFYSERVERBACK:
			if (event != QM_ACTIVATED)
				break;

			UI_PopMenu();
			break;
	}
}

/*
=================
SpecifyServer_MenuInit
=================
*/
void SpecifyServer_MenuInit( void )
{
	// zero set all our globals
	memset( &s_specifyserver, 0 ,sizeof(specifyserver_t) );

	SpecifyServer_Cache();

	s_specifyserver.menu.wrapAround = qtrue;
	s_specifyserver.menu.fullscreen = qtrue;

	s_specifyserver.banner.generic.type	 = MTYPE_BTEXT;
	s_specifyserver.banner.generic.x     = 320;
	s_specifyserver.banner.generic.y     = 16;
#ifdef IOQ3ZTM // SPECIFY_FAV
	if (s_specifyserverFav)
#ifdef TA_DATA
		s_specifyserver.banner.string		 = "NEW FAVORITE";
#else
		s_specifyserver.banner.string		 = "SPECIFY FAVORITE";
#endif
	else
#endif
	s_specifyserver.banner.string		 = "SPECIFY SERVER";
#ifdef IOQ3ZTM
	s_specifyserver.banner.color  		 = text_banner_color;
#else
	s_specifyserver.banner.color  		 = color_white;
#endif
	s_specifyserver.banner.style  		 = UI_CENTER;

	s_specifyserver.framel.generic.type  = MTYPE_BITMAP;
	s_specifyserver.framel.generic.name  = SPECIFYSERVER_FRAMEL;
	s_specifyserver.framel.generic.flags = QMF_INACTIVE;
	s_specifyserver.framel.generic.x	 = 0;  
	s_specifyserver.framel.generic.y	 = 78;
	s_specifyserver.framel.width  	     = 256;
	s_specifyserver.framel.height  	     = 329;

	s_specifyserver.framer.generic.type  = MTYPE_BITMAP;
	s_specifyserver.framer.generic.name  = SPECIFYSERVER_FRAMER;
	s_specifyserver.framer.generic.flags = QMF_INACTIVE;
	s_specifyserver.framer.generic.x	 = 376;
	s_specifyserver.framer.generic.y	 = 76;
	s_specifyserver.framer.width  	     = 256;
	s_specifyserver.framer.height  	     = 334;

	s_specifyserver.domain.generic.type       = MTYPE_FIELD;
	s_specifyserver.domain.generic.name       = "Address:";
	s_specifyserver.domain.generic.flags      = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_specifyserver.domain.generic.x	      = 206;
	s_specifyserver.domain.generic.y	      = 220;
	s_specifyserver.domain.field.widthInChars = 38;
	s_specifyserver.domain.field.maxchars     = 80;

	s_specifyserver.port.generic.type       = MTYPE_FIELD;
	s_specifyserver.port.generic.name	    = "Port:";
	s_specifyserver.port.generic.flags	    = QMF_PULSEIFFOCUS|QMF_SMALLFONT|QMF_NUMBERSONLY;
	s_specifyserver.port.generic.x	        = 206;
	s_specifyserver.port.generic.y	        = 250;
	s_specifyserver.port.field.widthInChars = 6;
	s_specifyserver.port.field.maxchars     = 5;

#ifdef TA_MISC // SPECIFY_FAV
	if (s_specifyserverFav)
	{
		s_specifyserver.go.generic.type	    = MTYPE_BITMAP;
		s_specifyserver.go.generic.name     = SPECIFYSERVER_ADD0;
		s_specifyserver.go.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
		s_specifyserver.go.generic.callback = SpecifyServer_Event;
		s_specifyserver.go.generic.id	    = ID_SPECIFYSERVERADD;
		s_specifyserver.go.generic.x		= 640;
		s_specifyserver.go.generic.y		= 480-64;
		s_specifyserver.go.width  		    = 128;
		s_specifyserver.go.height  		    = 64;
		s_specifyserver.go.focuspic         = SPECIFYSERVER_ADD1;
	}
	else
	{
		s_specifyserver.go.generic.type	    = MTYPE_BITMAP;
		s_specifyserver.go.generic.name     = SPECIFYSERVER_FIGHT0;
		s_specifyserver.go.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
		s_specifyserver.go.generic.callback = SpecifyServer_Event;
		s_specifyserver.go.generic.id	    = ID_SPECIFYSERVERGO;
		s_specifyserver.go.generic.x		= 640;
		s_specifyserver.go.generic.y		= 480-64;
		s_specifyserver.go.width  		    = 128;
		s_specifyserver.go.height  		    = 64;
		s_specifyserver.go.focuspic         = SPECIFYSERVER_FIGHT1;
	}
#elif defined IOQ3ZTM // SPECIFY_FAV
	s_specifyserver.add.generic.type	= MTYPE_BITMAP;
	s_specifyserver.add.generic.name	= SPECIFYSERVER_ADD0;
	s_specifyserver.add.generic.flags	= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_specifyserver.add.generic.callback= SpecifyServer_Event;
	s_specifyserver.add.generic.id		= ID_SPECIFYSERVERADD;
	s_specifyserver.add.generic.x		= 640;
	s_specifyserver.add.generic.y		= 480-64;
	s_specifyserver.add.width			= 128;
	s_specifyserver.add.height			= 64;
	s_specifyserver.add.focuspic		= SPECIFYSERVER_ADD1;

	s_specifyserver.go.generic.type	    = MTYPE_BITMAP;
	s_specifyserver.go.generic.name     = SPECIFYSERVER_FIGHT0;
	s_specifyserver.go.generic.flags    = QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_specifyserver.go.generic.callback = SpecifyServer_Event;
	s_specifyserver.go.generic.id	    = ID_SPECIFYSERVERGO;
	s_specifyserver.go.generic.x		= 320;
	s_specifyserver.go.generic.y		= 480-64;
	s_specifyserver.go.width  		    = 128;
	s_specifyserver.go.height  		    = 64;
	s_specifyserver.go.focuspic         = SPECIFYSERVER_FIGHT1;
#else
	s_specifyserver.go.generic.type	    = MTYPE_BITMAP;
	s_specifyserver.go.generic.name     = SPECIFYSERVER_FIGHT0;
	s_specifyserver.go.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_specifyserver.go.generic.callback = SpecifyServer_Event;
	s_specifyserver.go.generic.id	    = ID_SPECIFYSERVERGO;
	s_specifyserver.go.generic.x		= 640;
	s_specifyserver.go.generic.y		= 480-64;
	s_specifyserver.go.width  		    = 128;
	s_specifyserver.go.height  		    = 64;
	s_specifyserver.go.focuspic         = SPECIFYSERVER_FIGHT1;
#endif

	s_specifyserver.back.generic.type	  = MTYPE_BITMAP;
	s_specifyserver.back.generic.name     = SPECIFYSERVER_BACK0;
	s_specifyserver.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_specifyserver.back.generic.callback = SpecifyServer_Event;
	s_specifyserver.back.generic.id	      = ID_SPECIFYSERVERBACK;
	s_specifyserver.back.generic.x		  = 0;
	s_specifyserver.back.generic.y		  = 480-64;
	s_specifyserver.back.width  		  = 128;
	s_specifyserver.back.height  		  = 64;
	s_specifyserver.back.focuspic         = SPECIFYSERVER_BACK1;

	Menu_AddItem( &s_specifyserver.menu, &s_specifyserver.banner );
	Menu_AddItem( &s_specifyserver.menu, &s_specifyserver.framel );
	Menu_AddItem( &s_specifyserver.menu, &s_specifyserver.framer );
	Menu_AddItem( &s_specifyserver.menu, &s_specifyserver.domain );
	Menu_AddItem( &s_specifyserver.menu, &s_specifyserver.port );
#if defined IOQ3ZTM && !defined TA_MISC // SPECIFY_FAV
	Menu_AddItem( &s_specifyserver.menu, &s_specifyserver.add );
#endif
	Menu_AddItem( &s_specifyserver.menu, &s_specifyserver.go );
	Menu_AddItem( &s_specifyserver.menu, &s_specifyserver.back );

	// ZTM: NOTE: If PORT_SERVER is changed update this.
	Com_sprintf( s_specifyserver.port.field.buffer, 6, "%i", 27960 );
}

/*
=================
SpecifyServer_Cache
=================
*/
void SpecifyServer_Cache( void )
{
	int	i;

	// touch all our pics
	for (i=0; ;i++)
	{
		if (!specifyserver_artlist[i])
			break;
		trap_R_RegisterShaderNoMip(specifyserver_artlist[i]);
	}
}

/*
=================
UI_SpecifyServerMenu
=================
*/
#ifdef IOQ3ZTM // SPECIFY_FAV
void UI_SpecifyServerMenu( qboolean fav )
#else
void UI_SpecifyServerMenu( void )
#endif
{
#ifdef IOQ3ZTM // SPECIFY_FAV
	s_specifyserverFav = fav;
#endif
	SpecifyServer_MenuInit();
	UI_PushMenu( &s_specifyserver.menu );
}

