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
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

#if defined MISSIONPACK_HUD || defined IOQ3ZTM
#include "../ui/ui_shared.h"
#endif

#ifdef MISSIONPACK_HUD
// used for scoreboard
extern displayContextDef_t cgDC;
menuDef_t *menuScoreboard = NULL;
#else
int drawTeamOverlayModificationCount = -1;
#endif

#ifdef TURTLEARENA
#define HUD_X 18
#define HUD_Y 55 // letterbox view shouldn't overlap hud
#define HUD_WIDTH 172
#define HUD_HEIGHT (64 + 4 + ICON_SIZE)
#endif

int sortedTeamPlayers[TEAM_NUM_TEAMS][TEAM_MAXOVERLAY];
int	numSortedTeamPlayers[TEAM_NUM_TEAMS];
int sortedTeamPlayersTime[TEAM_NUM_TEAMS];

char systemChat[256];
char teamChat1[256];
char teamChat2[256];

#ifdef IOQ3ZTM // FONT_REWRITE
font_t *CG_FontForScale(float scale) {
	font_t *font = &cgs.media.fontSmall;
	if (scale <= cg_smallFont.value) {
		font = &cgs.media.fontTiny;
	} else if (scale > cg_bigFont.value) {
		font = &cgs.media.fontGiant;
	}

	return font;
}

int CG_Text_Width(const char *text, float scale, int limit) {
	return Com_FontStringWidthExt(CG_FontForScale(scale), text, scale, limit, qtrue);
}

int CG_Text_Height(const char *text, float scale, int limit) {
	return Com_FontStringHeightExt(CG_FontForScale(scale), text, scale, limit, qtrue);
}

void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
	font_t *font = CG_FontForScale(scale);
	int shadowofs;

	if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
		shadowofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
	} else {
		shadowofs = 0;
	}

	// y for this function is bottom of text line, not top...
	y -= CG_Text_Height(text, scale, 0);

	CG_DrawFontStringExt( font, scale, x, y, text, color, qfalse, qfalse, shadowofs, qtrue, adjust, limit, NULL );
}

void CG_Text_Paint_Limit(float *maxX, float x, float y, float scale, vec4_t color, const char* text, float adjust, int limit) {
	font_t *font = CG_FontForScale(scale);

	// y for this function is bottom of text line, not top...
	y -= CG_Text_Height(text, scale, limit);

	CG_DrawFontStringExt( font, scale, x, y, text, color, qfalse, qfalse, 0, qtrue, adjust, limit, maxX );
}
#elif defined MISSIONPACK_HUD
int CG_Text_Width(const char *text, float scale, int limit) {
	int count,len;
	float out;
	glyphInfo_t *glyph;
	float useScale;
// FIXME: see ui_main.c, same problem
//	const unsigned char *s = text;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
	out = 0;
	if (text) {
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
				out += glyph->xSkip;
				s++;
				count++;
			}
		}
	}
	return out * useScale;
}

int CG_Text_Height(const char *text, float scale, int limit) {
	int len, count;
	float max;
	glyphInfo_t *glyph;
	float useScale;
// TTimo: FIXME
//	const unsigned char *s = text;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
	max = 0;
	if (text) {
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
				if (max < glyph->height) {
					max = glyph->height;
				}
				s++;
				count++;
			}
		}
	}
	return max * useScale;
}

void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader) {
  float w, h;
  w = width * scale;
  h = height * scale;
  CG_AdjustFrom640( &x, &y, &w, &h );
  trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
  int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
	float useScale;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
	if (x == CENTER_X) {
		// center aligned
		float w;

		w = CG_Text_Width(text, scale, limit);
		x = (SCREEN_WIDTH - w) * 0.5f;
	} else if (x < 0) {
		// right aligned, offset by x
		float w;

		w = CG_Text_Width(text, scale, limit);
		x = SCREEN_WIDTH + x - w;
	}
  if (text) {
// TTimo: FIXME
//		const unsigned char *s = text;
		const char *s = text;
		trap_R_SetColor( color );
		memcpy(&newColor[0], &color[0], sizeof(vec4_t));
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
			//int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
			//float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					CG_Text_PaintChar(x + ofs, y - yadj + ofs, 
														glyph->imageWidth,
														glyph->imageHeight,
														useScale, 
														glyph->s,
														glyph->t,
														glyph->s2,
														glyph->t2,
														glyph->glyph);
					colorBlack[3] = 1.0;
					trap_R_SetColor( newColor );
				}
				CG_Text_PaintChar(x, y - yadj, 
													glyph->imageWidth,
													glyph->imageHeight,
													useScale, 
													glyph->s,
													glyph->t,
													glyph->s2,
													glyph->t2,
													glyph->glyph);
				// CG_DrawPic(x, y - yadj, scale * cgDC.Assets.textFont.glyphs[text[i]].imageWidth, scale * cgDC.Assets.textFont.glyphs[text[i]].imageHeight, cgDC.Assets.textFont.glyphs[text[i]].glyph);
				x += (glyph->xSkip * useScale) + adjust;
				s++;
				count++;
			}
		}
		trap_R_SetColor( NULL );
	}
}
#endif // MISSIONPACK_HUD

/*
==============
CG_DrawField

Draws large numbers for status bar and powerups
==============
*/
#ifndef MISSIONPACK_HUD
static void CG_DrawField (int x, int y, int width, int value) {
	char	num[16], *ptr;
	int		l;
	int		frame;

	if ( width < 1 ) {
		return;
	}

	// draw number string
	if ( width > 5 ) {
		width = 5;
	}

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += 2 + CHAR_WIDTH*(width - l);

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		CG_DrawPic( x,y, CHAR_WIDTH, CHAR_HEIGHT, cgs.media.numberShaders[frame] );
		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}
#endif // MISSIONPACK_HUD

/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, cgSkin_t *skin, vec3_t origin, vec3_t angles ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = CG_AddSkinToFrame( skin );
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	CG_AddRefEntityWithMinLight( &ent );
	trap_R_RenderScene( &refdef );
}

#ifdef IOQ3ZTM
/*
================
CG_Draw3DHeadModel
================
*/
void CG_Draw3DHeadModel( int clientNum, float x, float y, float w, float h, vec3_t origin, vec3_t angles )
{
	refdef_t		refdef;
	refEntity_t		ent;
	clientInfo_t	*ci;
	qhandle_t		model;
	cgSkin_t		*skin;

	ci = &cgs.clientinfo[ clientNum ];

	model = ci->headModel;
	skin = &ci->modelSkin;

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = CG_AddSkinToFrame( skin );
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
#ifdef IOQ3ZTM // ZTM: Show powerups on statusbar/scoreboard head models!
	CG_AddRefEntityWithPowerups( &ent, &cg_entities[clientNum].currentState );
#else
	trap_R_AddRefEntityToScene( &ent );
#endif
	trap_R_RenderScene( &refdef );
}
#endif

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles ) {
	clipHandle_t	cm;
	clientInfo_t	*ci;
	float			len;
	vec3_t			origin;
	vec3_t			mins, maxs;

	ci = &cgs.clientinfo[ clientNum ];

	if ( cg_draw3dIcons.integer ) {
		cm = ci->headModel;
		if ( !cm ) {
			return;
		}

		// offset the origin y and z to center the head
		trap_R_ModelBounds( cm, mins, maxs, 0, 0, 0 );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the head nearly fills the box
		// assume heads are taller than wide
		len = 0.7 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		// allow per-model tweaking
#ifdef TA_PLAYERSYS
		VectorAdd( origin, ci->playercfg.headOffset, origin );
#else
		VectorAdd( origin, ci->headOffset, origin );
#endif

#ifdef IOQ3ZTM
		CG_Draw3DHeadModel( clientNum, x, y, w, h, origin, headAngles );
#else
		CG_Draw3DModel( x, y, w, h, ci->headModel, &ci->modelSkin, origin, headAngles );
#endif
	} else if ( cg_drawIcons.integer ) {
		CG_DrawPic( x, y, w, h, ci->modelIcon );
	}

	// if they are deferred, draw a cross out
	if ( ci->deferred ) {
		CG_DrawPic( x, y, w, h, cgs.media.deferShader );
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D ) {
#ifdef TA_DATA // FLAG_MODEL
	gitem_t *item;
	int itemIndex;

	if( team == TEAM_RED ) {
		item = BG_FindItemForPowerup( PW_REDFLAG );
	} else if( team == TEAM_BLUE ) {
		item = BG_FindItemForPowerup( PW_BLUEFLAG );
	} else if( team == TEAM_FREE ) {
		item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
	} else {
		return;
	}
	if (!item) {
		return;
	}
	itemIndex = BG_ItemNumForItem(item);

	if ( !force2D && cg_draw3dIcons.integer )
	{
		float			len;
		vec3_t			origin, angles;
		vec3_t			mins, maxs;
		qhandle_t		model;
		cgSkin_t		*skin;

		VectorClear( angles );
		angles[YAW] = 60 * sin( cg.time / 2000.0 );

		model = cg_items[ itemIndex ].models[0];
		skin = &cg_items[ itemIndex ].skin;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds( model, mins, maxs, 0, 0, 0 );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * ( maxs[2] - mins[2] );
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		CG_Draw3DModel( x, y, w, h, model, skin, origin, angles );

		// Draw flag flap
		model = cg_items[ itemIndex ].models[1];

		CG_Draw3DModel( x, y, w, h, model, skin, origin, angles );
	}
	else
	{
		CG_DrawPic( x, y, w, h, cg_items[ itemIndex ].icon );
	}
#else
	qhandle_t		cm;
	float			len;
	vec3_t			origin, angles;
	vec3_t			mins, maxs;
	qhandle_t		handle;

	if ( !force2D && cg_draw3dIcons.integer ) {

		VectorClear( angles );

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds( cm, mins, maxs, 0, 0, 0 );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		angles[YAW] = 60 * sin( cg.time / 2000.0 );

		if( team == TEAM_RED ) {
			handle = cgs.media.redFlagModel;
		} else if( team == TEAM_BLUE ) {
			handle = cgs.media.blueFlagModel;
		} else if( team == TEAM_FREE ) {
			handle = cgs.media.neutralFlagModel;
		} else {
			return;
		}
		CG_Draw3DModel( x, y, w, h, handle, NULL, origin, angles );
	} else if ( cg_drawIcons.integer ) {
		gitem_t *item;

		if( team == TEAM_RED ) {
			item = BG_FindItemForPowerup( PW_REDFLAG );
		} else if( team == TEAM_BLUE ) {
			item = BG_FindItemForPowerup( PW_BLUEFLAG );
		} else if( team == TEAM_FREE ) {
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		} else {
			return;
		}
		if (item) {
		  CG_DrawPic( x, y, w, h, cg_items[ BG_ItemNumForItem( item ) ].icon );
		}
	}
#endif
}

/*
================
CG_DrawStatusBarHead

================
*/
#ifndef MISSIONPACK_HUD

static void CG_DrawStatusBarHead( float x ) {
	vec3_t		angles;
	float		size, stretch;
	float		frac;

	VectorClear( angles );

	if ( cg.cur_lc->damageTime && cg.time - cg.cur_lc->damageTime < DAMAGE_TIME ) {
		frac = (float)(cg.time - cg.cur_lc->damageTime ) / DAMAGE_TIME;
		size = ICON_SIZE * 1.25 * ( 1.5 - frac * 0.5 );

		stretch = size - ICON_SIZE * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.cur_lc->damageX * stretch * 0.5;

		cg.cur_lc->headStartYaw = 180 + cg.cur_lc->damageX * 45;

		cg.cur_lc->headEndYaw = 180 + 20 * cos( crandom()*M_PI );
		cg.cur_lc->headEndPitch = 5 * cos( crandom()*M_PI );

		cg.cur_lc->headStartTime = cg.time;
		cg.cur_lc->headEndTime = cg.time + 100 + random() * 2000;
	} else {
		if ( cg.time >= cg.cur_lc->headEndTime ) {
			// select a new head angle
			cg.cur_lc->headStartYaw = cg.cur_lc->headEndYaw;
			cg.cur_lc->headStartPitch = cg.cur_lc->headEndPitch;
			cg.cur_lc->headStartTime = cg.cur_lc->headEndTime;
			cg.cur_lc->headEndTime = cg.time + 100 + random() * 2000;

			cg.cur_lc->headEndYaw = 180 + 20 * cos( crandom()*M_PI );
			cg.cur_lc->headEndPitch = 5 * cos( crandom()*M_PI );
		}

		size = ICON_SIZE * 1.25;
	}

	// if the server was frozen for a while we may have a bad head start time
	if ( cg.cur_lc->headStartTime > cg.time ) {
		cg.cur_lc->headStartTime = cg.time;
	}

	frac = ( cg.time - cg.cur_lc->headStartTime ) / (float)( cg.cur_lc->headEndTime - cg.cur_lc->headStartTime );
	frac = frac * frac * ( 3 - 2 * frac );
	angles[YAW] = cg.cur_lc->headStartYaw + ( cg.cur_lc->headEndYaw - cg.cur_lc->headStartYaw ) * frac;
	angles[PITCH] = cg.cur_lc->headStartPitch + ( cg.cur_lc->headEndPitch - cg.cur_lc->headStartPitch ) * frac;

#ifdef TURTLEARENA
	CG_DrawHead( x,  (HUD_Y+(ICON_SIZE * 1.25)) - size, size, size,
				cg.cur_ps->clientNum, angles );
#else
	CG_DrawHead( x, 480 - size, size, size, 
				cg.cur_ps->clientNum, angles );
#endif
}
#endif // MISSIONPACK_HUD

/*
================
CG_DrawStatusBarFlag

================
*/
#ifndef MISSIONPACK_HUD
static void CG_DrawStatusBarFlag( float x, int team ) {
#ifdef TURTLEARENA
	CG_DrawFlagModel( x, HUD_Y, ICON_SIZE, ICON_SIZE, team, qfalse );
#else
	CG_DrawFlagModel( x, 480 - ICON_SIZE, ICON_SIZE, ICON_SIZE, team, qfalse );
#endif
}
#endif // MISSIONPACK_HUD

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team, int clientNum )
{
	vec4_t		hcolor;

	hcolor[3] = alpha;
	if ( team == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	} else if ( team == TEAM_BLUE ) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	} else {
		return;
	}
	trap_R_SetColor( hcolor );
	CG_SetScreenPlacement(PLACE_STRETCH, CG_GetScreenVerticalPlacement());
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	CG_PopScreenPlacement();
	trap_R_SetColor( NULL );
}

#ifdef TURTLEARENA
/*
================
CG_DrawFieldSmall
================
*/
void CG_DrawFieldSmall(int x, int y, int width, int value)
{
	char	num[16], *ptr;
	int		l;
	int		frame;
	int		char_width = CHAR_WIDTH/2;
	int		char_height = CHAR_HEIGHT/2;

	if ( width < 1 ) {
		return;
	}

	// draw number string
	if ( width > 6 ) {
		width = 6;
	}

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	case 5:
		value = value > 99999 ? 99999 : value;
		value = value < -9999 ? -9999 : value;
		break;
	case 6:
		value = value > 999999 ? 999999 : value;
		value = value < -99999 ? -99999 : value;
		break;
	}

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += 2 + (char_width*0.75f)*(width - l);

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		CG_DrawPic( x,y, char_width, char_height, cgs.media.numberShaders[frame] );
		x += (char_width*0.75f);
		ptr++;
		l--;
	}
}

/*
================
CG_GetHudColor

Returns qtrue if hcolor is set.
================
*/
qboolean CG_GetHudColor(vec4_t hcolor)
{
	int team;

	team = cg.cur_ps->persistant[PERS_TEAM];

	if ( team == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
		hcolor[3] = 1;
	} else if ( team == TEAM_BLUE ) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
		hcolor[3] = 1;
	} else {
		return qfalse;
	}

	return qtrue;
}

/*
================
CG_DrawHealthBar
================
*/
void CG_DrawHealthBar(int x, int y, int w, int h)
{
	vec4_t color_high = {1, 1, 1, 0.5f}; // white
	vec4_t color_normal = {1, 0.5f, 0, 1.0f}; // orange
	vec4_t color_normalflash = {1, 0.5f, 0, 0.5f}; // orange, half alpha
	vec4_t color_empty = {1.0f, 0.2f, 0.2f, 1.0f}; // red
	vec4_t color_hud;
	playerState_t *ps;
	int health, maxHealth;
	float healthFrac, healthExtraFrac;
	
	ps = cg.cur_ps;

	health = ps->stats[STAT_HEALTH];
	maxHealth = ps->stats[STAT_MAX_HEALTH];

	healthFrac = (health > maxHealth ? maxHealth : health) / (float)maxHealth;
	healthExtraFrac = (health > maxHealth ? health - maxHealth : 0) / (float)maxHealth;
	if (healthExtraFrac > 1.0f) {
		healthExtraFrac = 1.0f;
	}

	// Colorize hud background and healthbar
	if (CG_GetHudColor(color_hud)) {
		trap_R_SetColor(color_hud);
	}

	// Draw healthbar background
	CG_DrawPic( x, y, w, h, cgs.media.hudBarBackgroundShader );

	if (healthFrac > 0.25f) {
		trap_R_SetColor( color_normal );
	} else if ((cg.time >> 8) & 1) {
		trap_R_SetColor( color_normalflash );
	} else {
		trap_R_SetColor( color_normal );
	}

	// Draw healthbar for 0% to 100%
	CG_DrawPic( x+2, y+2, healthFrac * (w-4), h-4, cgs.media.hudBarShader );

	if (healthFrac < 1.0f) {
		trap_R_SetColor( color_empty );
		CG_DrawPic( x+2 + healthFrac * (w-4), y+2, (1.0f - healthFrac) * (w-4), h-4, cgs.media.hudBar2Shader );
	}
	// Draw extra healthbar for over 100%
	else if (healthExtraFrac > 0.0f) {
		trap_R_SetColor( color_high );
		CG_DrawPic( x+2, y+2, healthExtraFrac * (w-4), h-4, cgs.media.hudBarShader );
	}

	trap_R_SetColor( NULL );
}

/*
================
CG_DrawAirBar
================
*/
void CG_DrawAirBar( int x, int y, int w, int h )
{
	vec4_t		color_air = { 0.42f, 0.64f, 0.76f, 1.0f };
	vec4_t		color_empty = { 0.2f, 0.2f, 0.9f, 1.0f };
	vec4_t		color_hud = { 1.0f, 1.0f, 1.0f, 1.0f };
	const int	borderSize = 2;
	const int	fadeOutDelay = 1000;
	const int	fadeOutTime = 500;
	int value;
	float frac;
	float alpha;

	// Air time left
	value = cg.cur_ps->powerups[PW_AIR] - cg.time;

	// Disable airbar when not in deep water and it is full.
	if (cg.cur_lc->waterlevel <= 1 && value >= 30000) {
		if (!cg.cur_lc->airBarDrawn) {
			// First spawn in game, don't fade out because it hasn't been drawn yet!
			return;
		}

		if (!cg.cur_lc->airBarFadeTime) {
			cg.cur_lc->airBarFadeTime = cg.time;
		} else if (cg.cur_lc->airBarFadeTime && cg.cur_lc->airBarFadeTime + fadeOutDelay + fadeOutTime < cg.time) {
			cg.cur_lc->airBarDrawn = qfalse;
			return;
		}
	} else {
		cg.cur_lc->airBarDrawn = qtrue;
		cg.cur_lc->airBarFadeTime = 0;
	}

	if (cg.cur_lc->airBarFadeTime && cg.time > cg.cur_lc->airBarFadeTime + fadeOutDelay)
		alpha = 1.0f - ((cg.time - cg.cur_lc->airBarFadeTime - fadeOutDelay) / (float)fadeOutTime);
	else
		alpha = 1.0f;

	// Don't have less than zero air left
	if (value < 0) {
		value = 0;
	}

	if (value < 30000) {
		frac = value / 30000.0f;
	} else {
		frac = 1.0f;
	}

	// Colorize hud background
	CG_GetHudColor(color_hud);

	// Fade air bar
	color_air[3] = color_empty[3] = color_hud[3] = alpha;

	trap_R_SetColor(color_hud);

	// Draw healthbar background
	CG_DrawPic( x, y, w, h, cgs.media.hudBarBackgroundShader );

	trap_R_SetColor( color_air );
	CG_DrawPic( x+borderSize, y+borderSize, frac * (w-borderSize*2), h-borderSize*2, cgs.media.hudBarShader );

	if (frac < 1.0f) {
		trap_R_SetColor( color_empty );
		CG_DrawPic( x+borderSize + frac * (w-borderSize*2), y+borderSize,
					(1.0f - frac) * (w-borderSize*2), h-borderSize*2, cgs.media.hudBar2Shader );
	}

	trap_R_SetColor( NULL );
}
#endif

/*
================
CG_DrawStatusBar

================
*/
#ifndef MISSIONPACK_HUD
static void CG_DrawStatusBar( void ) {
	int			color;
	centity_t	*cent;
	playerState_t	*ps;
	int			value;
#ifdef TURTLEARENA
	int			start_x;
	int			x;
	int			y;
	vec4_t		color_hud;
#else
	vec4_t		hcolor;
	vec3_t		angles;
	vec3_t		origin;
#endif

	static float colors[4][4] = { 
//		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
		{ 1.0f, 0.69f, 0.0f, 1.0f },    // normal
		{ 1.0f, 0.2f, 0.2f, 1.0f },     // low health
		{ 0.5f, 0.5f, 0.5f, 1.0f },     // weapon firing
		{ 1.0f, 1.0f, 1.0f, 1.0f } };   // health > 100

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

#ifdef TURTLEARENA
	CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);

	ps = cg.cur_ps;
	cent = &cg_entities[ps->clientNum];

	start_x = x = HUD_X;
	y = HUD_Y;

	// Flag
	if( cg.cur_lc->predictedPlayerState.powerups[PW_REDFLAG] ) {
		CG_DrawStatusBarFlag( x + HUD_WIDTH + TEXT_ICON_SPACE, TEAM_RED );
	} else if( cg.cur_lc->predictedPlayerState.powerups[PW_BLUEFLAG] ) {
		CG_DrawStatusBarFlag( x + HUD_WIDTH + TEXT_ICON_SPACE, TEAM_BLUE );
	} else if( cg.cur_lc->predictedPlayerState.powerups[PW_NEUTRALFLAG] ) {
		CG_DrawStatusBarFlag( x + HUD_WIDTH + TEXT_ICON_SPACE, TEAM_FREE );
	}

#ifdef TA_SP
	// Lives (Single Player and Co-op only)
	if (cgs.gametype == GT_SINGLE_PLAYER) {
		value = ps->persistant[PERS_LIVES];
		if (value > 99) {
			value = 99;
		}

		CG_DrawSmallString( x + 68, y+64-22-12-22, va("x %d", value), 1.0F );
	}
#endif

	// Score
	CG_DrawFieldSmall(x + HUD_WIDTH - 78, y+64-22-12-CHAR_HEIGHT/2-4, 6, ps->persistant[PERS_SCORE]);

	// Colorize hud background
	if (CG_GetHudColor(color_hud)) {
		trap_R_SetColor(color_hud);
	}

	// Head background
	CG_DrawPic( x + 2, y, 64, 64, cgs.media.hudHeadBackgroundShader );

	// Make healthbar connect seemlessly
	CG_DrawPic( x + 34, y+64-22, 22, 22, cgs.media.hudBarBackgroundShader );

	trap_R_SetColor( NULL );

	// Air bar
	CG_DrawAirBar(x + 60, y+64-22-12, HUD_WIDTH-60-4, 12);

	// Health bar
	CG_DrawHealthBar(x + 56, y+64-22, (HUD_WIDTH-56) * ps->stats[STAT_MAX_HEALTH] / 100.0f, 22);

	// Draw head on top of everything else
	CG_DrawStatusBarHead( x );

	// Below healthbar
	y = HUD_Y + 64 + 4;
	x = start_x + 16;

	// Weapon icon and ammo
	if ( cent->currentState.weapon ) {
#ifdef TA_WEAPSYS_EX
		value = ps->stats[STAT_AMMO];
#else
		value = ps->ammo[cent->currentState.weapon];
#endif

		// Draw weapon icon
#ifdef TA_WEAPSYS
		if ( cg_weapongroups[cent->currentState.weapon].weaponIcon ) {
			CG_DrawPic( x, y, ICON_SIZE, ICON_SIZE,
				cg_weapongroups[cent->currentState.weapon].weaponIcon );
		}
#else
		if ( cg_weapons[cent->currentState.weapon].weaponIcon ) {
			CG_DrawPic( x, y, ICON_SIZE, ICON_SIZE,
				cg_weapons[cent->currentState.weapon].weaponIcon );
		}
#endif
		if (value == 0) {
			CG_DrawPic( x, y, ICON_SIZE, ICON_SIZE,
				cgs.media.noammoShader );
		}

		x += ICON_SIZE;

		// ammo
		if ( value > -1 ) {
			if ( cg.cur_lc->predictedPlayerState.weaponstate == WEAPON_FIRING
				&& cg.cur_lc->predictedPlayerState.weaponTime > 100 ) {
				// draw as dark grey when reloading
				color = 2;	// dark grey
			} else {
				if ( value >= 0 ) {
					color = 0;	// green
				} else {
					color = 1;	// red
				}
			}
			trap_R_SetColor( colors[color] );

			CG_DrawFieldSmall(x - CHAR_WIDTH/2 * 0.75f, y, 3, value);
			trap_R_SetColor( NULL );
		}

		x += (2 * (CHAR_WIDTH/2) * 0.75f) + 4;
	}

	// Holdable item and use count
#ifdef TA_HOLDSYS
	value = BG_ItemNumForHoldableNum(cg.cur_ps->holdableIndex);
#else
	value = cg.cur_ps->stats[STAT_HOLDABLE_ITEM];
#endif
	if ( value ) {
		CG_RegisterItemVisuals( value );
		if ( cg_items[ value ].icon ) {
			CG_DrawPic( x, y, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
		}
		x += ICON_SIZE;

#ifdef TA_HOLDSYS
		// draw value
		if (!bg_projectileinfo[BG_ProjectileIndexForHoldable(cg.cur_ps->holdableIndex)].grappling)
		{
			int giveQuantity = BG_ItemForItemNum(value)->quantity;
			int useCount = cg.cur_ps->holdable[cg.cur_ps->holdableIndex];

			if ((giveQuantity > 0 && useCount > 0)
				|| (giveQuantity == 0 && useCount > 1)) // Only happens with give command.
			{
				if (cg.cur_lc->predictedPlayerState.holdableTime > 100
#ifdef TA_ENTSYS // FUNC_USE
					&& !(cg.cur_lc->predictedPlayerState.eFlags & EF_USE_ENT)
#endif
					)
				{
					// draw as dark grey when reloading
					color = 2;	// dark grey
				} else {
					color = 0;	// green
				}
				trap_R_SetColor( colors[color] );

				CG_DrawFieldSmall(x, y, 2, useCount);
				trap_R_SetColor( NULL );
			}
		}

		//x += (2 * (CHAR_WIDTH/2) * 0.75f) + 4;
#endif
	}
#else
	CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);

	ps = cg.cur_ps;
	cent = &cg_entities[ps->clientNum];

	// draw the team background
	CG_DrawTeamBackground( 0, 420, 640, 60, 0.33f, ps->persistant[PERS_TEAM], ps->clientNum );

	VectorClear( angles );

	// draw any 3D icons first, so the changes back to 2D are minimized
#ifdef TA_WEAPSYS
	if ( cent->currentState.weapon && cg_weapongroups[ cent->currentState.weapon ].ammoModel )
#else
	if ( cent->currentState.weapon && cg_weapons[ cent->currentState.weapon ].ammoModel )
#endif
	{
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 90 + 20 * sin( cg.time / 1000.0 );
		CG_Draw3DModel( CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE,
#ifdef TA_WEAPSYS
					   cg_weapongroups[ cent->currentState.weapon ].ammoModel, NULL, origin, angles );
#else
					   cg_weapons[ cent->currentState.weapon ].ammoModel, NULL, origin, angles );
#endif
	}

	CG_DrawStatusBarHead( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE );

	if( cg.cur_lc->predictedPlayerState.powerups[PW_REDFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_RED );
	} else if( cg.cur_lc->predictedPlayerState.powerups[PW_BLUEFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_BLUE );
	} else if( cg.cur_lc->predictedPlayerState.powerups[PW_NEUTRALFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_FREE );
	}

	if ( ps->stats[ STAT_ARMOR ] ) {
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
		CG_Draw3DModel( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE,
					   cgs.media.armorModel, NULL, origin, angles );
	}
	//
	// ammo
	//
	if ( cent->currentState.weapon ) {
		value = ps->ammo[cent->currentState.weapon];
		if ( value > -1 ) {
			if ( cg.cur_lc->predictedPlayerState.weaponstate == WEAPON_FIRING
				&& cg.cur_lc->predictedPlayerState.weaponTime > 100 ) {
				// draw as dark grey when reloading
				color = 2;	// dark grey
			} else {
				if ( value >= 0 ) {
					color = 0;	// green
				} else {
					color = 1;	// red
				}
			}
			trap_R_SetColor( colors[color] );
			
			CG_DrawField (0, 432, 3, value);
			trap_R_SetColor( NULL );

			// if we didn't draw a 3D icon, draw a 2D icon for ammo
			if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
				qhandle_t	icon;

#ifdef TA_WEAPSYS
				icon = cg_weapongroups[ cg.cur_lc->predictedPlayerState.weapon ].ammoIcon;
#else
				icon = cg_weapons[ cg.cur_lc->predictedPlayerState.weapon ].ammoIcon;
#endif
				if ( icon ) {
					CG_DrawPic( CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, icon );
				}
			}
		}
	}

	//
	// health
	//
	value = ps->stats[STAT_HEALTH];
	if ( value > 100 ) {
		trap_R_SetColor( colors[3] );		// white
	} else if (value > 25) {
		trap_R_SetColor( colors[0] );	// green
	} else if (value > 0) {
		color = (cg.time >> 8) & 1;	// flash
		trap_R_SetColor( colors[color] );
	} else {
		trap_R_SetColor( colors[1] );	// red
	}

	// stretch the health up when taking damage
	CG_DrawField ( 185, 432, 3, value);
	CG_ColorForHealth( hcolor );
	trap_R_SetColor( hcolor );


	//
	// armor
	//
	value = ps->stats[STAT_ARMOR];
	if (value > 0 ) {
		trap_R_SetColor( colors[0] );
		CG_DrawField (370, 432, 3, value);
		trap_R_SetColor( NULL );
		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
			CG_DrawPic( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, 432, ICON_SIZE, ICON_SIZE, cgs.media.armorIcon );
		}

	}
#endif
}
#endif // MISSIONPACK_HUD

#if defined TA_WEAPSYS || defined IOQ3ZTM // SHOW_SPEED
/*
================
CG_DrawMiddleLeft
================
*/
void CG_DrawMiddleLeft(void)
{
	int y;

	CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);

	y = SCREEN_HEIGHT/2;

	if (cg_drawSpeed.integer) {
		CG_DrawSmallString( SCREEN_WIDTH/32, y, va("Speed %f", VectorLength(cg.cur_ps->velocity)), 1.0F );
		y += SMALLCHAR_HEIGHT;
	}

	// Index of the attack animation, not how many things we've hit.
	//if (cg.cur_ps->meleeAttack > 0) {
	//	CG_DrawSmallString( SCREEN_WIDTH/32, y, va("meleeAttack %d", cg.cur_ps->meleeAttack), 1.0F );
	//	y += SMALLCHAR_HEIGHT;
	//}
}
#endif

#ifdef TURTLEARENA // NIGHTS_ITEMS
/*
================
CG_DrawScoreChain

Draw NiGHTS Link (score chain) count
================
*/
void CG_DrawScoreChain(void)
{
	float	*fadeColor;
	float	frac;
	float	color[4];
	char	*s;
	int		y;

	if (cg.cur_ps->chain <= 1) {
		return;
	}

	CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);

	fadeColor = CG_FadeColor( cg.cur_lc->scorePickupTime, 2000 );
	if (!fadeColor) {
		return;
	}

	frac = fadeColor[3];

	CG_ColorForChain(cg.cur_ps->chain, color);
	color[3] = fadeColor[3];

	s = va("%d Link", cg.cur_ps->chain-1);

	y = SCREEN_HEIGHT - 32;
	CG_Text_Paint(CENTER_X, y, frac, color, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
}
#endif

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawAttacker

================
*/
static float CG_DrawAttacker( float y ) {
	int			t;
	float		size;
	vec3_t		angles;
	const char	*info;
	const char	*name;
	int			clientNum;

	if ( cg.cur_lc->predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	if ( !cg.cur_lc->attackerTime ) {
		return y;
	}

	clientNum = cg.cur_lc->predictedPlayerState.persistant[PERS_ATTACKER];
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.cur_ps->clientNum ) {
		return y;
	}

	t = cg.time - cg.cur_lc->attackerTime;
	if ( t > ATTACKER_HEAD_TIME ) {
		cg.cur_lc->attackerTime = 0;
		return y;
	}

	size = ICON_SIZE * 1.25;

	angles[PITCH] = 0;
	angles[YAW] = 180;
	angles[ROLL] = 0;
	CG_DrawHead( 640 - size, y, size, size, clientNum, angles );

	info = CG_ConfigString( CS_PLAYERS + clientNum );
	name = Info_ValueForKey(  info, "n" );
	y += size;
	CG_DrawBigString( -1, y, name, 0.5 );

	return y + BIGCHAR_HEIGHT + 2;
}

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char		*s;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime, 
		cg.latestSnapshotNum, cgs.serverCommandSequence );

	CG_DrawBigString( -5, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define	FPS_FRAMES	4
static float CG_DrawFPS( float y ) {
	char		*s;
	static int	previousTimes[FPS_FRAMES];
	static int	index;
	int		i, total;
	int		fps;
	static	int	previous;
	int		t, frameTime;

	if (cg.viewport != 0) {
		return y;
	}

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index > FPS_FRAMES ) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va( "%ifps", fps );

		CG_DrawBigString( -5, y + 2, s, 1.0F);
	}

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
	char		*s;
	int			mins, seconds, tens;
	int			msec;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%i:%i%i", mins, tens, seconds );

	CG_DrawBigString( -5, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

#ifdef TA_MISC // COMIC_ANNOUNCER
/*
=================
CG_DrawAnnouncements
=================
*/
int CG_DrawAnnouncements(int y)
{
	char	*s;
	int		i, j;

	for (j = 0, i = cg.cur_lc->announcement-1; j < MAX_ANNOUNCEMENTS; j++, i--)
	{
		if (i == -1) {
			i = MAX_ANNOUNCEMENTS-1;
		}

		if (cg.cur_lc->announcementTime[i] <= 0 || cg.cur_lc->announcementTime[i] + 10000 < cg.time) {
			break;
		}

		// Add announcement
		s = va( "Announcement: %s", cg.cur_lc->announcementMessage[i] );
		CG_DrawBigString( -5, y + 2, s, 1.0F);

		y += BIGCHAR_HEIGHT + 4;
	}

	return y;
}
#endif

/*
=================
CG_DrawTeamOverlay
=================
*/

static float CG_DrawTeamOverlay( float y, qboolean right, qboolean upper ) {
	int x, w, h, xx;
	int i, j, len;
	const char *p;
	vec4_t		hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	clientInfo_t *ci;
	gitem_t	*item;
	int ret_y, count;
	int team;

	if ( !cg_drawTeamOverlay.integer ) {
		return y;
	}

	team = cg.cur_ps->persistant[PERS_TEAM];

	if ( team != TEAM_RED && team != TEAM_BLUE ) {
		return y; // Not on any team
	}

	if (cg.time - sortedTeamPlayersTime[team] > 5000) {
		// Info is too out of date.
		return y;
	}

	plyrs = 0;

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers[team] > 8) ? 8 : numSortedTeamPlayers[team];
	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[team][i];
		if ( ci->infoValid && ci->team == team) {
			plyrs++;
#ifdef IOQ3ZTM // FONT_REWRITE
			len = Com_FontStringWidth(&cgs.media.fontTiny, ci->name, 0);
#else
			len = CG_DrawStrlen(ci->name);
#endif
			if (len > pwidth)
				pwidth = len;
		}
	}

	if (!plyrs)
		return y;

#ifdef IOQ3ZTM // FONT_REWRITE
	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH*TINYCHAR_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH*TINYCHAR_WIDTH;
#else
	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;
#endif

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && *p) {
#ifdef IOQ3ZTM // FONT_REWRITE
			len = Com_FontStringWidth(&cgs.media.fontTiny, p, 0);
#else
			len = CG_DrawStrlen(p);
#endif
			if (len > lwidth)
				lwidth = len;
		}
	}

#ifdef IOQ3ZTM // FONT_REWRITE
	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH*TINYCHAR_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH*TINYCHAR_WIDTH;

	w = pwidth + lwidth + (4 + 7) * TINYCHAR_WIDTH;
#else
	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;
#endif

	if ( right )
		x = 640 - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	if ( upper ) {
		ret_y = y + h;
	} else {
		y -= h;
		ret_y = y;
	}

	if ( team == TEAM_RED ) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f;
	} else { // if ( team == TEAM_BLUE )
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );

	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[team][i];
		if ( ci->infoValid && ci->team == team) {

			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			xx = x + TINYCHAR_WIDTH;

			CG_DrawStringExt( xx, y,
				ci->name, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH);

			if (lwidth) {
				p = CG_ConfigString(CS_LOCATIONS + ci->location);
				if (!p || !*p)
					p = "unknown";
//				len = CG_DrawStrlen(p);
//				if (len > lwidth)
//					len = lwidth;

#ifdef IOQ3ZTM // FONT_REWRITE
				xx = x + TINYCHAR_WIDTH * 2 + pwidth;
#else
//				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth + 
//					((lwidth/2 - len/2) * TINYCHAR_WIDTH);
				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
#endif
				CG_DrawStringExt( xx, y,
					p, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					TEAM_OVERLAY_MAXLOCATION_WIDTH);
			} else {
				p = "";
			}

#ifdef TURTLEARENA // NOARMOR
			CG_GetColorForHealth( ci->health, hcolor );

			Com_sprintf (st, sizeof(st), "%3i", ci->health);
#else
			CG_GetColorForHealth( ci->health, ci->armor, hcolor );

			Com_sprintf (st, sizeof(st), "%3i %3i", ci->health,	ci->armor);
#endif

#ifdef IOQ3ZTM // FONT_REWRITE
			xx = x + TINYCHAR_WIDTH * 3 + pwidth + lwidth;
#else
			xx = x + TINYCHAR_WIDTH * 3 + 
				TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;
#endif

			CG_DrawStringExt( xx, y,
				st, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3;

#ifdef TA_WEAPSYS
			if ( cg_weapongroups[ci->curWeapon].weaponIcon ) {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					cg_weapongroups[ci->curWeapon].weaponIcon );
			}
#else
			if ( cg_weapons[ci->curWeapon].weaponIcon ) {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
					cg_weapons[ci->curWeapon].weaponIcon );
			}
#endif
			else {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
					cgs.media.deferShader );
			}

			// Draw powerup icons
			if (right) {
				xx = x;
			} else {
				xx = x + w - TINYCHAR_WIDTH;
			}
			for (j = 0; j <= PW_NUM_POWERUPS; j++) {
				if (ci->powerups & (1 << j)) {

					item = BG_FindItemForPowerup( j );

					if (item) {
						CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
						trap_R_RegisterShader( item->icon ) );
						if (right) {
							xx -= TINYCHAR_WIDTH;
						} else {
							xx += TINYCHAR_WIDTH;
						}
					}
				}
			}

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
//#endif
}


/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight(stereoFrame_t stereoFrame)
{
	float	y;

	y = 0;

	CG_SetScreenPlacement(PLACE_RIGHT, PLACE_TOP);

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 1 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qtrue );
	} 
	if ( cg_drawSnapshot.integer ) {
		y = CG_DrawSnapshot( y );
	}
	if (cg_drawFPS.integer && (stereoFrame == STEREO_CENTER || stereoFrame == STEREO_RIGHT)) {
		y = CG_DrawFPS( y );
	}
	if ( cg_drawTimer.integer ) {
		y = CG_DrawTimer( y );
	}
#ifdef TA_MISC // COMIC_ANNOUNCER
	if ( cg_announcerText.integer ) {
		y = CG_DrawAnnouncements(y);
	}
#endif
	if ( cg_drawAttacker.integer ) {
		y = CG_DrawAttacker( y );
	}

}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
=================
CG_DrawScores

Draw the small two score display
=================
*/
#ifndef MISSIONPACK_HUD
static float CG_DrawScores( float y ) {
	const char	*s;
	int			s1, s2, score;
	int			x, w;
	int			h;
	vec4_t		color;
	float		y1;
	gitem_t		*item;
	const int	FLAG_SIZE = 64;

	if ( !cg_drawScores.integer ) {
		return y;
	}

	s1 = cgs.scores1;
	s2 = cgs.scores2;

	y -= BIGCHAR_HEIGHT + 8;
	y1 = y;

	// draw from the right side to left
	if ( cgs.gametype == GT_CTF ) {
		x = 640;

		// Blue
		s = va( "%2i", s2 );
		h = w = FLAG_SIZE;
		x -= w;

		// Display flag status
		item = BG_FindItemForPowerup( PW_BLUEFLAG );

		y1 = y + BIGCHAR_HEIGHT + 8 - h;

		if (item) {
			if( cgs.blueflag >= 0 && cgs.blueflag <= 2 ) {
				CG_DrawPic( x, y1, w, h, cgs.media.blueFlagShader[cgs.blueflag] );
			}
		}

		CG_DrawBigString( x + (w - Com_FontStringWidth(&cgs.media.fontBig, s, 0)) / 2, y, s, 1.0F);

		// Red
		s = va( "%2i", s1 );
		h = w = FLAG_SIZE;
		x -= w;

		// Display flag status
		item = BG_FindItemForPowerup( PW_REDFLAG );

		y1 = y + BIGCHAR_HEIGHT + 8 - h;

		if (item) {
			if( cgs.redflag >= 0 && cgs.redflag <= 2 ) {
				CG_DrawPic( x, y1, w, h, cgs.media.redFlagShader[cgs.redflag] );
			}
		}

		CG_DrawBigString( x + (w - Com_FontStringWidth(&cgs.media.fontBig, s, 0)) / 2, y, s, 1.0F);
	} else if ( cgs.gametype >= GT_TEAM ) {
		x = 640;

#ifdef MISSIONPACK
		if ( cgs.gametype == GT_1FCTF ) {
			// Display flag status
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );

			if (item) {
				h = w = FLAG_SIZE;
				x -= w;
				y1 = y + BIGCHAR_HEIGHT + 8 - h;
				if( cgs.flagStatus >= 0 && cgs.flagStatus <= 4 ) {
					vec4_t color = {1, 1, 1, 1};
					int index = 0;
					if (cgs.flagStatus == FLAG_TAKEN_RED) {
						color[1] = color[2] = 0;
						index = 1;
					} else if (cgs.flagStatus == FLAG_TAKEN_BLUE) {
						color[0] = color[1] = 0;
						index = 1;
					} else if (cgs.flagStatus == FLAG_DROPPED) {
						index = 2;
					}
					trap_R_SetColor(color);
					CG_DrawPic( x, y1, w, h, cgs.media.flagShaders[index] );
					trap_R_SetColor(NULL);
				}

				// Draw scores on top
				x += w;
			}
		}
#endif

		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 1.0f;
		color[3] = 0.33f;
		s = va( "%2i", s2 );
		if ( cgs.gametype == GT_1FCTF ) {
			h = w = FLAG_SIZE / 2;
		} else {
#ifdef IOQ3ZTM // FONT_REWRITE
			w = Com_FontStringWidth(&cgs.media.fontBig, s, 0) + 8;
#else
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
#endif
			h = BIGCHAR_HEIGHT + 8;
		}

		x -= w;
		CG_FillRect( x, y,  w, h, color );
		if ( cg.cur_ps->persistant[PERS_TEAM] == TEAM_BLUE ) {
			CG_DrawPic( x, y, w, h, cgs.media.selectShader );
		}
		CG_DrawBigString( x + 4, y, s, 1.0F);

		color[0] = 1.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = 0.33f;
		s = va( "%2i", s1 );
		if ( cgs.gametype == GT_1FCTF ) {
			h = w = FLAG_SIZE / 2;
		} else {
#ifdef IOQ3ZTM // FONT_REWRITE
			w = Com_FontStringWidth(&cgs.media.fontBig, s, 0) + 8;
#else
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
#endif
			h = BIGCHAR_HEIGHT + 8;
		}

		x -= w;

		CG_FillRect( x, y,  w, h, color );
		if ( cg.cur_ps->persistant[PERS_TEAM] == TEAM_RED ) {
			CG_DrawPic( x, y, w, h, cgs.media.selectShader );
		}
		CG_DrawBigString( x + 4, y, s, 1.0F);
	} else if (cgs.gametype != GT_SINGLE_PLAYER) {
		qboolean	spectator;

		x = 640;
		score = cg.cur_ps->persistant[PERS_SCORE];
		spectator = ( cg.cur_ps->persistant[PERS_TEAM] == TEAM_SPECTATOR );

		// always show your score in the second box if not in first place
		if ( s1 != score ) {
			s2 = score;
		}
		if ( s2 != SCORE_NOT_PRESENT ) {
			s = va( "%2i", s2 );
#ifdef IOQ3ZTM // FONT_REWRITE
			w = Com_FontStringWidth(&cgs.media.fontBig, s, 0) + 8;
#else
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
#endif
			x -= w;
			h = BIGCHAR_HEIGHT + 8;
			if ( !spectator && score == s2 && score != s1 ) {
				color[0] = 1.0f;
				color[1] = 0.0f;
				color[2] = 0.0f;
				color[3] = 0.33f;
				CG_FillRect( x, y,  w, h, color );
				CG_DrawPic( x, y, w, h, cgs.media.selectShader );
			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect( x, y,  w, h, color );
			}	
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

		// first place
		if ( s1 != SCORE_NOT_PRESENT ) {
			s = va( "%2i", s1 );
#ifdef IOQ3ZTM // FONT_REWRITE
			w = Com_FontStringWidth(&cgs.media.fontBig, s, 0) + 8;
#else
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
#endif
			x -= w;
			h = BIGCHAR_HEIGHT + 8;
			if ( !spectator && score == s1 ) {
				color[0] = 0.0f;
				color[1] = 0.0f;
				color[2] = 1.0f;
				color[3] = 0.33f;
				CG_FillRect( x, y,  w, h, color );
				CG_DrawPic( x, y, w, h, cgs.media.selectShader );
			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect( x, y,  w, h, color );
			}	
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}
	}

	return y1 - 8;
}
#endif // MISSIONPACK_HUD

/*
================
CG_DrawPowerups
================
*/
#ifndef MISSIONPACK_HUD
static float CG_DrawPowerups( float y ) {
	int		sorted[MAX_POWERUPS];
	int		sortedTime[MAX_POWERUPS];
	int		i, j, k;
	int		active;
	playerState_t	*ps;
	int		t;
	gitem_t	*item;
	int		x;
	int		color;
	float	size;
	float	f;
	static float colors[2][4] = { 
    { 0.2f, 1.0f, 0.2f, 1.0f } , 
    { 1.0f, 0.2f, 0.2f, 1.0f } 
  };

	ps = cg.cur_ps;

	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	// sort the list by time remaining
	active = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( !ps->powerups[ i ] ) {
			continue;
		}

#ifdef MISSIONPACK
		if (i == PW_SCOUT
			|| i == PW_GUARD
			|| i == PW_DOUBLER
			|| i == PW_AMMOREGEN)
		{
			continue;
		}
#endif

		t = ps->powerups[ i ] - cg.time;
		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if ( t < 0 || t > 999000) {
			continue;
		}

		// insert into the list
		for ( j = 0 ; j < active ; j++ ) {
			if ( sortedTime[j] >= t ) {
				for ( k = active - 1 ; k >= j ; k-- ) {
					sorted[k+1] = sorted[k];
					sortedTime[k+1] = sortedTime[k];
				}
				break;
			}
		}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	x = 640 - ICON_SIZE - CHAR_WIDTH * 2;
	for ( i = 0 ; i < active ; i++ ) {
		item = BG_FindItemForPowerup( sorted[i] );

    if (item) {

		  color = 1;

		  y -= ICON_SIZE;

		  trap_R_SetColor( colors[color] );
		  CG_DrawField( x, y, 2, sortedTime[ i ] / 1000 );

		  t = ps->powerups[ sorted[i] ];
		  if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
			  trap_R_SetColor( NULL );
		  } else {
			  vec4_t	modulate;

			  f = (float)( t - cg.time ) / POWERUP_BLINK_TIME;
			  f -= (int)f;
			  modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
			  trap_R_SetColor( modulate );
		  }

		  if ( cg.cur_lc->powerupActive == sorted[i] && 
			  cg.time - cg.cur_lc->powerupTime < PULSE_TIME ) {
			  f = 1.0 - ( ( (float)cg.time - cg.cur_lc->powerupTime ) / PULSE_TIME );
			  size = ICON_SIZE * ( 1.0 + ( PULSE_SCALE - 1.0 ) * f );
		  } else {
			  size = ICON_SIZE;
		  }

		  CG_DrawPic( 640 - size, y + ICON_SIZE / 2 - size / 2, 
			  size, size, trap_R_RegisterShader( item->icon ) );
    }
	}
	trap_R_SetColor( NULL );

	return y;
}
#endif // MISSIONPACK_HUD

/*
=====================
CG_DrawLowerRight

=====================
*/
#ifndef MISSIONPACK_HUD
static void CG_DrawLowerRight( void ) {
	float	y;

#ifdef TA_MISC // HUD
	y = 480;
#else
	y = 480 - ICON_SIZE;
#endif

	CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 2 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qfalse );
	} 

	y = CG_DrawScores( y );
	y = CG_DrawPowerups( y );
}
#endif // MISSIONPACK_HUD

/*
===================
CG_DrawPickupItem
===================
*/
#ifndef MISSIONPACK_HUD
static int CG_DrawPickupItem( int y ) {
	int		value;
	float	*fadeColor;
	gitem_t	*item;

	if ( cg.cur_ps->stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	y -= ICON_SIZE;

	value = cg.cur_lc->itemPickup;
	item = BG_ItemForItemNum( value );

#ifdef TA_WEAPSYS
	if (item && item->giType == IT_WEAPON
		&& item->giTag == WP_DEFAULT)
	{
		item = BG_FindItemForWeapon(cgs.clientinfo[cg.cur_ps->clientNum].playercfg.default_weapon);
		value = BG_ItemNumForItem(item);
	}
#endif
	if ( value ) {
		fadeColor = CG_FadeColor( cg.cur_lc->itemPickupTime, 3000 );
		if ( fadeColor ) {
			CG_RegisterItemVisuals( value );
			trap_R_SetColor( fadeColor );
			CG_DrawPic( 8, y, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
			CG_DrawBigString( ICON_SIZE + 16, y + (ICON_SIZE/2 - BIGCHAR_HEIGHT/2), item->pickup_name, fadeColor[0] );
			trap_R_SetColor( NULL );
		}
	}
	
	return y;
}
#endif // MISSIONPACK_HUD

/*
=====================
CG_DrawLowerLeft

=====================
*/
#ifndef MISSIONPACK_HUD
static void CG_DrawLowerLeft( void ) {
	float	y;

	y = 480 - ICON_SIZE;

	CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 3 ) {
		y = CG_DrawTeamOverlay( y, qfalse, qfalse );
	} 


	y = CG_DrawPickupItem( y );
}
#endif // MISSIONPACK_HUD


//===========================================================================================

/*
=================
CG_DrawTeamInfo
=================
*/
#ifndef MISSIONPACK_HUD
static void CG_DrawTeamInfo( void ) {
	int h;
	int i;
	vec4_t		hcolor;
	int		chatHeight;

#define CHATLOC_Y 420 // bottom end
#define CHATLOC_X 0

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_teamChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;
	if (chatHeight <= 0)
		return; // disabled

	CG_SetScreenPlacement( PLACE_LEFT, PLACE_BOTTOM );

	if (cgs.teamLastChatPos != cgs.teamChatPos) {
		if (cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer
#ifdef IOQ3ZTM // TEAM_CHAT_CON
			*1000
#endif
		) {
			cgs.teamLastChatPos++;
		}

		h = (cgs.teamChatPos - cgs.teamLastChatPos) * TINYCHAR_HEIGHT;

		if ( cg.cur_ps->persistant[PERS_TEAM] == TEAM_RED ) {
			hcolor[0] = 1.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		} else if ( cg.cur_ps->persistant[PERS_TEAM] == TEAM_BLUE ) {
			hcolor[0] = 0.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 1.0f;
			hcolor[3] = 0.33f;
		} else {
			hcolor[0] = 0.0f;
			hcolor[1] = 1.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}

		trap_R_SetColor( hcolor );
		CG_DrawPic( CHATLOC_X, CHATLOC_Y - h, 640, h, cgs.media.teamStatusBar );
		trap_R_SetColor( NULL );

		hcolor[0] = hcolor[1] = hcolor[2] = 1.0f;
		hcolor[3] = 1.0f;

		for (i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--) {
			CG_DrawStringExt( CHATLOC_X + TINYCHAR_WIDTH, 
				CHATLOC_Y - (cgs.teamChatPos - i)*TINYCHAR_HEIGHT, 
				cgs.teamChatMsgs[i % chatHeight], hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
		}
	}
}
#endif // MISSIONPACK_HUD

/*
===================
CG_DrawHoldableItem
===================
*/
#ifndef MISSIONPACK_HUD
#ifndef TURTLEARENA
static void CG_DrawHoldableItem( void ) { 
	int		value;

#ifdef TA_HOLDSYS
	value = BG_ItemNumForHoldableNum(cg.cur_ps->holdableIndex);
#else
	value = cg.cur_ps->stats[STAT_HOLDABLE_ITEM];
#endif
	if ( value ) {
		CG_RegisterItemVisuals( value );
		CG_DrawPic( 640-ICON_SIZE, (SCREEN_HEIGHT-ICON_SIZE)/2, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
	}

}
#endif
#endif // MISSIONPACK_HUD

#ifdef MISSIONPACK
/*
===================
CG_DrawPersistantPowerup
===================
*/
#ifndef MISSIONPACK_HUD
static void CG_DrawPersistantPowerup( void ) { 
	int		value;

#ifdef TURTLEARENA
	CG_SetScreenPlacement(PLACE_LEFT, PLACE_CENTER);
#else
	CG_SetScreenPlacement(PLACE_RIGHT, PLACE_CENTER);
#endif

	value = cg.cur_ps->stats[STAT_PERSISTANT_POWERUP];
	if ( value ) {
		CG_RegisterItemVisuals( value );
#ifdef TURTLEARENA
		CG_DrawPic( HUD_X + 16, HUD_Y+HUD_HEIGHT+4, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
#else
		CG_DrawPic( 640-ICON_SIZE, (SCREEN_HEIGHT-ICON_SIZE)/2 - ICON_SIZE, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
#endif
	}
}
#endif // MISSIONPACK_HUD
#endif // MISSIONPACK


/*
===================
CG_DrawReward
===================
*/
static void CG_DrawReward( void ) { 
	float	*color;
	int		i, count;
	float	x, y;
	char	buf[32];

	if ( !cg_drawRewards.integer ) {
		return;
	}

	CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	color = CG_FadeColor( cg.cur_lc->rewardTime, REWARD_TIME );
	if ( !color ) {
		if (cg.cur_lc->rewardStack > 0) {
			for(i = 0; i < cg.cur_lc->rewardStack; i++) {
#ifdef TA_MISC // COMIC_ANNOUNCER
				cg.cur_lc->rewardAnnoucement[i] = cg.cur_lc->rewardAnnoucement[i+1];
#else
				cg.cur_lc->rewardSound[i] = cg.cur_lc->rewardSound[i+1];
#endif
				cg.cur_lc->rewardShader[i] = cg.cur_lc->rewardShader[i+1];
				cg.cur_lc->rewardCount[i] = cg.cur_lc->rewardCount[i+1];
			}
			cg.cur_lc->rewardTime = cg.time;
			cg.cur_lc->rewardStack--;
			color = CG_FadeColor( cg.cur_lc->rewardTime, REWARD_TIME );
#ifdef TA_MISC // COMIC_ANNOUNCER
			CG_AddAnnouncement(cg.cur_lc->rewardAnnoucement[0], cg.cur_lc - cg.localClients);
#else
			trap_S_StartLocalSound(cg.cur_lc->rewardSound[0], CHAN_ANNOUNCER);
#endif
		} else {
			return;
		}
	}

	trap_R_SetColor( color );

	/*
	count = cg.cur_lc->rewardCount[0]/10;				// number of big rewards to draw

	if (count) {
		y = 4;
		x = 320 - count * ICON_SIZE;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, (ICON_SIZE*2)-4, (ICON_SIZE*2)-4, cg.cur_lc->rewardShader[0] );
			x += (ICON_SIZE*2);
		}
	}

	count = cg.cur_lc->rewardCount[0] - count*10;		// number of small rewards to draw
	*/

	if ( cg.cur_lc->rewardCount[0] >= 10 ) {
		y = 56;
		x = 320 - ICON_SIZE/2;
		CG_DrawPic( x, y, ICON_SIZE-4, ICON_SIZE-4, cg.cur_lc->rewardShader[0] );
		Com_sprintf(buf, sizeof(buf), "%d", cg.cur_lc->rewardCount[0]);
#ifdef IOQ3ZTM // FONT_REWRITE
		CG_DrawBigStringColor(CENTER_X, y+ICON_SIZE, buf, color);
#else
		CG_DrawStringExt( CENTER_X, y+ICON_SIZE, buf, color, qfalse, qtrue,
								SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
#endif
	}
	else {

		count = cg.cur_lc->rewardCount[0];

		y = 56;
		x = 320 - count * ICON_SIZE/2;
		for ( i = 0 ; i < count ; i++ ) {
			CG_DrawPic( x, y, ICON_SIZE-4, ICON_SIZE-4, cg.cur_lc->rewardShader[0] );
			x += ICON_SIZE;
		}
	}
	trap_R_SetColor( NULL );
}


/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define	LAG_SAMPLES		128


typedef struct {
	int		frameSamples[LAG_SAMPLES];
	int		frameCount;
	int		snapshotFlags[LAG_SAMPLES];
	int		snapshotSamples[LAG_SAMPLES];
	int		snapshotCount;
} lagometer_t;

lagometer_t		lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int			offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1) ] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect( void ) {
#ifndef TURTLEARENA
	float		x, y;
#endif
	int			cmdNum;
	usercmd_t	cmd;
	const char		*s;

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd, cg.cur_localClientNum );
	if ( cmd.serverTime <= cg.cur_ps->commandTime
		|| cmd.serverTime > cg.time ) {	// special check for map_restart
		return;
	}

	CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	// also add text in center of screen
	s = "Connection Interrupted";
	CG_DrawBigString( CENTER_X, 100, s, 1.0F);

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

#ifdef TURTLEARENA
	CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);

	CG_DrawPic( 0, 0, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga" ) );
#else
	CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);

	x = 640 - 48;
	y = 480 - 48;

	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga" ) );
#endif
}


#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	int		a, x, y, i;
	float	v;
	float	ax, ay, aw, ah, mid, range;
	int		color;
	float	vscale;

	if ( !cg_lagometer.integer || cgs.localServer ) {
		CG_DrawDisconnect();
		return;
	}

#ifdef TURTLEARENA
	CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
#else
	CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);
#endif

	//
	// draw the graph
	//
#ifdef TURTLEARENA
	x = 0;
#else
	x = 640 - 48;
#endif
#ifdef MISSIONPACK_HUD
	y = 480 - 144;
#else
	y = 480 - 48;
#endif

	trap_R_SetColor( NULL );
	CG_DrawPic( x, y, 48, 48, cgs.media.lagometerShader );

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
			}
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic ( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_BLUE)] );
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;	// YELLOW for rate delay
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_GREEN)] );
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;		// RED for dropped snapshots
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_RED)] );
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap_R_SetColor( NULL );

	if ( cg_nopredict.integer || cg_synchronousClients.integer ) {
		CG_DrawBigString( x, y, "snc", 1.0 );
	}

	CG_DrawDisconnect();
}



/*
===============================================================================

VOIP METER

===============================================================================
*/

/*
==============
CG_DrawVoipMeter
==============
*/
void CG_DrawVoipMeter( void ) {
	char	buffer[16];
	char	string[256];
	int		limit, i;
	int		voipTime;
	float	voipPower;

	if ( !cg_voipShowMeter.integer ) {
		return; // player doesn't want to show meter at all.
	}

	voipTime = trap_GetVoipTime( cg.cur_lc->clientNum );

	// check if voip was used in the last 1/4 second
	if ( !voipTime || voipTime < cg.time - 250 ) {
		return;
	}

	CG_SetScreenPlacement( PLACE_CENTER, PLACE_TOP );

	voipPower = trap_GetVoipPower( cg.cur_lc->clientNum );

	limit = (int) (voipPower * 10.0f);
	if (limit > 10)
		limit = 10;

	for (i = 0; i < limit; i++)
		buffer[i] = '*';
	while (i < 10)
		buffer[i++] = ' ';
	buffer[i] = '\0';

	Com_sprintf( string, sizeof ( string ), "VoIP: [%s]", buffer );
	CG_DrawStringExt( CENTER_X, 10, string, colorWhite, qfalse, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
}


/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the viewport
for a few moments
==============
*/
void CG_CenterPrint( int localClientNum, const char *str, int y, float charScale ) {
	cglc_t	*lc;
	char	*s;

	lc = &cg.localClients[localClientNum];

	Q_strncpyz( lc->centerPrint, str, sizeof(lc->centerPrint) );

	lc->centerPrintTime = cg.time;
	lc->centerPrintY = y;
	lc->centerPrintCharScale = charScale;

	// count the number of lines for centering
	lc->centerPrintLines = 1;
	s = lc->centerPrint;
	while( *s ) {
		if (*s == '\n')
			lc->centerPrintLines++;
		s++;
	}
}


/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	char	*start;
	int		l;
	int		y;
#if defined MISSIONPACK_HUD || defined IOQ3ZTM
	int		h;
#else
	int		charWidth;
#endif
	int		charHeight;
	float	*color;

	if ( !cg.cur_lc->centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.cur_lc->centerPrintTime, 1000 * cg_centertime.value );
	if ( !color ) {
		return;
	}

	CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	trap_R_SetColor( color );

	start = cg.cur_lc->centerPrint;

#if defined MISSIONPACK_HUD || defined IOQ3ZTM
	charHeight = CG_Text_Height("I", cg.cur_lc->centerPrintCharScale, 0);
#else
	charWidth = BIGCHAR_WIDTH * 2 * cg.cur_lc->centerPrintCharScale;
	charHeight = (int)(charWidth * 1.5);
#endif

	y = cg.cur_lc->centerPrintY - cg.cur_lc->centerPrintLines * charHeight / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 50; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

#if defined MISSIONPACK_HUD || defined IOQ3ZTM
		h = CG_Text_Height(linebuffer, cg.cur_lc->centerPrintCharScale, 0);
		CG_Text_Paint(CENTER_X, y + h, cg.cur_lc->centerPrintCharScale, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
		y += h + 6;
#else
		CG_DrawStringExt( CENTER_X, y, linebuffer, color, qfalse, qtrue,
			charWidth, charHeight, 0 );

		y += charHeight;
#endif
		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap_R_SetColor( NULL );
}


/*
==============
CG_GlobalCenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_GlobalCenterPrint( const char *str, int y, float charScale ) {
	char	*s;

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharScale = charScale;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s ) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}


/*
===================
CG_DrawGlobalCenterString
===================
*/
static void CG_DrawGlobalCenterString( void ) {
	char	*start;
	int		l;
	int		y;
#if defined MISSIONPACK_HUD || defined IOQ3ZTM
	int		h;
#else
	int		charWidth;
#endif
	int		charHeight;
	float	*color;

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value );
	if ( !color ) {
		return;
	}

	CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	trap_R_SetColor( color );

	start = cg.centerPrint;

#if defined MISSIONPACK_HUD || defined IOQ3ZTM
	charHeight = CG_Text_Height("I", cg.centerPrintCharScale, 0);
#else
	charWidth = BIGCHAR_WIDTH * 2 * cg.centerPrintCharScale;
	charHeight = (int)(charWidth * 1.5);
#endif

	y = cg.centerPrintY - cg.centerPrintLines * charHeight / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 50; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

#if defined MISSIONPACK_HUD || defined IOQ3ZTM
		h = CG_Text_Height(linebuffer, cg.centerPrintCharScale, 0);
		CG_Text_Paint(CENTER_X, y + h, cg.centerPrintCharScale, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
		y += h + 6;
#else
		CG_DrawStringExt( CENTER_X, y, linebuffer, color, qfalse, qtrue,
			charWidth, charHeight, 0 );

		y += charHeight;
#endif
		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap_R_SetColor( NULL );
}


/*
================================================================================

CROSSHAIR

================================================================================
*/


/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair(void)
{
	float		w, h;
	qhandle_t	hShader;
	float		f;
	float		x, y;
	int			ca;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	if ( cg.cur_ps->persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if ( cg.cur_lc->renderingThirdPerson ) {
		return;
	}

	CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	// set color based on health
	if ( cg_crosshairHealth.integer ) {
		vec4_t		hcolor;

		CG_ColorForHealth( hcolor );
		trap_R_SetColor( hcolor );
	} else {
		trap_R_SetColor( NULL );
	}

	if (cg.numViewports > 1) {
		// In splitscreen make crosshair normal [non-splitscreen] size, so it is easier to see.
		w = h = cg_crosshairSize.value * 2.0f;
	} else {
		w = h = cg_crosshairSize.value;
	}

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.cur_lc->itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;

	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ ca % NUM_CROSSHAIRS ];

	CG_DrawPic( ((SCREEN_WIDTH-w)*0.5f)+x, ((SCREEN_HEIGHT-h)*0.5f)+y, w, h, hShader );
}

/*
=================
CG_DrawCrosshair3D
=================
*/
static void CG_DrawCrosshair3D(void)
{
	float		w;
	qhandle_t	hShader;
	float		f;
	int			ca;

	vec4_t hcolor;
	trace_t trace;
	vec3_t endpos;
	float stereoSep, zProj, maxdist, xmax;
	char rendererinfos[128];
	refEntity_t ent;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	if ( cg.cur_ps->persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if ( cg.cur_lc->renderingThirdPerson ) {
		return;
	}

	// set color based on health
	if ( cg_crosshairHealth.integer ) {
		CG_ColorForHealth( hcolor );
	} else {
		hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1;
	}

	w = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.cur_lc->itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
	}

	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ ca % NUM_CROSSHAIRS ];

	// Use a different method rendering the crosshair so players don't see two of them when
	// focusing their eyes at distant objects with high stereo separation
	// We are going to trace to the next shootable object and place the crosshair in front of it.

	// first get all the important renderer information
	trap_Cvar_VariableStringBuffer("r_zProj", rendererinfos, sizeof(rendererinfos));
	zProj = atof(rendererinfos);
	trap_Cvar_VariableStringBuffer("r_stereoSeparation", rendererinfos, sizeof(rendererinfos));
	stereoSep = zProj / atof(rendererinfos);
	
	xmax = zProj * tan(cg.refdef.fov_x * M_PI / 360.0f);
	
	// let the trace run through until a change in stereo separation of the crosshair becomes less than one pixel.
	maxdist = cgs.glconfig.vidWidth * stereoSep * zProj / (2 * xmax);
	VectorMA(cg.refdef.vieworg, maxdist, cg.refdef.viewaxis[0], endpos);
	CG_Trace(&trace, cg.refdef.vieworg, NULL, NULL, endpos, 0, MASK_SHOT);
	
	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_DEPTHHACK | RF_CROSSHAIR;
	ent.shaderRGBA[0] = 255 * hcolor[0];
	ent.shaderRGBA[1] = 255 * hcolor[1];
	ent.shaderRGBA[2] = 255 * hcolor[2];
	ent.shaderRGBA[3] = 255 * hcolor[3];
	
	VectorCopy(trace.endpos, ent.origin);
	
	// scale the crosshair so it appears the same size for all distances
	ent.radius = w / 640 * xmax * trace.fraction * maxdist / zProj;
	ent.customShader = hShader;

	trap_R_AddRefEntityToScene(&ent);
}


/*
=================
CG_DrawThirdPersonCrosshair
=================
*/
static void CG_DrawThirdPersonCrosshair(void)
{
	float		w;
	qhandle_t	hShader;
	float		f;
	int			ca;

	vec4_t hcolor;
	trace_t trace;
	vec3_t endpos, distToScreen;
	float maxdist, xmax;
	refEntity_t ent;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	if ( cg.cur_ps->persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if ( !cg.cur_lc->renderingThirdPerson || !cg_thirdPerson[cg.cur_localClientNum].integer ) {
		return;
	}

	// set color based on health
	if ( cg_crosshairHealth.integer ) {
		CG_ColorForHealth( hcolor );
	} else {
		hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1;
	}

	w = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.cur_lc->itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
	}

	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ ca % NUM_CROSSHAIRS ];

	xmax = tan(cg.refdef.fov_x * M_PI / 360.0f);
	// We are going to trace to the next shootable object and place the crosshair in front of it.
	maxdist = cgs.glconfig.vidWidth / (2 * xmax);
	VectorMA(cg.firstPersonViewOrg, maxdist, cg.firstPersonViewAxis[0], endpos);
	CG_Trace(&trace, cg.firstPersonViewOrg, NULL, NULL, endpos, 0, MASK_SHOT);

	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_DEPTHHACK | RF_CROSSHAIR;
	ent.shaderRGBA[0] = 255 * hcolor[0];
	ent.shaderRGBA[1] = 255 * hcolor[1];
	ent.shaderRGBA[2] = 255 * hcolor[2];
	ent.shaderRGBA[3] = 255 * hcolor[3];

	VectorCopy(trace.endpos, ent.origin);
	VectorSubtract( cg.refdef.vieworg, ent.origin, distToScreen );

	// scale the crosshair so it appears the same size for all distances
	ent.radius = w / 640 * xmax * VectorLength( distToScreen );
	ent.customShader = hShader;

	trap_R_AddRefEntityToScene(&ent);
}



/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void ) {
	trace_t		trace;
	vec3_t		start, end;
	int			content;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 131072, cg.refdef.viewaxis[0], end );

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end, 
		cg.cur_ps->clientNum, CONTENTS_SOLID|CONTENTS_BODY );
	if ( trace.entityNum >= MAX_CLIENTS ) {
		return;
	}

	// if the player is in fog, don't show it
	content = CG_PointContents( trace.endpos, 0 );
	if ( content & CONTENTS_FOG ) {
		return;
	}

	// if the player is invisible, don't show it
	if ( cg_entities[ trace.entityNum ].currentState.powerups & ( 1 << PW_INVIS ) ) {
		return;
	}

	// update the fade timer
	cg.cur_lc->crosshairClientNum = trace.entityNum;
	cg.cur_lc->crosshairClientTime = cg.time;
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	float		*color;
	char		*name;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}
	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	if ( cg.cur_lc->renderingThirdPerson ) {
		return;
	}

	CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.cur_lc->crosshairClientTime, 1000 );
	if ( !color ) {
		trap_R_SetColor( NULL );
		return;
	}

	name = cgs.clientinfo[ cg.cur_lc->crosshairClientNum ].name;
#if defined MISSIONPACK_HUD || defined IOQ3ZTM
	color[3] *= 0.5f;
	CG_Text_Paint( CENTER_X, 170 + CG_Text_Height(name, 0.3f, 0), 0.3f, color, name, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
#else
	CG_DrawBigString( CENTER_X, 170, name, color[3] * 0.5f );
#endif
	trap_R_SetColor( NULL );

	if ( cg_voipShowCrosshairMeter.integer )
	{
		float voipPower = trap_GetVoipPower( cg.cur_lc->crosshairClientNum );
		int voipTime = trap_GetVoipTime( cg.cur_lc->crosshairClientNum );

		if ( voipPower > 0 && voipTime > 0 && voipTime >= cg.time - 250 ) {
			int limit, i;
			char string[256];
			char buffer[16];

			limit = (int) (voipPower * 10.0f);
			if (limit > 10)
				limit = 10;

			for (i = 0; i < limit; i++)
				buffer[i] = '*';
			while (i < 10)
				buffer[i++] = ' ';
			buffer[i] = '\0';

			Com_sprintf( string, sizeof ( string ), "VoIP: [%s]", buffer );
#if defined MISSIONPACK_HUD || defined IOQ3ZTM
			CG_Text_Paint( CENTER_X, 170 + CG_Text_Height(name, 0.3f, 0) * 2.5, 0.3f, color, string, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
#else
			CG_DrawBigString( CENTER_X, 170 + BIGCHAR_HEIGHT * 1.5f, string, color[3] * 0.5f );
#endif
		}
	}
}

/*
=====================
CG_DrawShaderInfo
=====================
*/
static void CG_DrawShaderInfo( void ) {
	char		name[MAX_QPATH];
	float		x, y, width, height;
	vec4_t		color = { 1, 1, 1, 1 };
	qhandle_t	hShader;
	trace_t		trace;
	vec3_t		start, end;
	qboolean	everything = qtrue;

	if ( !cg_drawShaderInfo.integer ) {
		return;
	}

	width = 128;
	height = 128;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 131072, cg.refdef.viewaxis[0], end );

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end,
		cg.cur_ps->clientNum, everything ? ~0 : CONTENTS_SOLID );

	if ( trace.surfaceNum <= 0 )
		return;

	// scan the known entities to see if the crosshair is sighted on one
	hShader = trap_R_GetSurfaceShader( trace.surfaceNum, -1 );

	trap_R_GetShaderName( hShader, name, sizeof ( name ) );

	CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);

#ifdef MISSIONPACK_HUD
	color[3] *= 0.5f;
	CG_Text_Paint( 0, SCREEN_HEIGHT - height - 28, 0.3f, color, name, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
#else
	CG_DrawBigString( 0, SCREEN_HEIGHT - height - 28, name, color[3] * 0.5f );
#endif
	trap_R_SetColor( NULL );

	x = 0;
	y = SCREEN_HEIGHT - height;

	CG_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void) {
	CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);
	CG_DrawBigString(CENTER_X, 440, "SPECTATOR", 1.0F);
	if ( cgs.gametype == GT_TOURNAMENT ) {
		CG_DrawBigString(CENTER_X, 460, "waiting to play", 1.0F);
	}
	else if ( cgs.gametype >= GT_TEAM ) {
#ifdef TA_MISC // SMART_JOIN_MENU
		CG_DrawBigString(CENTER_X, 460, "press ESC and use the TEAM menu to play", 1.0F);
#elif defined IOQ3ZTM
		CG_DrawBigString(CENTER_X, 460, "press ESC and use the START menu to play", 1.0F);
#else
		CG_DrawBigString(CENTER_X, 460, "press ESC and use the JOIN menu to play", 1.0F);
#endif
	}
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void) {
	char	*s;
	int		sec;
	char	yesKeys[64];
	char	noKeys[64];
	float	y;

	if ( !cgs.voteTime ) {
		return;
	}

#ifdef TURTLEARENA
	CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
#else
	CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
#endif

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}

#ifdef TURTLEARENA
	y = 304;
#else
	y = 58;
#endif

	CG_KeysStringForBinding( Com_LocalClientCvarName( cg.cur_localClientNum, "vote yes" ), yesKeys, sizeof (yesKeys) );
	CG_KeysStringForBinding( Com_LocalClientCvarName( cg.cur_localClientNum, "vote no" ), noKeys, sizeof (noKeys) );

	s = va( "Vote (%i): %s", sec, cgs.voteString );
	CG_DrawSmallString( 2, y, s, 1.0F );
	s = va( "Yes (%s): %i, No (%s): %i", yesKeys, cgs.voteYes, noKeys, cgs.voteNo );
	CG_DrawSmallString( 2, y + SMALLCHAR_HEIGHT + 2, s, 1.0F );
#ifdef MISSIONPACK_HUD
	s = "or press ESC then click Vote";
	CG_DrawSmallString( 2, y + ( SMALLCHAR_HEIGHT + 2 ) * 2, s, 1.0F );
#endif
}

/*
=================
CG_DrawTeamVote
=================
*/
static void CG_DrawTeamVote(void) {
	char	*s;
	int		sec, cs_offset;
	float	y;

	if ( cgs.clientinfo[cg.cur_ps->clientNum].team == TEAM_RED )
		cs_offset = 0;
	else if ( cgs.clientinfo[cg.cur_ps->clientNum].team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !cgs.teamVoteTime[cs_offset] ) {
		return;
	}

#ifdef TURTLEARENA
	CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);
#else
	CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
#endif

	// play a talk beep whenever it is modified
	if ( cgs.teamVoteModified[cs_offset] ) {
		cgs.teamVoteModified[cs_offset] = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.teamVoteTime[cs_offset] ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
#ifdef TURTLEARENA
	y = 336;
#else
	y = 90;
#endif
	s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
							cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
	CG_DrawSmallString( 0, y, s, 1.0F );
}

/*
=============
CG_AnyScoreboardShowing
=============
*/
qboolean CG_AnyScoreboardShowing( void ) {
	int i;

	for ( i = 0; i < CG_MaxSplitView(); i++ ) {
		if ( cg.localClients[i].clientNum != -1 && cg.localClients[i].scoreBoardShowing ) {
			return qtrue;
		}
	}

	return qfalse;
}

static qboolean CG_DrawScoreboard( void ) {
#ifdef MISSIONPACK_HUD
	static qboolean firstTime[MAX_SPLITVIEW] = {qtrue, qtrue, qtrue, qtrue};

	CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	if (menuScoreboard) {
		menuScoreboard->window.flags &= ~WINDOW_FORCED;
	}
	if (cg_paused.integer) {
		if (cg.cur_lc) {
			firstTime[cg.cur_localClientNum] = qtrue;
		}
		return qfalse;
	}

	// should never happen in Team Arena
	if (cgs.gametype == GT_SINGLE_PLAYER && cg.cur_lc && cg.cur_lc->predictedPlayerState.pm_type == PM_INTERMISSION ) {
		firstTime[cg.cur_localClientNum] = qtrue;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && cg.cur_lc && !cg.cur_lc->showScores ) {
		return qfalse;
	}

	if ( !cg.cur_lc || cg.cur_lc->showScores || cg.cur_lc->predictedPlayerState.pm_type == PM_DEAD ||
		 cg.cur_lc->predictedPlayerState.pm_type == PM_INTERMISSION ) {
	} else {
		if ( !CG_FadeColor( cg.cur_lc->scoreFadeTime, FADE_TIME ) ) {
			// next time scoreboard comes up, don't print killer
			cg.cur_lc->killerName[0] = 0;
			firstTime[cg.cur_localClientNum] = qtrue;
			return qfalse;
		}
	}

	if (menuScoreboard == NULL) {
		if ( cgs.gametype >= GT_TEAM ) {
			menuScoreboard = Menus_FindByName("teamscore_menu");
		} else {
			menuScoreboard = Menus_FindByName("score_menu");
		}
	}

	if (menuScoreboard) {
		if (cg.cur_lc && firstTime[cg.cur_localClientNum]) {
			firstTime[cg.cur_localClientNum] = qfalse;
			CG_SetScoreSelection(menuScoreboard);

			// Update time now to prevent spectator list from jumping.
			cg.spectatorTime = trap_Milliseconds();
		}
		Menu_Paint(menuScoreboard, qtrue);
	}

	return qtrue;
#else
	return CG_DrawOldScoreboard();
#endif
}

#ifdef TA_SP
/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawSPIntermission( void ) {
	char		str[1024];
	char		name[64];
	vec4_t		color;

	if (!cg.cur_ps->persistant[PERS_LIVES] && !cg.cur_ps->persistant[PERS_CONTINUES]) {
		return;
	}

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	if (cg_singlePlayer.integer) {
		if (cgs.clientinfo[ cg.cur_ps->clientNum ].headModelName[0] == '*') {
			Q_strncpyz(name, cgs.clientinfo[ cg.cur_ps->clientNum ].headModelName+1, sizeof (name));
		} else {
			Q_strncpyz(name, cgs.clientinfo[ cg.cur_ps->clientNum ].headModelName, sizeof (name));
		}

		name[0] = toupper(name[0]);
	} else {
		Q_strncpyz(name, cgs.clientinfo[ cg.cur_ps->clientNum ].name, sizeof (name));
	}

	Com_sprintf(str, sizeof (str), "%s got through the level", name);

#if defined MISSIONPACK_HUD || defined IOQ3ZTM
	CG_Text_Paint( CENTER_X, 210 + CG_Text_Height(name, 0.3f, 0), 0.3f, color, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
#else
	CG_DrawBigString( CENTER_X, 210, str, color[3] );
#endif
	trap_R_SetColor( NULL );
}
#endif

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
//	int key;
#if defined MISSIONPACK_HUD && !defined TA_SP
	//if (cg_singlePlayer.integer) {
	//	CG_DrawCenterString();
	//	return;
	//}
#else
	if ( cgs.gametype == GT_SINGLE_PLAYER ) {
#ifdef TA_SP
		CG_DrawSPIntermission();
#endif
		CG_DrawCenterString();
		return;
	}
#endif
	cg.cur_lc->scoreFadeTime = cg.time;
	cg.cur_lc->scoreBoardShowing = CG_DrawScoreboard();
}

/*
=================
CG_DrawBotInfo

Draw info for bot that client is following.
=================
*/
static qboolean CG_DrawBotInfo( int y ) {
	const char	*info, *str, *leader, *carrying, *action, *node;
	int x;

	if ( !(cg.cur_ps->pm_flags & PMF_FOLLOW) ) {
		return qfalse;
	}

	info = CG_ConfigString( CS_BOTINFO + cg.cur_ps->clientNum );

	if (!*info) {
		return qfalse;
	}

	node = Info_ValueForKey(info, "n");

	if ( *node ) {
		str = va("AI Node: %s", node);
		x = 0.5 * ( 640 - BIGCHAR_WIDTH * CG_DrawStrlen( str ) );

		CG_DrawBigString( x, y, str, 1.0F );

		y += BIGCHAR_HEIGHT + 2;
	}

	action = Info_ValueForKey(info, "a");

	if ( *action ) {
		x = 0.5 * ( 640 - BIGCHAR_WIDTH * CG_DrawStrlen( action ) );

		CG_DrawBigString( x, y, action, 1.0F );

		y += BIGCHAR_HEIGHT + 2;
	}

	leader = Info_ValueForKey(info, "l");

	if ( *leader ) {
		str = "bot is leader";
		x = 0.5 * ( 640 - BIGCHAR_WIDTH * CG_DrawStrlen( str ) );

		CG_DrawBigString( x, y, str, 1.0F );

		y += BIGCHAR_HEIGHT + 2;
	}

	carrying = Info_ValueForKey(info, "c");

	if ( *carrying ) {
		str = va("bot carrying: %s", carrying);
		x = 0.5 * ( 640 - BIGCHAR_WIDTH * CG_DrawStrlen( str ) );

		CG_DrawBigString( x, y, str, 1.0F );
	}

	return qtrue;
}

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow( void ) {
	const char	*name;

	if ( !(cg.cur_ps->pm_flags & PMF_FOLLOW) ) {
		return qfalse;
	}

	CG_SetScreenPlacement(PLACE_CENTER, PLACE_TOP);

	CG_DrawBigString( CENTER_X, 24, "following", 1.0F );

	name = cgs.clientinfo[ cg.cur_ps->clientNum ].name;

	CG_DrawGiantString(CENTER_X, 40, name, 1.0F );

	CG_DrawBotInfo( 40 + GIANT_HEIGHT );

	return qtrue;
}

#ifdef TA_ENTSYS // FUNC_USE
/*
=================
CG_DrawUseEntity
=================
*/
static qboolean CG_DrawUseEntity(void)
{
	const char *s;
	float		h;
	float		w;
	float		x;
	float		y;

	if (!(cg.cur_ps->eFlags & EF_USE_ENT))
		return qfalse;

	CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);

	s = "Use Entity!";

#ifdef IOQ3ZTM // FONT_REWRITE
	h = Com_FontStringHeight(&cgs.media.fontBig, s, 0);
	w = Com_FontStringWidth(&cgs.media.fontBig, s, 0);
#else
	h = BIGCHAR_HEIGHT
	w = BIGCHAR_WIDTH * CG_DrawStrlen( s );
#endif
	x = ( SCREEN_WIDTH - w ) * 0.5f;
	y = SCREEN_HEIGHT-h-12;

	CG_DrawBigString(x, y, s, 1);

	return qtrue;
}
#endif



#ifndef TURTLEARENA // NO_AMMO_WARNINGS
/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning( void ) {
	const char	*s;

	if ( cg_drawAmmoWarning.integer == 0 ) {
		return;
	}

	if ( !cg.cur_lc->lowAmmoWarning ) {
		return;
	}

	CG_SetScreenPlacement(PLACE_CENTER, PLACE_TOP);

	if ( cg.cur_lc->lowAmmoWarning == 2 ) {
		s = "OUT OF AMMO";
	} else {
		s = "LOW AMMO WARNING";
	}
	CG_DrawBigString(CENTER_X, 64, s, 1.0F);
}
#endif

#if defined MISSIONPACK && !defined TURTLEARENA // WEAPONS
/*
=================
CG_DrawProxWarning
=================
*/
static void CG_DrawProxWarning( void ) {
	char s [32];
  static int proxTime;
  static int proxCounter;
  static int proxTick;

	if( !(cg.cur_ps->eFlags & EF_TICKING ) ) {
    proxTime = 0;
		return;
	}

	CG_SetScreenPlacement(PLACE_CENTER, PLACE_TOP);

  if (proxTime == 0) {
    proxTime = cg.time + 5000;
    proxCounter = 5;
    proxTick = 0;
  }

  if (cg.time > proxTime) {
    proxTick = proxCounter--;
    proxTime = cg.time + 1000;
  }

  if (proxTick != 0) {
    Com_sprintf(s, sizeof(s), "INTERNAL COMBUSTION IN: %i", proxTick);
  } else {
    Com_sprintf(s, sizeof(s), "YOU HAVE BEEN MINED");
  }

	CG_DrawBigStringColor( CENTER_X, 64 + BIGCHAR_HEIGHT, s, g_color_table[ColorIndex(COLOR_RED)] );
}
#endif


/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup( void ) {
	int			sec;
	int			i;
#if defined MISSIONPACK_HUD || defined IOQ3ZTM
	float		scale;
#else
 	int			cw;
	int			w;
#endif
	clientInfo_t	*ci1, *ci2;
	const char	*s;

	sec = cg.warmup;
	if ( !sec ) {
		return;
	}

	CG_SetScreenPlacement(PLACE_CENTER, PLACE_TOP);

	if ( sec < 0 ) {
		s = "Waiting for players";
		CG_DrawBigString(CENTER_X, 24, s, 1.0F);
		cg.warmupCount = 0;
		return;
	}

	if (cgs.gametype == GT_TOURNAMENT) {
		// find the two active players
		ci1 = NULL;
		ci2 = NULL;
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_FREE ) {
				if ( !ci1 ) {
					ci1 = &cgs.clientinfo[i];
				} else {
					ci2 = &cgs.clientinfo[i];
				}
			}
		}

		if ( ci1 && ci2 ) {
			s = va( "%s vs %s", ci1->name, ci2->name );
#if defined MISSIONPACK_HUD || defined IOQ3ZTM
			CG_Text_Paint(CENTER_X, 60, 0.6f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
#else
			w = CG_DrawStrlen( s );
			if ( w > 640 / GIANT_WIDTH ) {
				cw = 640 / w;
			} else {
				cw = GIANT_WIDTH;
			}
			CG_DrawStringExt( 320 - w * cw/2, 20,s, colorWhite, 
					qfalse, qtrue, cw, (int)(cw * 1.5f), 0 );
#endif
		}
	} else {
		s = cgs.gametypeName;
#if defined MISSIONPACK_HUD || defined IOQ3ZTM
		CG_Text_Paint(CENTER_X, 90, 0.6f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
#else
		w = CG_DrawStrlen( s );
		if ( w > 640 / GIANT_WIDTH ) {
			cw = 640 / w;
		} else {
			cw = GIANT_WIDTH;
		}
		CG_DrawStringExt( CENTER_X, 25,s, colorWhite, 
				qfalse, qtrue, cw, (int)(cw * 1.1f), 0 );
#endif
	}

	sec = ( sec - cg.time ) / 1000;
	if ( sec < 0 ) {
		cg.warmup = 0;
		sec = 0;
	}
	s = va( "Starts in: %i", sec + 1 );
	if ( sec != cg.warmupCount ) {
		cg.warmupCount = sec;
		switch ( sec ) {
		case 0:
			trap_S_StartLocalSound( cgs.media.count1Sound, CHAN_ANNOUNCER );
			break;
		case 1:
			trap_S_StartLocalSound( cgs.media.count2Sound, CHAN_ANNOUNCER );
			break;
		case 2:
			trap_S_StartLocalSound( cgs.media.count3Sound, CHAN_ANNOUNCER );
			break;
		default:
			break;
		}
	}

#if defined MISSIONPACK_HUD || defined IOQ3ZTM
	switch ( cg.warmupCount ) {
	case 0:
		scale = 0.54f;
		break;
	case 1:
		scale = 0.51f;
		break;
	case 2:
		scale = 0.48f;
		break;
	default:
		scale = 0.45f;
		break;
	}

	CG_Text_Paint(CENTER_X, 125, scale, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
#else
	switch ( cg.warmupCount ) {
	case 0:
		cw = 28;
		break;
	case 1:
		cw = 24;
		break;
	case 2:
		cw = 20;
		break;
	default:
		cw = 16;
		break;
	}

	CG_DrawStringExt( CENTER_X, 70, s, colorWhite, 
			qfalse, qtrue, cw, (int)(cw * 1.5), 0 );
#endif
}


/*
=====================
CG_DrawSmallWrappedText

Draw multiline text
=====================
*/
void CG_DrawSmallWrappedText(int x, int y, const char *textPtr) {
	const char *p, *start;
	char buff[1024];

	if (!textPtr || *textPtr == '\0') {
		return;
	}

	start = textPtr;
	p = strchr(textPtr, '\n');
	while (p && *p) {
		strncpy(buff, start, p-start+1);
		buff[p-start] = '\0';
		CG_DrawSmallString(x, y, buff, 1.0f );
		y += SMALLCHAR_HEIGHT + 3;
		start += p - start + 1;
		p = strchr(p+1, '\n');
	}
	CG_DrawSmallString(x, y, start, 1.0f );
}

/*
=====================
CG_DrawNotify

Draw console notify area.
=====================
*/
void CG_DrawNotify( void ) {
	int x;

#ifdef MISSIONPACK_HUD
	// voice head is being shown
	if ( cg.cur_lc->voiceTime && cg.cur_lc->clientNum != cg.cur_lc->currentVoiceClient )
		x = 72;
	else
#endif
		x = 0;

	CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);
	CG_DrawSmallWrappedText(x, 0, cg.cur_lc->consoleText);
}

//==================================================================================
#ifdef MISSIONPACK_HUD
/* 
=================
CG_DrawTimedMenus
=================
*/
void CG_DrawTimedMenus( void ) {
	if (cg.cur_lc->voiceTime) {
		int t = cg.time - cg.cur_lc->voiceTime;
		if ( t > 2500 ) {
			cg.cur_lc->voiceTime = 0;
		}
	}

	if ( cg.cur_lc->voiceTime && cg.cur_lc->clientNum != cg.cur_lc->currentVoiceClient ) {
		Menus_OpenByName("voiceMenu");
	} else {
		Menus_CloseByName("voiceMenu");
	}
}
#endif
#ifdef TA_SP
/*
=================
CG_DrawGameOver
=================
*/
void CG_DrawGameOver(void)
{
	playerState_t	*ps;

	if (cgs.gametype != GT_SINGLE_PLAYER) {
		return;
	}

	CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);

	ps = cg.cur_ps;

	// Show "Game Over"!
	if (!ps->persistant[PERS_LIVES] && !ps->persistant[PERS_CONTINUES])
	{
		int	y;
#if !defined MISSIONPACK_HUD && !defined IOQ3ZTM
		int charWidth;
#endif
		vec4_t color = {1,1,1,1};
		const char *str = "Game Over";

#if defined MISSIONPACK_HUD || defined IOQ3ZTM
		y = 120 + CG_Text_Height(str, 0.5, 0) / 2;
		CG_Text_Paint(CENTER_X, y, 0.5, color, str, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
#else
		charWidth = GIANTCHAR_WIDTH*2;
		y = 120;

		CG_DrawStringExt( CENTER_X, y, str, color, qfalse, qtrue,
			charWidth, (int)(charWidth * 1.5), 0 );
#endif
	}
}
#endif
/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D(stereoFrame_t stereoFrame)
{
#ifdef MISSIONPACK
	if (cg.cur_lc->orderPending && cg.time > cg.cur_lc->orderTime) {
		CG_CheckOrderPending( cg.cur_localClientNum );
	}
#endif
	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

	if ( cg.singleCamera ) {
		return;
	}

	if ( cg_draw2D.integer == 0 ) {
		return;
	}

#ifdef IOQ3ZTM // LETTERBOX
	// Draw black bars if needed.
	CG_DrawLetterbox();
#endif

	if ( cg.cur_ps->pm_type == PM_INTERMISSION
#ifdef TA_SP
		|| cg.cur_ps->pm_type == PM_SPINTERMISSION
#endif
	) {
		CG_DrawIntermission();
		return;
	}

/*
	if (cg.cameraMode) {
		return;
	}
*/
	if ( cg.cur_ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		CG_DrawSpectator();

		if(stereoFrame == STEREO_CENTER)
			CG_DrawCrosshair();

		CG_DrawCrosshairNames();
	} else {
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if (
#ifndef TURTLEARENA
		!cg.cur_lc->showScores &&
#endif
		cg.cur_ps->stats[STAT_HEALTH] > 0 ) {

#ifdef MISSIONPACK_HUD
			if ( cg_drawStatus.integer ) {
				CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);

				CG_DrawTimedMenus();
				Menu_PaintAll();
			}
#else
			CG_DrawStatusBar();
#endif
      
#if defined TA_WEAPSYS || defined IOQ3ZTM // SHOW_SPEED
			CG_DrawMiddleLeft();
#endif

#ifdef TURTLEARENA // NIGHTS_ITEMS
			CG_DrawScoreChain();
#endif

#ifndef TURTLEARENA // NO_AMMO_WARNINGS
			CG_DrawAmmoWarning();
#endif

#if defined MISSIONPACK && !defined TURTLEARENA // WEAPONS
			CG_DrawProxWarning();
#endif      
			if(stereoFrame == STEREO_CENTER)
				CG_DrawCrosshair();
			CG_DrawCrosshairNames();
#ifndef TA_WEAPSYS_EX
			CG_DrawWeaponSelect();
#endif
#ifndef MISSIONPACK_HUD
#ifndef TURTLEARENA
			CG_DrawHoldableItem();
#endif
#ifdef MISSIONPACK
			CG_DrawPersistantPowerup();
#endif
#endif
			CG_DrawReward();

#ifdef TA_ENTSYS // FUNC_USE
			CG_DrawUseEntity();
#endif
		}
    
#ifndef MISSIONPACK_HUD
		if ( cgs.gametype >= GT_TEAM ) {
			CG_DrawTeamInfo();
		}
#endif
	}

	CG_DrawVote();
	CG_DrawTeamVote();

	CG_DrawLagometer();

	CG_DrawVoipMeter();

#ifdef MISSIONPACK
	if (!cg_paused.integer) {
		CG_DrawUpperRight(stereoFrame);
	}
#else
	CG_DrawUpperRight(stereoFrame);
#endif

#ifndef MISSIONPACK_HUD
	CG_DrawLowerRight();
	CG_DrawLowerLeft();
#endif

	CG_DrawShaderInfo();

	CG_DrawFollow();

	// don't draw center string if scoreboard is up
	cg.cur_lc->scoreBoardShowing = CG_DrawScoreboard();
	if (!cg.cur_lc->scoreBoardShowing) {
#ifdef TA_SP
		CG_DrawGameOver();
#endif
		CG_DrawCenterString();
	}

	CG_DrawNotify();
}


static void CG_DrawTourneyScoreboard( void ) {
#ifdef MISSIONPACK_HUD
#else
	CG_DrawOldTourneyScoreboard();
#endif
}

/*
=====================
CG_FogView
=====================
*/
void CG_FogView( void ) {
	qboolean inwater;

	inwater = ( cg.refdef.rdflags & RDF_UNDERWATER );

	trap_R_GetViewFog( cg.refdef.vieworg, &cg.refdef.fogType, cg.refdef.fogColor, &cg.refdef.fogDepthForOpaque, &cg.refdef.fogDensity, inwater );

	if ( cg.refdef.fogType == FT_NONE && ( cg.refdef.fogColor[0] || cg.refdef.fogColor[1] || cg.refdef.fogColor[2] ) ) {
		// use global fog with custom color
		cg.refdef.fogType = cgs.globalFogType;
		cg.refdef.fogDepthForOpaque = cgs.globalFogDepthForOpaque;
		cg.refdef.fogDensity = cgs.globalFogDensity;
	} else if ( cg.refdef.fogType == FT_NONE ) {
		// no view fog, use global fog
		cg.refdef.fogType = cgs.globalFogType;
		cg.refdef.fogDepthForOpaque = cgs.globalFogDepthForOpaque;
		cg.refdef.fogDensity = cgs.globalFogDensity;

		VectorCopy( cgs.globalFogColor, cg.refdef.fogColor );
	}
}

/*
=====================
CG_DrawMiscGamemodels
=====================
*/
void CG_DrawMiscGamemodels( void ) {
	int i, j;
	refEntity_t ent;
	int drawn = 0;

	memset( &ent, 0, sizeof( ent ) );

	ent.reType = RT_MODEL;
	ent.nonNormalizedAxes = qtrue;

	// ydnar: static gamemodels don't project shadows
	ent.renderfx = RF_NOSHADOW;

	for ( i = 0; i < cg.numMiscGameModels; i++ ) {
		if ( cgs.miscGameModels[i].radius ) {
			if ( CG_CullPointAndRadius( cgs.miscGameModels[i].org, cgs.miscGameModels[i].radius ) ) {
				continue;
			}
		}

		if ( !trap_R_inPVS( cg.refdef.vieworg, cgs.miscGameModels[i].org ) ) {
			continue;
		}

		VectorCopy( cgs.miscGameModels[i].org, ent.origin );
		VectorCopy( cgs.miscGameModels[i].org, ent.oldorigin );
		VectorCopy( cgs.miscGameModels[i].org, ent.lightingOrigin );

/*		{
			vec3_t v;
			vec3_t vu = { 0.f, 0.f, 1.f };
			vec3_t vl = { 0.f, 1.f, 0.f };
			vec3_t vf = { 1.f, 0.f, 0.f };

			VectorCopy( cgs.miscGameModels[i].org, v );
			VectorMA( v, cgs.miscGameModels[i].radius, vu, v );
			CG_RailTrail2( NULL, cgs.miscGameModels[i].org, v );

			VectorCopy( cgs.miscGameModels[i].org, v );
			VectorMA( v, cgs.miscGameModels[i].radius, vf, v );
			CG_RailTrail2( NULL, cgs.miscGameModels[i].org, v );

			VectorCopy( cgs.miscGameModels[i].org, v );
			VectorMA( v, cgs.miscGameModels[i].radius, vl, v );
			CG_RailTrail2( NULL, cgs.miscGameModels[i].org, v );

			VectorCopy( cgs.miscGameModels[i].org, v );
			VectorMA( v, -cgs.miscGameModels[i].radius, vu, v );
			CG_RailTrail2( NULL, cgs.miscGameModels[i].org, v );

			VectorCopy( cgs.miscGameModels[i].org, v );
			VectorMA( v, -cgs.miscGameModels[i].radius, vf, v );
			CG_RailTrail2( NULL, cgs.miscGameModels[i].org, v );

			VectorCopy( cgs.miscGameModels[i].org, v );
			VectorMA( v, -cgs.miscGameModels[i].radius, vl, v );
			CG_RailTrail2( NULL, cgs.miscGameModels[i].org, v );
		}*/

		for ( j = 0; j < 3; j++ ) {
			VectorCopy( cgs.miscGameModels[i].axes[j], ent.axis[j] );
		}
		ent.hModel = cgs.miscGameModels[i].model;

		ent.customSkin = CG_AddSkinToFrame( &cgs.miscGameModels[i].skin );
		trap_R_AddRefEntityToScene( &ent );

		drawn++;
	}
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the viewport
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {
	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	// optionally draw the tournement scoreboard instead
	if ( cg.cur_ps->persistant[PERS_TEAM] == TEAM_SPECTATOR &&
		( cg.cur_ps->pm_flags & PMF_SCOREBOARD ) ) {
		CG_DrawTourneyScoreboard();
		return;
	}

	// clear around the rendered view if sized down
	CG_TileClear();

	if(stereoView != STEREO_CENTER)
		CG_DrawCrosshair3D();

	CG_DrawThirdPersonCrosshair();

	CG_PB_RenderPolyBuffers();

	CG_DrawMiscGamemodels();

	CG_FogView();

	cg.refdef.skyAlpha = cg.skyAlpha;

	// draw 3D view
	trap_R_RenderScene( &cg.refdef );

	// draw status bar and other floating elements
 	CG_Draw2D(stereoView);
}

/*
=================
CG_DrawDemoRecording
=================
*/
void CG_DrawDemoRecording( void ) {
	char	string[1024];
	char	demoName[MAX_QPATH];
	int		pos;

	if ( trap_GetDemoState() != DS_RECORDING ) {
		return;
	}
#ifdef MISSIONPACK
	if ( cg_recordSPDemo.integer ) {
		return;
	}
#endif

	trap_GetDemoName( demoName, sizeof( demoName ) );
	pos = trap_GetDemoPos();
	Com_sprintf( string, sizeof (string), "RECORDING %s: %ik", demoName, pos / 1024 );

	CG_SetScreenPlacement( PLACE_CENTER, PLACE_TOP );

	CG_DrawStringExt( ( SCREEN_WIDTH - strlen( string ) * TINYCHAR_WIDTH ) / 2, 20, string, g_color_table[ColorIndex(COLOR_WHITE)],
			qfalse, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
}

/*
=================
CG_DrawMessageMode
=================
*/
void CG_DrawMessageMode( void ) {
	if ( !( trap_Key_GetCatcher( ) & KEYCATCH_MESSAGE ) ) {
		return;
	}

	// draw the chat line
	CG_DrawBigString( 8, 232, cg.messagePrompt, 1.0f );

#ifdef IOQ3ZTM // FONT_REWRITE
	MField_Draw( &cg.messageField, 8 + ( strlen( cg.messagePrompt ) * BIGCHAR_WIDTH ), 232,
			&cgs.media.fontBig, g_color_table[ColorIndex(COLOR_WHITE)], 2 );
#else
	MField_Draw( &cg.messageField, 8 + ( strlen( cg.messagePrompt ) * BIGCHAR_WIDTH ), 232,
			BIGCHAR_WIDTH, BIGCHAR_HEIGHT, g_color_table[ColorIndex(COLOR_WHITE)] );
#endif
}

/*
=====================
CG_DrawScreen2D

Perform drawing that fills the screen, drawing over all viewports
=====================
*/
void CG_DrawScreen2D( stereoFrame_t stereoView ) {
	if ( !cg.snap ) {
		return;
	}

	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

	if ( cg_draw2D.integer == 0 ) {
		return;
	}

	CG_DrawWarmup();

#ifdef TURTLEARENA
	if ( cg.singleCamera && cgs.gametype != GT_SINGLE_PLAYER && cg.allLocalClientsAtIntermission )
#else
	if ( cg.singleCamera )
#endif
	{
		CG_DrawScoreboard();
	} else {
		CG_DrawGlobalCenterString();
	}

	CG_DrawDemoRecording();
	CG_DrawMessageMode();
}

#ifdef IOQ3ZTM // LETTERBOX
/*
=============
CG_ToggleLetterbox
=============
*/
void CG_ToggleLetterbox(qboolean onscreen, qboolean instant)
{
	if (instant)
	{
		cg.cur_lc->letterbox = onscreen;
		cg.cur_lc->letterboxTime = -1;
		return;
	}

	// Don't restart current move type.
	if (cg.cur_lc->letterbox == onscreen)
	{
		return;
	}

	cg.cur_lc->letterbox = onscreen;
	cg.cur_lc->letterboxTime = cg.time;

	// ZTM: Play sounds like in LOZ:TP
	if (cg.cur_lc->letterbox)
	{
		// move on screen
		trap_S_StartLocalSound( cgs.media.letterBoxOnSound, CHAN_LOCAL_SOUND );
	}
	else
	{
		// move off screen
		trap_S_StartLocalSound( cgs.media.letterBoxOffSound, CHAN_LOCAL_SOUND );
	}
}

/*
=============
CG_DrawLetterbox
=============
*/
void CG_DrawLetterbox(void)
{
	const int letterbox_movetime = 150; // Time in msec to move the letterbox on or off of the screen.
	const int letterbox_height = 50;
	float color[4] = {0,0,0,1}; // letterbox color
	int pixels; // height of the black bar, in pixels
	int movetime;

	// 0 = new game default, -1 = instant was used.
	if (!cg.cur_lc->letterbox && cg.cur_lc->letterboxTime <= 0)
	{
		return;
	}

	// Get time since move started.
	movetime = cg.time - cg.cur_lc->letterboxTime;
	if (movetime < letterbox_movetime)
	{
		pixels = ( ( letterbox_height / (float)letterbox_movetime ) * movetime );
		if (pixels > letterbox_height)
			pixels = letterbox_height;
	}
	else
	{
		// moving is done
		pixels = letterbox_height;
	}

	// If moving off screen, draw according.
	if (!cg.cur_lc->letterbox) {
		pixels = letterbox_height - pixels;
	}

	// Check if there is anything to draw.
	if (pixels <= 0) {
		return;
	}

	CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);
	CG_FillRect(0, 0, 640, pixels, color);
	CG_FillRect(0, 480-pixels, 640, pixels, color);
}
#endif


