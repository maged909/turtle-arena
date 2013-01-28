/*
===========================================================================
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
Copyright (C) 2010-2012 Zack Middleton

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
=============================================================================

MULTIPLAYER PLAYER / CHARACTER SELECT MENU

=============================================================================
*/

#include "ui_local.h"

#ifdef TA_MISC

#define ART_BACK					"menu/art/back_0"
#define ART_BACK_FOCUS				"menu/art/back_1"
#define ART_NEXT					"menu/art/next_0"
#define ART_NEXT_FOCUS				"menu/art/next_1"
#define ART_PLAY					"menu/art/play_0"
#define ART_PLAY_FOCUS				"menu/art/play_1"

#define ART_FX_BASE			"menu/art/fx_base"
#define ART_FX_BLUE			"menu/art/fx_blue"
#ifdef TA_DATA
#define ART_FX_MAGENTA		"menu/art/fx_magenta"
#else
#define ART_FX_MAGENTA		"menu/art/fx_cyan"
#endif
#define ART_FX_GREEN		"menu/art/fx_grn"
#define ART_FX_RED			"menu/art/fx_red"
#ifdef TA_DATA
#define ART_FX_CYAN			"menu/art/fx_cyan"
#else
#define ART_FX_CYAN			"menu/art/fx_teal"
#endif
#define ART_FX_WHITE		"menu/art/fx_white"
#define ART_FX_YELLOW		"menu/art/fx_yel"
#ifdef TA_DATA // MORE_COLOR_EFFECTS
#define ART_FX_ORANGE		"menu/art/fx_orange"
#define ART_FX_LIME			"menu/art/fx_lime"
#define ART_FX_VIVIDGREEN	"menu/art/fx_vividgreen"
#define ART_FX_LIGHTBLUE	"menu/art/fx_lightblue"
#define ART_FX_PURPLE		"menu/art/fx_purple"
#define ART_FX_PINK			"menu/art/fx_pink"

#define NUM_COLOR_EFFECTS 13
#else
#define NUM_COLOR_EFFECTS 7
#endif

#define ID_BACK						10
#define ID_PLAY						11
#define ID_CLIENT0					12
#define ID_CLIENT0_ENABLED			(ID_CLIENT0+MAX_SPLITVIEW)

#define MAX_NAMELENGTH	20

#define MAX_PLAYERICONS 256
#define MAX_PLAYERMODELS 64
#define	MAX_PLAYERSKINS	8

// Player#
char *pnum[MAX_SPLITVIEW] = { "P1", "P2", "P3", "P4" };

typedef struct {
	char	name[32];
	int		numskins;
	char	skins[MAX_PLAYERSKINS][16];
} modelinfo_t;

typedef struct {
	menuframework_s	menu;

	menutext_s		art_banner;

	menutext_s			item_p1;
	menuradiobutton_s	clientEnabled[MAX_SPLITVIEW-1];
	menubitmap_s		clientCharacter[MAX_SPLITVIEW];

	//menulist_s		clientSkin;

	menufield_s			name[MAX_SPLITVIEW];
	menulist_s			handicap[MAX_SPLITVIEW];
	menulist_s			effects[MAX_SPLITVIEW];
#ifdef IOQ3ZTM // UI_COLOR2
	menulist_s			effects2[MAX_SPLITVIEW];
#endif

	menubitmap_s	back;
	menubitmap_s	play;

	qhandle_t			fxBasePic;
	qhandle_t			fxPic[NUM_COLOR_EFFECTS];

	void			(*action)(void);

	qboolean		default_clientEnabled[MAX_SPLITVIEW-1];

	int				nummodels;
	modelinfo_t		modelinfos[MAX_PLAYERMODELS];

	char			clientShaders[MAX_SPLITVIEW][MAX_QPATH];
	int				selectedModel[MAX_SPLITVIEW];
	int				selectedSkin[MAX_SPLITVIEW];
} playersetup_t;

static playersetup_t	s_playersetup;

#ifdef TA_DATA // MORE_COLOR_EFFECTS
static int gamecodetoui[NUM_COLOR_EFFECTS] = {8,4,6,0,10,2,12,1,3,5,7,9,11};
static int uitogamecode[NUM_COLOR_EFFECTS] = {4,8,6,9,2,10,3,11,1,12,5,13,7};
#else
static int gamecodetoui[NUM_COLOR_EFFECTS] = {4,2,3,0,5,1,6};
static int uitogamecode[NUM_COLOR_EFFECTS] = {4,6,2,3,1,5,7};
#endif

static const char *handicap_items[] = {
	"None",
	"95",
	"90",
	"85",
	"80",
	"75",
	"70",
	"65",
	"60",
	"55",
	"50",
	"45",
	"40",
	"35",
	"30",
	"25",
	"20",
	"15",
	"10",
	"5",
	NULL
};


/*
=================
PlayerSettings_DrawName
=================
*/
static void PlayerSettings_DrawName( void *self ) {
	menufield_s		*f;
	qboolean		focus;
	int				style;
	char			*txt;
	char			c;
	float			*color;
	int				n;
	int				basex, x, y;
	int				i;

	f = (menufield_s*)self;
	basex = f->generic.x;
	y = f->generic.y;
	focus = (f->generic.parent->cursor == f->generic.menuPosition);

	style = UI_LEFT|UI_SMALLFONT;
	color = text_color_normal;
	if ( f->generic.flags & QMF_GRAYED )
		color = text_color_disabled;
	else if( focus ) {
		style |= UI_PULSE;
		color = text_color_highlight;
	}

	// draw the actual name
	txt = f->field.buffer;
	if ( !(f->generic.flags & QMF_GRAYED) )
		color = g_color_table[ColorIndex(COLOR_WHITE)];
	x = basex;
	for (i = 0; i < strlen(f->field.buffer)+1; i++) {
		if ( (c = *txt) != 0 ) {
			if ( !focus && Q_IsColorString( txt ) ) {
				n = ColorIndex( *(txt+1) );
				if( n == 0 ) {
					n = 7;
				}
				if ( !(f->generic.flags & QMF_GRAYED) )
					color = g_color_table[n];
				txt += 2;
				continue;
			}
			x += UI_DrawChar( x, y, c, style, color );
			txt++;
		}
#ifdef IOQ3ZTM // FONT_REWITE
		// draw cursor if we have focus
		if( focus && f->field.cursor == i) {
			if ( trap_Key_GetOverstrikeMode() ) {
				c = 11;
			} else {
				c = 10;
			}

			UI_DrawChar( basex - Com_FontCharLeftOffset( UI_FontForStyle(style), c, 0 ), y, c, (style & ~UI_PULSE) | UI_BLINK, color_white );
		}
		basex = x;
#endif
	}

#ifndef IOQ3ZTM // FONT_REWITE
	// draw cursor if we have focus
	if( focus ) {
		if ( trap_Key_GetOverstrikeMode() ) {
			c = 11;
		} else {
			c = 10;
		}

		style &= ~UI_PULSE;
		style |= UI_BLINK;

		UI_DrawChar( basex + f->field.cursor * SMALLCHAR_WIDTH, y, c, style, color_white );
	}
#endif
}


/*
=================
PlayerSettings_DrawEffects
=================
*/
static void PlayerSettings_DrawEffects( void *self ) {
	menulist_s		*item;
	qboolean		focus;
	float			color[4];
	float			xOffset;

	item = (menulist_s *)self;
	focus = (item->generic.parent->cursor == item->generic.menuPosition);

	xOffset = 128.0f / (NUM_COLOR_EFFECTS + 1);

	if (item->generic.flags & QMF_GRAYED) {
		trap_R_SetColor( colorMdGrey );
	}

	UI_DrawHandlePic( item->generic.x, item->generic.y + 2, 128, 8, s_playersetup.fxBasePic );
	UI_DrawHandlePic( item->generic.x + item->curvalue * xOffset + xOffset * 0.5f, item->generic.y, 16, 12, s_playersetup.fxPic[item->curvalue] );

	trap_R_SetColor( NULL );

	if (focus) {
		if (item->curvalue == 1) {
			// In Turtle Arena, don't draw orange on orange. In Quake3 don't draw yellow on yellow.
			color[0] = color[1] = color[2] = 1.0f; // white
		} else {
			VectorCopy(text_color_highlight, color); // orange
		}

		color[3] = 0.5 + 0.5 * sin( uis.realtime / PULSE_DIVISOR );

		// UI_DrawRect, but width and height are 2.
		// top
		UI_FillRect( item->generic.x + item->curvalue * xOffset + xOffset * 0.5f, item->generic.y, 16, 2, color );
		// bottom
		UI_FillRect( item->generic.x + item->curvalue * xOffset + xOffset * 0.5f, item->generic.y + 10, 16, 2, color );
		// left
		UI_FillRect( item->generic.x + item->curvalue * xOffset + xOffset * 0.5f, item->generic.y + 2, 2, 8, color );
		// right
		UI_FillRect( item->generic.x + item->curvalue * xOffset + xOffset * 0.5f + 14, item->generic.y + 2, 2, 8, color );
	}
}

/*
=================
UI_SetCharacter
=================
*/
static void UI_SetCharacter(int localClientNum, int model, int skin ) {
	if (s_playersetup.selectedModel[localClientNum] == model
		&& s_playersetup.selectedSkin[localClientNum] == skin) {
		return;
	}

	s_playersetup.selectedModel[localClientNum] = model;
	s_playersetup.selectedSkin[localClientNum] = skin;

	trap_Cvar_Set( Com_LocalClientCvarName(localClientNum, "model"), s_playersetup.modelinfos[model].name );
	trap_Cvar_Set( Com_LocalClientCvarName(localClientNum, "headmodel"), "" );

	// Change shader and force loading it
	Com_sprintf(s_playersetup.clientShaders[localClientNum], sizeof (s_playersetup.clientShaders[localClientNum]),
				"models/players/%s/icon_%s", s_playersetup.modelinfos[model].name, s_playersetup.modelinfos[model].skins[skin]);
	s_playersetup.clientCharacter[localClientNum].shader = 0;
	s_playersetup.clientCharacter[localClientNum].focusshader = 0;
}

/*
=================
Character_Key
=================
*/
static sfxHandle_t Character_Key( menubitmap_s *s, int key )
{
	sfxHandle_t	sound;
	int			localClientNum;
	int			curvalue;
	int			numitems;

	localClientNum = s->generic.id - ID_CLIENT0;
	curvalue = s_playersetup.selectedModel[localClientNum];
	numitems = s_playersetup.nummodels;

#ifdef TA_MISC // MENU: Right Mouse button = left arrow
	if (key == K_MOUSE2 && !(s->generic.flags & QMF_HASMOUSEFOCUS)) {
		return 0;
	}
#endif

	sound = 0;
	switch (key)
	{
		case K_KP_RIGHTARROW:
		case K_RIGHTARROW:
			curvalue++;
			if (curvalue >= numitems)
				curvalue = 0;
			sound = menu_move_sound;
			break;

		case K_KP_LEFTARROW:
		case K_LEFTARROW:
#ifdef TA_MISC // MENU: Right Mouse button = left arrow
		case K_MOUSE2:
#endif
			curvalue--;
			if (curvalue < 0)
				curvalue = numitems-1;
			sound = menu_move_sound;
			break;
	}

	if (sound) {
		UI_SetCharacter(localClientNum, curvalue, 0);
	}

	return (sound);
}

/*
=================
UI_PlayerSetupMenu_CharacterEvent
=================
*/
static void UI_PlayerSetupMenu_CharacterEvent( void *ptr, int notification ) {
	if (notification != QM_ACTIVATED)
		return;

	Character_Key((menubitmap_s*)ptr, K_RIGHTARROW);
}

/*
=================
UI_PlayerSetupMenu_PlayerToggleEvent
=================
*/
static void UI_PlayerSetupMenu_PlayerToggleEvent( void *ptr, int notification ) {
	int		id;
	int		localClientNum;

	if (notification != QM_ACTIVATED)
		return;

	id = ((menucommon_s*)ptr)->id;
	localClientNum = id - ID_CLIENT0_ENABLED;

	if (((menuradiobutton_s*)ptr)->curvalue) {
		s_playersetup.clientCharacter[localClientNum].generic.flags &= ~QMF_GRAYED;
		s_playersetup.name[localClientNum].generic.flags &= ~QMF_GRAYED;
		s_playersetup.handicap[localClientNum].generic.flags &= ~QMF_GRAYED;
		s_playersetup.effects[localClientNum].generic.flags &= ~QMF_GRAYED;
		s_playersetup.effects2[localClientNum].generic.flags &= ~QMF_GRAYED;
	} else {
		s_playersetup.clientCharacter[localClientNum].generic.flags |= QMF_GRAYED;
		s_playersetup.name[localClientNum].generic.flags |= QMF_GRAYED;
		s_playersetup.handicap[localClientNum].generic.flags |= QMF_GRAYED;
		s_playersetup.effects[localClientNum].generic.flags |= QMF_GRAYED;
		s_playersetup.effects2[localClientNum].generic.flags |= QMF_GRAYED;
	}
}

/*
=================
UI_PlayerSetupMenu_SaveChanges
=================
*/
static void UI_PlayerSetupMenu_SaveChanges( void ) {
	int i;
	qboolean inGame;

	inGame = trap_Cvar_VariableValue( "cl_paused" );

	for (i = 0; i < MAX_SPLITVIEW; ++i) {
		// name
		trap_Cvar_Set( Com_LocalClientCvarName(i, "name"), s_playersetup.name[i].field.buffer );

		// handicap
		trap_Cvar_SetValue( Com_LocalClientCvarName(i, "handicap"),
				100 - s_playersetup.handicap[i].curvalue * 5 );

		// effects color
		trap_Cvar_SetValue( Com_LocalClientCvarName(i, "color1"),
				uitogamecode[s_playersetup.effects[i].curvalue] );
#ifdef IOQ3ZTM // UI_COLOR2
		trap_Cvar_SetValue( Com_LocalClientCvarName(i, "color2"),
				uitogamecode[s_playersetup.effects2[i].curvalue] );
#endif

		// When ingame and player enable changes call dropin and dropout as needed.
		if (i > 0 && inGame && s_playersetup.default_clientEnabled[i-1] != s_playersetup.clientEnabled[i-1].curvalue) {
			if (s_playersetup.clientEnabled[i-1].curvalue) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va("%s\n", Com_LocalClientCvarName(i, "dropin")) );
			} else {
				trap_Cmd_ExecuteText( EXEC_APPEND, va("%s\n", Com_LocalClientCvarName(i, "dropout")) );
			}
		}
	}
}

/*
=================
UI_PlayerSetupMenu_PlayEvent
=================
*/
static void UI_PlayerSetupMenu_PlayEvent( void *ptr, int notification ) {
	int i, localClients;

	if (notification != QM_ACTIVATED)
		return;

	// Set bits for enabled local clients.
	for (i = 1, localClients = 1; i < MAX_SPLITVIEW; ++i) {
		if (s_playersetup.clientEnabled[i-1].curvalue) {
			localClients |= (1<<i);
		}
	}

	trap_Cvar_SetValue( "cl_localClients", localClients );

	UI_PlayerSetupMenu_SaveChanges();

	if (s_playersetup.action) {
		s_playersetup.action();
	}
}


/*
=================
UI_PlayerSetupMenu_BackEvent
=================
*/
static void UI_PlayerSetupMenu_BackEvent( void* ptr, int notification ) {
	if (notification != QM_ACTIVATED) {
		return;
	}

	trap_Cvar_SetValue( "cl_localClients", 1 );
	UI_PlayerSetupMenu_SaveChanges();
	UI_PopMenu();
}


/*
=================
UI_PlayerSetupMenu_Key
=================
*/
static sfxHandle_t UI_PlayerSetupMenu_Key( int key ) {
	sfxHandle_t sound;
	void *ptr;
	int id;

	ptr = Menu_ItemAtCursor(&s_playersetup.menu);

	if (ptr) {
		id = ((menucommon_s*)ptr)->id;

		if (id >= ID_CLIENT0 && id < ID_CLIENT0+MAX_SPLITVIEW) {
			sound = Character_Key((menubitmap_s*)ptr, key);
	
			if (sound) {
				return sound;
			}
		}
	}

	if(
#ifndef TA_MISC // MENU: Right Mouse button = left arrow
	key == K_MOUSE2 ||
#endif
	key == K_ESCAPE ) {
		trap_Cvar_SetValue( "cl_localClients", 1 );
		UI_PlayerSetupMenu_SaveChanges();
	}

	return Menu_DefaultKey( &s_playersetup.menu, key );
}


/*
=================
UI_PlayerSetupMenu_BuildList
=================
*/
static void UI_PlayerSetupMenu_BuildList( void )
{
	int		numdirs;
	int		numfiles;
	char	dirlist[2048];
	char	filelist[2048];
	char	skinname[MAX_QPATH];
	char*	dirptr;
	char*	fileptr;
	int		i;
	int		j;
	int		dirlen;
	int		filelen;
	modelinfo_t	*modelinfo;

	s_playersetup.nummodels = 0;

	// iterate directory of all player models
	numdirs = trap_FS_GetFileList("models/players", "/", dirlist, 2048 );
	dirptr  = dirlist;
	for (i=0; i<numdirs && s_playersetup.nummodels < MAX_PLAYERMODELS; i++,dirptr+=dirlen+1)
	{
		dirlen = strlen(dirptr);
		
		if (dirlen && dirptr[dirlen-1]=='/') dirptr[dirlen-1]='\0';

		if (!strcmp(dirptr,".") || !strcmp(dirptr,".."))
			continue;

		modelinfo = NULL;

		// iterate all skin files in directory
		numfiles = trap_FS_GetFileList( va("models/players/%s",dirptr), "$images", filelist, 2048 );
		fileptr  = filelist;
		for (j=0; j<numfiles && s_playersetup.nummodels < MAX_PLAYERMODELS;j++,fileptr+=filelen+1)
		{
			filelen = strlen(fileptr);

			COM_StripExtension(fileptr,skinname, sizeof(skinname));

			// look for icon_????
			if (!Q_stricmpn(skinname,"icon_",5))
			{
				if (!modelinfo) {
					modelinfo = &s_playersetup.modelinfos[s_playersetup.nummodels];
					s_playersetup.nummodels++;

					Q_strncpyz(modelinfo->name, dirptr, sizeof (modelinfo->name));
				}

				if (!Q_stricmpn(skinname,"icon_default",12) && modelinfo->numskins) {
					// Make default the first skin
					Q_strncpyz(modelinfo->skins[modelinfo->numskins++], modelinfo->skins[0], sizeof (modelinfo->skins[0]));
					Q_strncpyz(modelinfo->skins[0], skinname+5, sizeof (modelinfo->skins[0]));
				} else {
					Q_strncpyz(modelinfo->skins[modelinfo->numskins++], skinname+5, sizeof (modelinfo->skins[0]));
				}

				if (modelinfo->numskins >= MAX_PLAYERMODELS) {
					break;
				}
			}
		}
	}

	//APSFIXME - Degenerate no models case
}

/*
=================
UI_PlayerSetupMenu_Init
=================
*/
static void UI_PlayerSetupMenu_Init( int maxLocalClients, void (*action)(void), qboolean playButton ) {
	int		i, j;
	int		x, y;
	int		c, h;
	int		spacing;
	int		leftOffset;
	char	playerModel[MAX_QPATH];
	char	*pdest;
	uiClientState_t	cs;
	qboolean inGame;

	if (maxLocalClients > MAX_SPLITVIEW) {
		maxLocalClients = MAX_SPLITVIEW;
	} else if (maxLocalClients <= 0) {
		maxLocalClients = 1;
	}

	// Setup equal spacing for the character select columns
	spacing = SCREEN_WIDTH / maxLocalClients;
	leftOffset = (spacing - 128) / 2;

	inGame = trap_Cvar_VariableValue( "cl_paused" );

	trap_GetClientState( &cs );

	memset( &s_playersetup, 0, sizeof(s_playersetup) );
	s_playersetup.action = action;

	s_playersetup.menu.fullscreen = qtrue;
	s_playersetup.menu.wrapAround = qtrue;
	s_playersetup.menu.key = UI_PlayerSetupMenu_Key;

	UI_PlayerSetupMenu_Cache();

	s_playersetup.art_banner.generic.type		= MTYPE_BTEXT;
	s_playersetup.art_banner.generic.flags		= QMF_CENTER_JUSTIFY;
	s_playersetup.art_banner.generic.x			= 320;
	s_playersetup.art_banner.generic.y			= 16;
	s_playersetup.art_banner.string				= "SETUP PLAYERS";
	s_playersetup.art_banner.color				= text_banner_color;
	s_playersetup.art_banner.style				= UI_CENTER;

	for (i = 0; i < MAX_SPLITVIEW; ++i) {
		if (i == 0) {
			// Main player is always enabled.
			s_playersetup.item_p1.generic.type	= MTYPE_TEXT;
			s_playersetup.item_p1.generic.flags	= QMF_LEFT_JUSTIFY;
			s_playersetup.item_p1.generic.x		= leftOffset+spacing*i+28;
			s_playersetup.item_p1.generic.y		= 112 - 32;
			s_playersetup.item_p1.string		= pnum[i];
			s_playersetup.item_p1.color			= text_big_color;
			s_playersetup.item_p1.style			= UI_LEFT;
		} else {
			s_playersetup.clientEnabled[i-1].generic.type		= MTYPE_RADIOBUTTON;
			s_playersetup.clientEnabled[i-1].generic.name		= pnum[i];
			s_playersetup.clientEnabled[i-1].generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
			s_playersetup.clientEnabled[i-1].generic.x			= leftOffset+spacing*i+28;
			s_playersetup.clientEnabled[i-1].generic.y			= 112 - 32;
			s_playersetup.clientEnabled[i-1].generic.callback	= UI_PlayerSetupMenu_PlayerToggleEvent;
			s_playersetup.clientEnabled[i-1].generic.id			= ID_CLIENT0_ENABLED+i;
			s_playersetup.clientEnabled[i-1].curvalue			= 1;
		}

		s_playersetup.clientCharacter[i].generic.type		= MTYPE_BITMAP;
		s_playersetup.clientCharacter[i].generic.name		= s_playersetup.clientShaders[i];
		s_playersetup.clientCharacter[i].generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
		s_playersetup.clientCharacter[i].generic.x			= leftOffset+spacing*i;
		s_playersetup.clientCharacter[i].generic.y			= 112;
		s_playersetup.clientCharacter[i].generic.callback	= UI_PlayerSetupMenu_CharacterEvent;
		s_playersetup.clientCharacter[i].generic.id			= ID_CLIENT0+i;
		s_playersetup.clientCharacter[i].width				= 128;
		s_playersetup.clientCharacter[i].height				= 128;
		s_playersetup.clientCharacter[i].focuspic			= s_playersetup.clientShaders[i];
		s_playersetup.clientCharacter[i].focuscolor			= text_color_highlight;

		// clientSkin

		y = 112+128+SMALLCHAR_HEIGHT*2;
		x = leftOffset+spacing*i;
		s_playersetup.name[i].generic.type			= MTYPE_FIELD;
		s_playersetup.name[i].generic.flags			= QMF_NODEFAULTINIT;
		s_playersetup.name[i].generic.ownerdraw		= PlayerSettings_DrawName;
		s_playersetup.name[i].field.widthInChars	= 8;//MAX_NAMELENGTH;
		s_playersetup.name[i].field.maxchars		= MAX_NAMELENGTH;
		s_playersetup.name[i].generic.x				= x;
		s_playersetup.name[i].generic.y				= y;
		s_playersetup.name[i].generic.left			= x;
		s_playersetup.name[i].generic.top			= y;
		s_playersetup.name[i].generic.right			= x + 128;
		s_playersetup.name[i].generic.bottom		= y + PROP_HEIGHT;

		y += SMALLCHAR_HEIGHT+2;
		s_playersetup.handicap[i].generic.type		= MTYPE_SPINCONTROL;
		s_playersetup.handicap[i].generic.name		= "Handicap";
		s_playersetup.handicap[i].generic.flags		= QMF_LEFT_JUSTIFY|QMF_SMALLFONT;
		s_playersetup.handicap[i].generic.x			= x;
		s_playersetup.handicap[i].generic.y			= y;
		s_playersetup.handicap[i].itemnames			= handicap_items;

		y += SMALLCHAR_HEIGHT+4;
		s_playersetup.effects[i].generic.type		= MTYPE_SPINCONTROL;
		s_playersetup.effects[i].generic.flags		= QMF_NODEFAULTINIT;
		s_playersetup.effects[i].generic.ownerdraw	= PlayerSettings_DrawEffects;
		s_playersetup.effects[i].generic.x			= x;
		s_playersetup.effects[i].generic.y			= y;
		s_playersetup.effects[i].generic.left		= x;
		s_playersetup.effects[i].generic.top		= y;
		s_playersetup.effects[i].generic.right		= x + 128;
		s_playersetup.effects[i].generic.bottom		= y + 12;
		s_playersetup.effects[i].numitems			= NUM_COLOR_EFFECTS;

#ifdef IOQ3ZTM // UI_COLOR2
		y += 14;
		s_playersetup.effects2[i].generic.type		= MTYPE_SPINCONTROL;
		s_playersetup.effects2[i].generic.flags		= QMF_NODEFAULTINIT;
		s_playersetup.effects2[i].generic.ownerdraw	= PlayerSettings_DrawEffects;
		s_playersetup.effects2[i].generic.x			= x;
		s_playersetup.effects2[i].generic.y			= y;
		s_playersetup.effects2[i].generic.left		= x;
		s_playersetup.effects2[i].generic.top		= y;
		s_playersetup.effects2[i].generic.right		= x + 128;
		s_playersetup.effects2[i].generic.bottom	= y + 12;
		s_playersetup.effects2[i].numitems			= NUM_COLOR_EFFECTS;
#endif

		if (i > 0) {
			// Extra players default to disabled in main menu.
			s_playersetup.default_clientEnabled[i-1] = (inGame && cs.clientNums[i] != -1);

			if (!s_playersetup.default_clientEnabled[i-1]) {
				s_playersetup.clientEnabled[i-1].curvalue = 0;
				s_playersetup.clientCharacter[i].generic.flags |= QMF_GRAYED;
				s_playersetup.name[i].generic.flags |= QMF_GRAYED;
				s_playersetup.handicap[i].generic.flags |= QMF_GRAYED;
				s_playersetup.effects[i].generic.flags |= QMF_GRAYED;
				s_playersetup.effects2[i].generic.flags |= QMF_GRAYED;
			}
		}
	}

	s_playersetup.back.generic.type			= MTYPE_BITMAP;
	s_playersetup.back.generic.name			= ART_BACK;
	s_playersetup.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersetup.back.generic.x			= 0;
	s_playersetup.back.generic.y			= 480-64;
	s_playersetup.back.generic.callback		= UI_PlayerSetupMenu_BackEvent;
	s_playersetup.back.generic.id			= ID_BACK;
	s_playersetup.back.width				= 128;
	s_playersetup.back.height				= 64;
	s_playersetup.back.focuspic				= ART_BACK_FOCUS;

	if (s_playersetup.action) {
		s_playersetup.play.generic.type			= MTYPE_BITMAP;
		s_playersetup.play.generic.name			= ART_PLAY;
		s_playersetup.play.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
		s_playersetup.play.generic.callback		= UI_PlayerSetupMenu_PlayEvent;
		s_playersetup.play.generic.id			= ID_PLAY;
		s_playersetup.play.generic.x			= 640;
		s_playersetup.play.generic.y			= 480-64;
		s_playersetup.play.width				= 128;
		s_playersetup.play.height				= 64;
		s_playersetup.play.focuspic				= ART_PLAY_FOCUS;

		if (!playButton) {
			s_playersetup.play.generic.name			= ART_NEXT;
			s_playersetup.play.focuspic				= ART_NEXT_FOCUS;
		}
	}

	Menu_AddItem( &s_playersetup.menu, ( void * )&s_playersetup.art_banner );
	Menu_AddItem( &s_playersetup.menu, ( void * )&s_playersetup.back );

	for (i = 0; i < maxLocalClients; ++i) {
		if (i == 0) {
			Menu_AddItem( &s_playersetup.menu, ( void * )&s_playersetup.item_p1 );
		} else {
			Menu_AddItem( &s_playersetup.menu, ( void * )&s_playersetup.clientEnabled[i-1] );
		}
		Menu_AddItem( &s_playersetup.menu, ( void * )&s_playersetup.clientCharacter[i] );

		Menu_AddItem( &s_playersetup.menu, ( void * )&s_playersetup.name[i] );
		Menu_AddItem( &s_playersetup.menu, ( void * )&s_playersetup.handicap[i] );
		Menu_AddItem( &s_playersetup.menu, ( void * )&s_playersetup.effects[i] );
		Menu_AddItem( &s_playersetup.menu, ( void * )&s_playersetup.effects2[i] );
	}

	if (s_playersetup.action) {
		Menu_AddItem( &s_playersetup.menu, ( void * )&s_playersetup.play );
	}

	// Select characters
	for (i = 0; i < MAX_SPLITVIEW; ++i) {
		trap_Cvar_VariableStringBuffer(Com_LocalClientCvarName(i, "model"), playerModel, sizeof(playerModel));

		// Remove skin
		pdest = strchr(playerModel, '/');
		if (pdest) {
			*pdest = '\0';
		}

		s_playersetup.selectedModel[i] = -1;
		s_playersetup.selectedSkin[i] = -1;

		for (j = 0; j < s_playersetup.nummodels; ++j) {
			if (Q_stricmp(s_playersetup.modelinfos[j].name, playerModel) == 0) {
				UI_SetCharacter(i, j, 0);
				break;
			}
		}

		// Didn't find character.
		if (j == s_playersetup.nummodels) {
			UI_SetCharacter(i, i, 0);
		}

		// name
		Q_strncpyz( s_playersetup.name[i].field.buffer, UI_Cvar_VariableString(
				Com_LocalClientCvarName(i, "name")), sizeof(s_playersetup.name[i].field.buffer) );

		// handicap
		h = Com_Clamp( 5, 100, trap_Cvar_VariableValue(Com_LocalClientCvarName(i, "handicap")) );
		s_playersetup.handicap[i].curvalue = 20 - h / 5;

		// effects color
		c = trap_Cvar_VariableValue( Com_LocalClientCvarName(i, "color1") ) - 1;
		if( c < 0 || c > NUM_COLOR_EFFECTS-1 ) {
			c = NUM_COLOR_EFFECTS-1;
		}
		s_playersetup.effects[i].curvalue = gamecodetoui[c];

#ifdef IOQ3ZTM // UI_COLOR2
		c = trap_Cvar_VariableValue( Com_LocalClientCvarName(i, "color2") ) - 1;
		if( c < 0 || c > NUM_COLOR_EFFECTS-1 ) {
			c = NUM_COLOR_EFFECTS-1;
		}
		s_playersetup.effects2[i].curvalue = gamecodetoui[c];
#endif
	}
}


/*
=================
UI_PlayerSetupMenu_Cache
=================
*/
void UI_PlayerSetupMenu_Cache( void ) {
	int i, j;

	trap_R_RegisterShaderNoMip( ART_BACK );
	trap_R_RegisterShaderNoMip( ART_BACK_FOCUS );

	trap_R_RegisterShaderNoMip( ART_NEXT );
	trap_R_RegisterShaderNoMip( ART_NEXT_FOCUS );
	trap_R_RegisterShaderNoMip( ART_PLAY );
	trap_R_RegisterShaderNoMip( ART_PLAY_FOCUS );

	UI_PlayerSetupMenu_BuildList();
	for( i = 0; i < s_playersetup.nummodels; i++ ) {
		for ( j = 0; j < s_playersetup.modelinfos[i].numskins; j++ ) {
			trap_R_RegisterShaderNoMip( va("models/players/%s/icon_%s", s_playersetup.modelinfos[i].name,
					s_playersetup.modelinfos[i].skins[j]) );
		}
	}

	s_playersetup.fxBasePic = trap_R_RegisterShaderNoMip( ART_FX_BASE );
#ifdef TA_DATA // MORE_COLOR_EFFECTS
	s_playersetup.fxPic[0] = trap_R_RegisterShaderNoMip( ART_FX_RED );
	s_playersetup.fxPic[1] = trap_R_RegisterShaderNoMip( ART_FX_ORANGE );
	s_playersetup.fxPic[2] = trap_R_RegisterShaderNoMip( ART_FX_YELLOW );
	s_playersetup.fxPic[3] = trap_R_RegisterShaderNoMip( ART_FX_LIME );
	s_playersetup.fxPic[4] = trap_R_RegisterShaderNoMip( ART_FX_GREEN );
	s_playersetup.fxPic[5] = trap_R_RegisterShaderNoMip( ART_FX_VIVIDGREEN );
	s_playersetup.fxPic[6] = trap_R_RegisterShaderNoMip( ART_FX_CYAN );
	s_playersetup.fxPic[7] = trap_R_RegisterShaderNoMip( ART_FX_LIGHTBLUE );
	s_playersetup.fxPic[8] = trap_R_RegisterShaderNoMip( ART_FX_BLUE );
	s_playersetup.fxPic[9] = trap_R_RegisterShaderNoMip( ART_FX_PURPLE );
	s_playersetup.fxPic[10] = trap_R_RegisterShaderNoMip( ART_FX_MAGENTA );
	s_playersetup.fxPic[11] = trap_R_RegisterShaderNoMip( ART_FX_PINK );
	s_playersetup.fxPic[12] = trap_R_RegisterShaderNoMip( ART_FX_WHITE );
#else
	s_playersetup.fxPic[0] = trap_R_RegisterShaderNoMip( ART_FX_RED );
	s_playersetup.fxPic[1] = trap_R_RegisterShaderNoMip( ART_FX_YELLOW );
	s_playersetup.fxPic[2] = trap_R_RegisterShaderNoMip( ART_FX_GREEN );
	s_playersetup.fxPic[3] = trap_R_RegisterShaderNoMip( ART_FX_CYAN );
	s_playersetup.fxPic[4] = trap_R_RegisterShaderNoMip( ART_FX_BLUE );
	s_playersetup.fxPic[5] = trap_R_RegisterShaderNoMip( ART_FX_MAGENTA );
	s_playersetup.fxPic[6] = trap_R_RegisterShaderNoMip( ART_FX_WHITE );
#endif
}

void UI_PlayerSetupMenu( int maxLocalClients, void (*action)(void), qboolean playButton ) {
	UI_PlayerSetupMenu_Init(maxLocalClients, action, playButton);
	UI_PushMenu( &s_playersetup.menu );
	if (s_playersetup.action) {
		Menu_SetCursorToItem( &s_playersetup.menu, &s_playersetup.play );
	}
}

#endif // TURTLEARENA

