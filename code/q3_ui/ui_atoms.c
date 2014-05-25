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
/**********************************************************************
	UI_ATOMS.C

	User interface building blocks and support functions.
**********************************************************************/
#include "ui_local.h"

uiStatic_t		uis;
qboolean		m_entersound;		// after a frame, so caching won't disrupt the sound

/*
=================
UI_MaxSplitView
=================
*/
int UI_MaxSplitView(void) {
	return uis.maxSplitView;
}

/*
=================
UI_NumLocalClients
=================
*/
int UI_NumLocalClients(void) {
	int numLocalClients = 0;
	int i;

	for (i = 0; i < UI_MaxSplitView(); i++) {
		if (cg.localClients[i].clientNum >= 0 && cg.localClients[i].clientNum < MAX_CLIENTS) {
			numLocalClients++;
		}
	}

	return numLocalClients;
}

/*
=================
UI_PushMenu
=================
*/
void UI_PushMenu( menuframework_s *menu )
{
	int				i;
	menucommon_s*	item;

	// avoid stacking menus invoked by hotkeys
	for (i=0 ; i<uis.menusp ; i++)
	{
		if (uis.stack[i] == menu)
		{
			uis.menusp = i;
			break;
		}
	}

	if (i == uis.menusp)
	{
		if (uis.menusp >= MAX_MENUDEPTH)
			trap_Error("UI_PushMenu: menu stack overflow");

		uis.stack[uis.menusp++] = menu;
	}

	uis.activemenu = menu;

	// default cursor position
	menu->cursor      = 0;
	menu->cursor_prev = 0;

	m_entersound = qtrue;

	trap_Mouse_SetState( 0, ( trap_Mouse_GetState( 0 ) & ~MOUSE_CLIENT ) | MOUSE_CGAME );
	trap_Key_SetCatcher( KEYCATCH_UI );

	// force first available item to have focus
	for (i=0; i<menu->nitems; i++)
	{
		item = (menucommon_s *)menu->items[i];
		if (!(item->flags & (QMF_GRAYED|QMF_MOUSEONLY|QMF_INACTIVE)))
		{
			menu->cursor_prev = -1;
			Menu_SetCursor( menu, i );
			break;
		}
	}

	uis.firstdraw = qtrue;
}

/*
=================
UI_PopMenu
=================
*/
void UI_PopMenu (void)
{
	trap_S_StartLocalSound( menu_out_sound, CHAN_LOCAL_SOUND );

	uis.menusp--;

	if (uis.menusp < 0)
		trap_Error ("UI_PopMenu: menu stack underflow");

	if (uis.menusp) {
		uis.activemenu = uis.stack[uis.menusp-1];
		uis.firstdraw = qtrue;
	}
	else {
		UI_ForceMenuOff ();
	}
}

void UI_ForceMenuOff (void)
{
	uis.menusp     = 0;
	uis.activemenu = NULL;

	trap_Mouse_SetState( 0, ( trap_Mouse_GetState( 0 ) & ~MOUSE_CGAME ) | MOUSE_CLIENT );
	trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
	trap_Cvar_SetValue( "cl_paused", 0 );
}

/*
=================
UI_LerpColor
=================
*/
void UI_LerpColor(vec4_t a, vec4_t b, vec4_t c, float t)
{
	int i;

	// lerp and clamp each component
	for (i=0; i<4; i++)
	{
		c[i] = a[i] + t*(b[i]-a[i]);
		if (c[i] < 0)
			c[i] = 0;
		else if (c[i] > 1.0)
			c[i] = 1.0;
	}
}

#ifdef IOQ3ZTM // FONT_REWRITE
/*
=================
UI_DrawFontBannerString
=================
*/
void UI_DrawFontBannerString( font_t *font, int x, int y, const char* str, int style, vec4_t color ) {
	vec4_t			drawcolor;
	int				width;

	switch( style & UI_FORMATMASK ) {
		case UI_CENTER:
			width = Com_FontStringWidth(font, str, 0);
			x -= width / 2;
			break;

		case UI_RIGHT:
			width = Com_FontStringWidth(font, str, 0);
			x -= width;
			break;

		case UI_LEFT:
		default:
			break;
	}

	if ( style & UI_DROPSHADOW ) {
		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
		drawcolor[3] = color[3];
		UI_DrawFontStringColor( font, x+2, y+2, str, drawcolor );
	}

	UI_DrawFontStringColor( font, x, y, str, color );
}

/*
=================
UI_DrawFontProportionalString
=================
*/
void UI_DrawFontProportionalString( font_t *font,
#ifndef TA_DATA
				font_t *fontGlow,
#endif
				int x, int y, const char* str, int style, vec4_t color )
{
	vec4_t	drawcolor;
	int		width;

	switch( style & UI_FORMATMASK ) {
		case UI_CENTER:
			width = Com_FontStringWidth(font, str, 0);
			x -= width / 2;
			break;

		case UI_RIGHT:
			width = Com_FontStringWidth(font, str, 0);
			x -= width;
			break;

		case UI_LEFT:
		default:
			break;
	}

	if ( style & UI_DROPSHADOW ) {
		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
		drawcolor[3] = color[3];
		UI_DrawFontStringColor( font, x+2, y+2, str, drawcolor );
	}

	if ( style & UI_INVERSE ) {
		drawcolor[0] = color[0] * 0.8;
		drawcolor[1] = color[1] * 0.8;
		drawcolor[2] = color[2] * 0.8;
		drawcolor[3] = color[3];
		UI_DrawFontStringColor( font, x, y, str, drawcolor );
		return;
	}

	if ( style & UI_PULSE ) {
		drawcolor[0] = color[0] * 0.8;
		drawcolor[1] = color[1] * 0.8;
		drawcolor[2] = color[2] * 0.8;
		drawcolor[3] = color[3];
		UI_DrawFontStringColor( font, x, y, str, color );

#ifdef TURTLEARENA
		drawcolor[0] = text_color_highlight[0];
		drawcolor[1] = text_color_highlight[1];
		drawcolor[2] = text_color_highlight[2];
#else
		drawcolor[0] = color[0];
		drawcolor[1] = color[1];
		drawcolor[2] = color[2];
#endif
		drawcolor[3] = 0.5 + 0.5 * sin( uis.realtime / PULSE_DIVISOR );
#ifdef TA_DATA
		UI_DrawFontStringColor( font, x, y, str, drawcolor );
#else
		UI_DrawFontStringColor( fontGlow, x, y, str, drawcolor );
#endif
		return;
	}

	UI_DrawFontStringColor( font, x, y, str, color );
}

/*
=================
UI_DrawString
=================
*/
void UI_DrawString( int x, int y, const char* str, int style, vec4_t color )
{
	vec4_t	newcolor;
	vec4_t	lowlight;
	float	*drawcolor;
	vec4_t	dropcolor;
	float	width;
	font_t	*font;

	if( !str ) {
		return;
	}

	if ((style & UI_BLINK) && ((uis.realtime/BLINK_DIVISOR) & 1))
		return;

	font = UI_FontForStyle( style );

	if (style & UI_PULSE)
	{
		lowlight[0] = 0.8*color[0];
		lowlight[1] = 0.8*color[1];
		lowlight[2] = 0.8*color[2];
		lowlight[3] = 0.8*color[3];
		UI_LerpColor(color,lowlight,newcolor,0.5+0.5*sin(uis.realtime/PULSE_DIVISOR));
		drawcolor = newcolor;
	}	
	else
		drawcolor = color;

	switch( style & UI_FORMATMASK ) {
		case UI_CENTER:
			width = Com_FontStringWidth(font, str, 0);
			x -= width / 2;
			break;

		case UI_RIGHT:
			width = Com_FontStringWidth(font, str, 0);
			x -= width;
			break;

		case UI_LEFT:
		default:
			break;
	}

	if ( style & UI_DROPSHADOW )
	{
		dropcolor[0] = dropcolor[1] = dropcolor[2] = 0;
		dropcolor[3] = drawcolor[3];
		UI_DrawFontStringColor( font, x+2, y+2, str, dropcolor );
	}

	UI_DrawFontStringColor( font, x, y, str, drawcolor );
}

/*
==================
UI_FontForStyle
==================
*/
font_t *UI_FontForStyle( int style ) {
	font_t *font;

	if (style & UI_SMALLFONT)
		font = &uis.fontSmall;
	else if (style & UI_GIANTFONT)
		font = &uis.fontGiant;
	else
		font = &uis.fontBig;
	
	return font;
}

/*
==================
UI_ProportionalFontForStyle
==================
*/
font_t *UI_ProportionalFontForStyle( int style ) {
	font_t *font;

	if (style & UI_SMALLFONT)
		font = &uis.fontPropSmall;
	else
		font = &uis.fontPropBig;
	
	return font;
}

/*
==================
UI_ProportionalGlowFontForStyle
==================
*/
font_t *UI_ProportionalGlowFontForStyle( int style ) {
#ifdef TA_DATA
	return UI_ProportionalFontForStyle( style );
#else
	font_t *font;

	if (style & UI_SMALLFONT)
		font = &uis.fontPropGlowSmall;
	else
		font = &uis.fontPropGlowBig;

	return font;
#endif
}

/*
==================
UI_LoadFont
==================
*/
qboolean UI_LoadFont(font_t *font, const char *ttfName, const char *shaderName, int pointSize,
			int shaderCharWidth, float fontKerning)
{
	font->fontInfo.name[0] = 0;
	font->fontShader = 0;
	font->pointSize = pointSize;
	font->kerning = fontKerning;

	font->shaderCharWidth = shaderCharWidth;

	if (ttfName[0] != '\0') {
		trap_R_RegisterFont(ttfName, pointSize, &font->fontInfo);

		if (font->fontInfo.name[0]) {
			return qtrue;
		}
	}

	if (shaderName[0] != '\0') {
		font->fontShader = trap_R_RegisterShaderNoMip(shaderName);

		if (font->fontShader) {
			return qtrue;
		}
	}

	return qfalse;
}

/*
** UI_DrawFontChar
** Characters are drawn at native screen resolution, unless adjustFrom640 is set to qtrue
*/
void UI_DrawFontChar( font_t *font, float x, float y, int ch, qboolean adjustFrom640 )
{
	float	ax, ay, aw, ah;
	float	useScale;

	if (!font) {
		return;
	}

	ch &= 0xff;

    if (ch == ' ') {
		return;
	}

#if 0 // SCR_DrawChar
	if ( y < -font->pointSize ) {
		return;
	}
#endif

	useScale = Com_FontScale( font, 0 );

    if (font->fontInfo.name[0]) {
		glyphInfo_t *glyph;
		float yadj;
		float xadj;

		y += Com_FontCharHeight( font, 0 );

		glyph = &font->fontInfo.glyphs[ch];

		yadj = useScale * glyph->top;
		xadj = useScale * glyph->left;

		ax = x+xadj;
		ay = y-yadj;
		aw = useScale * glyph->imageWidth;
		ah = useScale * glyph->imageHeight;

		if (adjustFrom640) {
			CG_AdjustFrom640( &ax, &ay, &aw, &ah );
		}

		trap_R_DrawStretchPic( ax, ay, aw, ah,
						   glyph->s, glyph->t,
						   glyph->s2, glyph->t2,
						   glyph->glyph );
    } else {
		int row, col;
		float frow, fcol;
		float size;

#if 1 // SCR_DrawChar
		if ( y < -font->pointSize ) {
			return;
		}
#endif

		size = useScale * font->pointSize;

		ax = x;
		ay = y;
		aw = size;
		ah = size;

		if (adjustFrom640) {
			CG_AdjustFrom640( &ax, &ay, &aw, &ah );
		}

		row = ch>>4;
		col = ch&15;

		frow = row*0.0625;
		fcol = col*0.0625;
		size = 0.0625;

		trap_R_DrawStretchPic( ax, ay, aw, ah,
						   fcol, frow,
						   fcol + size, frow + size,
						   font->fontShader );
    }
}

/*
==================
CG_DrawFontStringExt

Draws a multi-colored string with a optional drop shadow, optionally forcing
to a fixed color.
==================
*/
void UI_DrawFontStringExt( font_t *font, float x, float y, const char *string, const float *setColor, qboolean forceColor,
		qboolean noColorEscape, qboolean drawShadow, qboolean adjustFrom640, int maxChars )
{
	vec4_t		color;
	const char	*s;
	float		xx;
	int			cnt;

	if (maxChars <= 0)
		maxChars = 32767; // do them all!

	if (drawShadow) // ZTM: Should this be in font_t? It is not in SCR_DrawSmallStringExt, but is in SCR_Draw(Big)StringExt
	{
		// draw the drop shadow
		color[0] = color[1] = color[2] = 0;
		color[3] = setColor[3];
		trap_R_SetColor( color );
		s = string;
		xx = x;
		while ( *s ) {
			if ( !noColorEscape && Q_IsColorString( s ) ) {
				s += 2;
				continue;
			}
			UI_DrawFontChar( font, xx+2, y+2, *s, adjustFrom640 );
			xx += Com_FontCharWidth( font, *s, 0 );
			s++;
		}
	}

	// draw the colored text
	s = string;
	xx = x;
	cnt = 0;
	trap_R_SetColor( setColor );
	while ( *s && cnt < maxChars) {
#if 1 // SCR_DrawStringExt // Is one way better then the other? Do they do the same thing?
		if ( !noColorEscape && Q_IsColorString( s ) ) {
			if ( !forceColor ) {
				Com_Memcpy( color, g_color_table[ColorIndex(*(s+1))], sizeof( color ) );
				color[3] = setColor[3];
				trap_R_SetColor( color );
			}
			s += 2;
			continue;
		}
#else // SCR_DrawSmallStringExt
		if ( Q_IsColorString( s ) ) {
			if ( !forceColor ) {
				Com_Memcpy( color, g_color_table[ColorIndex(*(s+1))], sizeof( color ) );
				color[3] = setColor[3];
				trap_R_SetColor( color );
			}
			if ( !noColorEscape ) {
				s += 2;
				continue;
			}
		}
#endif
        UI_DrawFontChar( font, xx, y, *s, adjustFrom640 );
        xx += Com_FontCharWidth( font, *s, 0 );
        cnt++;
		s++;
	}
	trap_R_SetColor( NULL );
}

void UI_DrawFontString( font_t *font, int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	UI_DrawFontStringExt( font, x, y, s, color, qfalse, qfalse, qtrue, qtrue, 0 );
}

void UI_DrawFontStringColor( font_t *font, int x, int y, const char *s, vec4_t color ) {
	UI_DrawFontStringExt( font, x, y, s, color, qtrue, qfalse, qtrue, qtrue, 0 );
}
#endif

/*
=================
UI_DrawProportionalString2
=================
*/
static int	propMap[128][3] = {
{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},

{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},
{0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1}, {0, 0, -1},

{0, 0, PROP_SPACE_WIDTH},		// SPACE
{11, 122, 7},	// !
{154, 181, 14},	// "
{55, 122, 17},	// #
{79, 122, 18},	// $
{101, 122, 23},	// %
{153, 122, 18},	// &
{9, 93, 7},		// '
{207, 122, 8},	// (
{230, 122, 9},	// )
{177, 122, 18},	// *
{30, 152, 18},	// +
{85, 181, 7},	// ,
{34, 93, 11},	// -
{110, 181, 6},	// .
{130, 152, 14},	// /

{22, 64, 17},	// 0
{41, 64, 12},	// 1
{58, 64, 17},	// 2
{78, 64, 18},	// 3
{98, 64, 19},	// 4
{120, 64, 18},	// 5
{141, 64, 18},	// 6
{204, 64, 16},	// 7
{162, 64, 17},	// 8
{182, 64, 18},	// 9
{59, 181, 7},	// :
{35,181, 7},	// ;
{203, 152, 14},	// <
{56, 93, 14},	// =
{228, 152, 14},	// >
{177, 181, 18},	// ?

{28, 122, 22},	// @
{5, 4, 18},		// A
{27, 4, 18},	// B
{48, 4, 18},	// C
{69, 4, 17},	// D
{90, 4, 13},	// E
{106, 4, 13},	// F
{121, 4, 18},	// G
{143, 4, 17},	// H
{164, 4, 8},	// I
{175, 4, 16},	// J
{195, 4, 18},	// K
{216, 4, 12},	// L
{230, 4, 23},	// M
{6, 34, 18},	// N
{27, 34, 18},	// O

{48, 34, 18},	// P
{68, 34, 18},	// Q
{90, 34, 17},	// R
{110, 34, 18},	// S
{130, 34, 14},	// T
{146, 34, 18},	// U
{166, 34, 19},	// V
{185, 34, 29},	// W
{215, 34, 18},	// X
{234, 34, 18},	// Y
{5, 64, 14},	// Z
{60, 152, 7},	// [
{106, 151, 13},	// '\'
{83, 152, 7},	// ]
{128, 122, 17},	// ^
{4, 152, 21},	// _

{134, 181, 5},	// '
{5, 4, 18},		// A
{27, 4, 18},	// B
{48, 4, 18},	// C
{69, 4, 17},	// D
{90, 4, 13},	// E
{106, 4, 13},	// F
{121, 4, 18},	// G
{143, 4, 17},	// H
{164, 4, 8},	// I
{175, 4, 16},	// J
{195, 4, 18},	// K
{216, 4, 12},	// L
{230, 4, 23},	// M
{6, 34, 18},	// N
{27, 34, 18},	// O

{48, 34, 18},	// P
{68, 34, 18},	// Q
{90, 34, 17},	// R
{110, 34, 18},	// S
{130, 34, 14},	// T
{146, 34, 18},	// U
{166, 34, 19},	// V
{185, 34, 29},	// W
{215, 34, 18},	// X
{234, 34, 18},	// Y
{5, 64, 14},	// Z
{153, 152, 13},	// {
{11, 181, 5},	// |
{180, 152, 13},	// }
{79, 93, 17},	// ~
{0, 0, -1}		// DEL
};

static int propMapB[26][3] = {
{11, 12, 33},
{49, 12, 31},
{85, 12, 31},
{120, 12, 30},
{156, 12, 21},
{183, 12, 21},
{207, 12, 32},

{13, 55, 30},
{49, 55, 13},
{66, 55, 29},
{101, 55, 31},
{135, 55, 21},
{158, 55, 40},
{204, 55, 32},

{12, 97, 31},
{48, 97, 31},
{82, 97, 30},
{118, 97, 30},
{153, 97, 30},
{185, 97, 25},
{213, 97, 30},

{11, 139, 32},
{42, 139, 51},
{93, 139, 32},
{126, 139, 31},
{158, 139, 25},
};

#define PROPB_GAP_WIDTH		4
#define PROPB_SPACE_WIDTH	12
#define PROPB_HEIGHT		36

/*
=================
UI_DrawBannerString
=================
*/
static void UI_DrawBannerString2( int x, int y, const char* str, vec4_t color )
{
	const char* s;
	unsigned char	ch;
	float	ax;
	float	ay;
	float	aw;
	float	ah;
	float	agapwidth;
	float	aspacewidth;
	float	frow;
	float	fcol;
	float	fwidth;
	float	fheight;

	// draw the colored text
	trap_R_SetColor( color );
	
	ax = x;
	ay = y;
	agapwidth = PROPB_GAP_WIDTH;
	ah = PROPB_HEIGHT;
	CG_AdjustFrom640( &ax, &ay, &agapwidth, &ah );

	aspacewidth = PROPB_SPACE_WIDTH;
	CG_AdjustFrom640( NULL, NULL, &aspacewidth, NULL );

	s = str;
	while ( *s )
	{
		ch = *s & 127;
		if ( ch == ' ' ) {
			aw = aspacewidth;
		}
		else if ( ch >= 'A' && ch <= 'Z' ) {
			ch -= 'A';
			fcol = (float)propMapB[ch][0] / 256.0f;
			frow = (float)propMapB[ch][1] / 256.0f;
			fwidth = (float)propMapB[ch][2] / 256.0f;
			fheight = (float)PROPB_HEIGHT / 256.0f;
			aw = (float)propMapB[ch][2];
			CG_AdjustFrom640( NULL, NULL, &aw, NULL );
#ifdef IOQ3ZTM // FONT_REWRITE
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, uis.fontBanner.fontShader );
#else
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, uis.charsetPropB );
#endif
		}

		ax += (aw + agapwidth);
		s++;
	}

	trap_R_SetColor( NULL );
}

void UI_DrawBannerString( int x, int y, const char* str, int style, vec4_t color ) {
	const char *	s;
	int				ch;
	int				width;
	vec4_t			drawcolor;

#ifdef IOQ3ZTM // FONT_REWRITE
	if (uis.fontBanner.fontInfo.name[0]) {
		UI_DrawFontBannerString(&uis.fontBanner, x, y, str, style, color);
		return;
	}
#endif

	// find the width of the drawn text
	s = str;
	width = 0;
	while ( *s ) {
		ch = *s;
		if ( ch == ' ' ) {
			width += PROPB_SPACE_WIDTH;
		}
		else if ( ch >= 'A' && ch <= 'Z' ) {
			width += propMapB[ch - 'A'][2] + PROPB_GAP_WIDTH;
		}
		s++;
	}
	width -= PROPB_GAP_WIDTH;

	switch( style & UI_FORMATMASK ) {
		case UI_CENTER:
			x -= width / 2;
			break;

		case UI_RIGHT:
			x -= width;
			break;

		case UI_LEFT:
		default:
			break;
	}

	if ( style & UI_DROPSHADOW ) {
		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
		drawcolor[3] = color[3];
		UI_DrawBannerString2( x+2, y+2, str, drawcolor );
	}

	UI_DrawBannerString2( x, y, str, color );
}


/*
=================
UI_ProportionalSizeScale
=================
*/
float UI_ProportionalSizeScale( int style ) {
#ifdef IOQ3ZTM // FONT_REWRITE
	if (style & UI_SMALLFONT) {
		return (float)uis.fontPropSmall.pointSize / (float)uis.fontPropBig.pointSize;
	}
#else
	if(  style & UI_SMALLFONT ) {
		return PROP_SMALL_SIZE_SCALE;
	}
#endif

	return 1.00;
}

/*
=================
UI_ProportionalStringWidth
=================
*/
int UI_ProportionalStringWidth( const char* str, int style ) {
	const char *	s;
	int				ch;
	int				charWidth;
	int				width;
#ifdef IOQ3ZTM // FONT_REWRITE
	font_t *font;

	font = UI_ProportionalFontForStyle( style );

	if (font->fontInfo.name[0]) {
		return Com_FontStringWidth(font, str, 0);
	}
#endif

	s = str;
	width = 0;
	while ( *s ) {
		ch = *s & 127;
		charWidth = propMap[ch][2];
		if ( charWidth != -1 ) {
			width += charWidth;
			width += PROP_GAP_WIDTH;
		}
		s++;
	}

	width -= PROP_GAP_WIDTH;
	return width * UI_ProportionalSizeScale(style);
}

static void UI_DrawProportionalString2( int x, int y, const char* str, vec4_t color, float sizeScale, qhandle_t charset )
{
	const char* s;
	unsigned char	ch;
	float	ax;
	float	ay;
	float	aw = 0;
	float	ah;
	float	agapwidth;
	float	aspacewidth;
	float	frow;
	float	fcol;
	float	fwidth;
	float	fheight;

	// draw the colored text
	trap_R_SetColor( color );
	
	ax = x;
	ay = y;
	agapwidth = PROP_GAP_WIDTH * sizeScale;
	ah = PROP_HEIGHT * sizeScale;
	CG_AdjustFrom640( &ax, &ay, &agapwidth, &ah );

	aspacewidth = PROP_SPACE_WIDTH * sizeScale;
	CG_AdjustFrom640( NULL, NULL, &aspacewidth, NULL );

	s = str;
	while ( *s )
	{
		ch = *s & 127;
		if ( ch == ' ' ) {
			aw = aspacewidth;
		}
		else if ( propMap[ch][2] != -1 ) {
			fcol = (float)propMap[ch][0] / 256.0f;
			frow = (float)propMap[ch][1] / 256.0f;
			fwidth = (float)propMap[ch][2] / 256.0f;
			fheight = (float)PROP_HEIGHT / 256.0f;
			aw = (float)propMap[ch][2] * sizeScale;
			CG_AdjustFrom640( NULL, NULL, &aw, NULL );
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, charset );
		}

		ax += (aw + agapwidth);
		s++;
	}

	trap_R_SetColor( NULL );
}

/*
=================
UI_DrawProportionalString
=================
*/
void UI_DrawProportionalString( int x, int y, const char* str, int style, vec4_t color ) {
	vec4_t	drawcolor;
	int		width;
	float	sizeScale;
	qhandle_t charsetProp;
#ifdef IOQ3ZTM // FONT_REWRITE
	font_t *font;
#ifndef TA_DATA
	font_t *fontGlow;
#endif

	font = UI_ProportionalFontForStyle( style );
#ifndef TA_DATA
	fontGlow = UI_ProportionalGlowFontForStyle( style );
#endif

	if (font->fontInfo.name[0]) {
		UI_DrawFontProportionalString(font,
#ifndef TA_DATA
				fontGlow,
#endif
				x, y, str, style, color);
		return;
	}

	charsetProp = font->fontShader;
#else
	charsetProp = uis.charsetProp;
#endif

	sizeScale = UI_ProportionalSizeScale( style );

	switch( style & UI_FORMATMASK ) {
		case UI_CENTER:
			width = UI_ProportionalStringWidth( str, style );
			x -= width / 2;
			break;

		case UI_RIGHT:
			width = UI_ProportionalStringWidth( str, style );
			x -= width;
			break;

		case UI_LEFT:
		default:
			break;
	}

	if ( style & UI_DROPSHADOW ) {
		drawcolor[0] = drawcolor[1] = drawcolor[2] = 0;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2( x+2, y+2, str, drawcolor, sizeScale, charsetProp );
	}

	if ( style & UI_INVERSE ) {
		drawcolor[0] = color[0] * 0.7;
		drawcolor[1] = color[1] * 0.7;
		drawcolor[2] = color[2] * 0.7;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2( x, y, str, drawcolor, sizeScale, charsetProp );
		return;
	}

	if ( style & UI_PULSE ) {
		drawcolor[0] = color[0] * 0.7;
		drawcolor[1] = color[1] * 0.7;
		drawcolor[2] = color[2] * 0.7;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2( x, y, str, color, sizeScale, charsetProp );

#ifdef TURTLEARENA // ZTM: Main menu text drawing.
        // ZTM: hack-ish thing to do?...
		drawcolor[0] = text_color_highlight[0];
		drawcolor[1] = text_color_highlight[1];
		drawcolor[2] = text_color_highlight[2];
#else
		drawcolor[0] = color[0];
		drawcolor[1] = color[1];
		drawcolor[2] = color[2];
#endif
		drawcolor[3] = 0.5 + 0.5 * sin( uis.realtime / PULSE_DIVISOR );
#ifdef TA_DATA
		UI_DrawProportionalString2( x, y, str, drawcolor, sizeScale, charsetProp );
#elif defined IOQ3ZTM // FONT_REWRITE
		UI_DrawProportionalString2( x, y, str, drawcolor, sizeScale, fontGlow->fontShader );
#else
		UI_DrawProportionalString2( x, y, str, drawcolor, sizeScale, uis.charsetPropGlow );
#endif
		return;
	}

	UI_DrawProportionalString2( x, y, str, color, sizeScale, charsetProp );
}

/*
=================
UI_DrawProportionalString_Wrapped
=================
*/
void UI_DrawProportionalString_AutoWrapped( int x, int y, int xmax, int ystep, const char* str, int style, vec4_t color ) {
	int width;
	char *s1,*s2,*s3;
	char c_bcp;
	char buf[1024];

	if (!str || str[0]=='\0')
		return;
	
	Q_strncpyz(buf, str, sizeof(buf));
	s1 = s2 = s3 = buf;

	while (1) {
		do {
			s3++;
		} while (*s3!=' ' && *s3!='\0');
		c_bcp = *s3;
		*s3 = '\0';
		width = UI_ProportionalStringWidth(s1, style);
		*s3 = c_bcp;
		if (width > xmax) {
			if (s1==s2)
			{
				// fuck, don't have a clean cut, we'll overflow
				s2 = s3;
			}
			*s2 = '\0';
			UI_DrawProportionalString(x, y, s1, style, color);
			y += ystep;
			if (c_bcp == '\0')
      {
        // that was the last word
        // we could start a new loop, but that wouldn't be much use
        // even if the word is too long, we would overflow it (see above)
        // so just print it now if needed
        s2++;
        if (*s2 != '\0') // if we are printing an overflowing line we have s2 == s3
          UI_DrawProportionalString(x, y, s2, style, color);
				break; 
      }
			s2++;
			s1 = s2;
			s3 = s2;
		}
		else
		{
			s2 = s3;
			if (c_bcp == '\0') // we reached the end
			{
				UI_DrawProportionalString(x, y, s1, style, color);
				break;
			}
		}
	}
}

#ifndef IOQ3ZTM // FONT_REWRITE
/*
=================
UI_DrawString2
=================
*/
static void UI_DrawString2( int x, int y, const char* str, vec4_t color, int charw, int charh )
{
	const char* s;
	char	ch;
	int forceColor = qfalse; //APSFIXME;
	vec4_t	tempcolor;
	float	ax;
	float	ay;
	float	aw;
	float	ah;
	float	frow;
	float	fcol;

	if (y < -charh)
		// offscreen
		return;

	// draw the colored text
	trap_R_SetColor( color );
	
	ax = x;
	ay = y;
	aw = charw;
	ah = charh;

	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	s = str;
	while ( *s )
	{
		if ( Q_IsColorString( s ) )
		{
			if ( !forceColor )
			{
				memcpy( tempcolor, g_color_table[ColorIndex(s[1])], sizeof( tempcolor ) );
				tempcolor[3] = color[3];
				trap_R_SetColor( tempcolor );
			}
			s += 2;
			continue;
		}

		ch = *s & 255;
		if (ch != ' ')
		{
			frow = (ch>>4)*0.0625;
			fcol = (ch&15)*0.0625;
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol + 0.0625, frow + 0.0625, uis.charset );
		}

		ax += aw;
		s++;
	}

	trap_R_SetColor( NULL );
}

/*
=================
UI_DrawString
=================
*/
void UI_DrawString( int x, int y, const char* str, int style, vec4_t color )
{
	int		len;
	int		charw;
	int		charh;
	vec4_t	newcolor;
	vec4_t	lowlight;
	float	*drawcolor;
	vec4_t	dropcolor;

	if( !str ) {
		return;
	}

	if ((style & UI_BLINK) && ((uis.realtime/BLINK_DIVISOR) & 1))
		return;

	if (style & UI_SMALLFONT)
	{
		charw =	SMALLCHAR_WIDTH;
		charh =	SMALLCHAR_HEIGHT;
	}
	else if (style & UI_GIANTFONT)
	{
		charw =	GIANTCHAR_WIDTH;
		charh =	GIANTCHAR_HEIGHT;
	}
	else
	{
		charw =	BIGCHAR_WIDTH;
		charh =	BIGCHAR_HEIGHT;
	}

	if (style & UI_PULSE)
	{
		lowlight[0] = 0.8*color[0]; 
		lowlight[1] = 0.8*color[1];
		lowlight[2] = 0.8*color[2];
		lowlight[3] = 0.8*color[3];
		UI_LerpColor(color,lowlight,newcolor,0.5+0.5*sin(uis.realtime/PULSE_DIVISOR));
		drawcolor = newcolor;
	}	
	else
		drawcolor = color;

	switch (style & UI_FORMATMASK)
	{
		case UI_CENTER:
			// center justify at x
			len = strlen(str);
			x   = x - len*charw/2;
			break;

		case UI_RIGHT:
			// right justify at x
			len = strlen(str);
			x   = x - len*charw;
			break;

		default:
			// left justify at x
			break;
	}

	if ( style & UI_DROPSHADOW )
	{
		dropcolor[0] = dropcolor[1] = dropcolor[2] = 0;
		dropcolor[3] = drawcolor[3];
		UI_DrawString2(x+2,y+2,str,dropcolor,charw,charh);
	}

	UI_DrawString2(x,y,str,drawcolor,charw,charh);
}
#endif

/*
=================
UI_DrawChar
=================
*/
int UI_DrawChar( int x, int y, int ch, int style, vec4_t color )
{
	char	buff[2];
#ifdef IOQ3ZTM // FONT_REWRITE
	font_t *font;

	font = UI_FontForStyle( style );

	// Remap bitmap font symbols
	if (font->fontInfo.name[0])
	{
		switch (ch)
		{
			case 10: // Overwrite text
				ch = '|';
				break;

			case 11: // Insert text
				ch = '_';
				break;

			case 13: // Select marker
				ch = '>';
				break;

			// Old slider symbols
			case 128: // Left end
			case 129: // Intervolt
			case 130: // Right end
			case 131: // Slider cursor
				break;

			default:
				break;
		}
	}
#endif

	buff[0] = ch;
	buff[1] = '\0';

	UI_DrawString( x, y, buff, style, color );

#ifdef IOQ3ZTM // FONT_REWRITE
	return Com_FontStringWidth(font, buff, 0);
#else
	if (style & UI_SMALLFONT)
		return SMALLCHAR_WIDTH;
	else if (style & UI_GIANTFONT)
		return GIANTCHAR_WIDTH;
	else
		return BIGCHAR_WIDTH;
#endif
}

qboolean UI_IsFullscreen( void ) {
	if ( uis.activemenu && ( trap_Key_GetCatcher() & KEYCATCH_UI )
#ifdef TA_DATA
		&& !trap_Cvar_VariableValue("cl_paused")
#endif
		)
	{
		return uis.activemenu->fullscreen;
	}

	return qfalse;
}

void UI_SetActiveMenu( uiMenuCommand_t menu ) {
	switch ( menu ) {
	case UIMENU_NONE:
		UI_ForceMenuOff();
		return;
	case UIMENU_MAIN:
		UI_MainMenu();
		return;
	case UIMENU_INGAME:
		/*
		//GRank
		UI_RankingsMenu();
		return;
		*/
		trap_Cvar_SetValue( "cl_paused", 1 );
		UI_InGameMenu();
		return;
		
	case UIMENU_TEAM:
	case UIMENU_POSTGAME:
	default:
#ifndef NDEBUG
	  Com_Printf("UI_SetActiveMenu: bad enum %d\n", menu );
#endif
	  break;
	}
}

/*
=================
UI_KeyEvent
=================
*/
void UI_KeyEvent( int key, qboolean down ) {
	sfxHandle_t		s;

	if (!uis.activemenu) {
		return;
	}

	if (!down) {
		return;
	}

	if (uis.activemenu->key)
		s = uis.activemenu->key( key );
	else
		s = Menu_DefaultKey( uis.activemenu, key );

	if ((s > 0) && (s != menu_null_sound))
		trap_S_StartLocalSound( s, CHAN_LOCAL_SOUND );
}

/*
=================
UI_MouseEvent
=================
*/
void UI_MouseEvent( int localClientNum, int dx, int dy )
{
	float			ax, ay, aw, ah;
	int				xbias, ybias;
	int				i;
	menucommon_s*	m;

	if (localClientNum != 0) {
		// q3_ui currently only supports one cursor
		return;
	}

	if (!uis.activemenu)
		return;

	ax = 0;
	ay = 0;
	aw = 1;
	ah = 1;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	xbias = ax/aw;
	ybias = ay/ah;

	// update mouse screen position
	uis.cursorx = Com_Clamp( -xbias, SCREEN_WIDTH+xbias, uis.cursorx + dx );
	uis.cursory = Com_Clamp( -ybias, SCREEN_HEIGHT+ybias, uis.cursory + dy );

	// region test the active menu items
	for (i=0; i<uis.activemenu->nitems; i++)
	{
		m = (menucommon_s*)uis.activemenu->items[i];

		if (m->flags & (QMF_GRAYED|QMF_INACTIVE))
			continue;

		if ((uis.cursorx < m->left) ||
			(uis.cursorx > m->right) ||
			(uis.cursory < m->top) ||
			(uis.cursory > m->bottom))
		{
			// cursor out of item bounds
			continue;
		}

		// set focus to item at cursor
		if (uis.activemenu->cursor != i)
		{
			Menu_SetCursor( uis.activemenu, i );
			((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor_prev]))->flags &= ~QMF_HASMOUSEFOCUS;

			if ( !(((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor]))->flags & QMF_SILENT ) ) {
				trap_S_StartLocalSound( menu_move_sound, CHAN_LOCAL_SOUND );
			}
		}

		((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor]))->flags |= QMF_HASMOUSEFOCUS;
		return;
	}  

	if (uis.activemenu->nitems > 0) {
		// out of any region
		((menucommon_s*)(uis.activemenu->items[uis.activemenu->cursor]))->flags &= ~QMF_HASMOUSEFOCUS;
	}
}

/*
=================
UI_GetCursorPos
=================
*/
void UI_GetCursorPos( int localClientNum, int *x, int *y )
{
	if (localClientNum != 0) {
		// ui currently only supports one cursor
		if ( x ) *x = 0;
		if ( y ) *y = 0;
	}

	if ( x ) *x = uis.cursorx;
	if ( y ) *y = uis.cursory;
}

/*
=================
UI_SetCursorPos
=================
*/
void UI_SetCursorPos( int localClientNum, int x, int y )
{
	if (localClientNum != 0) {
		// ui currently only supports one cursor
		return;
	}

	uis.cursorx = x;
	uis.cursory = y;
	UI_MouseEvent( localClientNum, 0, 0 );
}

/*
=================
UI_Cache
=================
*/
void UI_Cache_f( void ) {
	MainMenu_Cache();
	InGame_Cache();
	InSelectPlayer_Cache();
#ifdef TA_MISC // INGAME_SERVER_MENU
	InServer_Cache();
#endif
	ConfirmMenu_Cache();
#ifdef TA_MISC
	UI_PlayerSetupMenu_Cache();
#else
	PlayerModel_Cache();
	PlayerSettings_Cache();
#endif
	Controls_Cache();
	UI_Joystick_Cache();
	Demos_Cache();
	UI_CinematicsMenu_Cache();
	Preferences_Cache();
	ServerInfo_Cache();
#ifdef TA_MISC
	UI_Multiplayer_Cache();
#endif
	SpecifyServer_Cache();
	ArenaServers_Cache();
	StartServer_Cache();
#ifndef TA_SP
	ServerOptions_Cache();
#endif
	DriverInfo_Cache();
	GraphicsOptions_Cache();
	UI_DisplayOptionsMenu_Cache();
	UI_SoundOptionsMenu_Cache();
	UI_NetworkOptionsMenu_Cache();
#ifdef TA_SP
	UI_SPMenu_Cache();
	UI_SPPlayerMenu_Cache();
	LoadGame_Cache();
#else
	UI_SPLevelMenu_Cache();
	UI_SPSkillMenu_Cache();
#endif
	UI_SPPostgameMenu_Cache();
	TeamMain_Cache();
	UI_AddBots_Cache();
	UI_RemoveBots_Cache();
	UI_SetupMenu_Cache();
	UI_SelectPlayer_Cache();
//	UI_LoadConfig_Cache();
//	UI_SaveConfigMenu_Cache();
#ifndef TA_SP
	UI_BotSelectMenu_Cache();
#endif
	UI_ModsMenu_Cache();

}

#ifdef TA_SP // Save/load
/*
==================
UI_GetSaveGameInfo

See save_t in g_save.c for more info.
==================
*/
qboolean UI_GetSaveGameInfo(fileHandle_t f, char *loadmap, byte *pMaxclients, byte *pLocalClients) {
	char buffer[MAX_QPATH];
	byte version;

	trap_FS_Read( &version, 1, f );

	trap_Cvar_VariableStringBuffer( "g_saveVersions", buffer, sizeof (buffer) );

	if ( !strstr( va( "%d", version ), buffer ) ) {
		// Version not supported
		return qfalse;
	}

	trap_FS_Read( loadmap, MAX_QPATH, f );
	trap_FS_Read( pMaxclients, 1, f );
	trap_FS_Read( pLocalClients, 1, f );

	return qtrue;
}

/*
==================
UI_LoadGame_f

Load a saved game
==================
*/
static void UI_LoadGame_f(void) {
	char savegame[MAX_QPATH];
	char filename[MAX_QPATH];
	char loadmap[MAX_QPATH];
	char currentMap[MAX_QPATH];
	byte maxclients;
	byte localClients;
	fileHandle_t f;

	// Set savefile name.
	if ( trap_Argc() < 2 ) {
		Com_Printf("Usage: loadgame <savename>\n");
		return;
	} else {
		trap_Argv( 1, savegame, sizeof( savegame ) );
	}

	Com_sprintf( filename, sizeof (filename), "saves/%s.sav", savegame );

    if ( trap_FS_FOpenFile( filename, &f, FS_READ) < 0 ) {
		Com_Printf("Failed to open savefile (%s)!\n", filename);
		trap_FS_FCloseFile( f );
		return;
	}

	if ( !UI_GetSaveGameInfo(f, loadmap, &maxclients, &localClients) ) {
		trap_FS_FCloseFile( f );
		Com_Printf( "Warning: Unsupported savefile (%s)\n", savegame );
		return;
	}

	trap_FS_FCloseFile( f );

	trap_Cvar_SetValue( "sv_maxclients", maxclients );
	trap_Cvar_SetValue( "cl_localClients", localClients );

	// Set filename for game
	trap_Cvar_SetValue( "savegame_loading", 1 );
	trap_Cvar_Set( "savegame_filename", filename );

	trap_Cvar_VariableStringBuffer( "mapname", currentMap, sizeof (currentMap) );

	if ( trap_Cvar_VariableValue( "sv_running" ) && currentMap[0] && Q_stricmp( loadmap, currentMap ) == 0) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "map_restart 0\n" );
	} else {
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "map %s\n", loadmap ) );
	}
}

/*
==================
UI_Arcade_f
==================
*/
static void UI_Arcade_f( void ) {
	UI_StartServerMenu( qfalse );
}

/*
==================
UI_SPComplete_f
==================
*/
static void UI_SPComplete_f( void ) {
	trap_Cvar_Set( "com_errorMessage", "Completed single player platformer test!" );
}

/*
==================
UI_SPGameOver_f
==================
*/
static void UI_SPGameOver_f( void ) {
	trap_Cvar_Set( "com_errorMessage", "Game Over" );
}
#endif

consoleCommand_t	ui_commands[] = {
#ifdef TA_SP
	{ "loadgame", UI_LoadGame_f, 0 },
	{ "singleplayermenu", UI_SPMenu_f, 0 },
	{ "arcade", UI_Arcade_f, 0 },
	{ "sp_complete", UI_SPComplete_f, 0 },
	{ "sp_gameover", UI_SPGameOver_f, 0 },
#else
	{ "levelselect", UI_SPLevelMenu_f, 0 },
#endif
	{ "postgame", UI_SPPostgameMenu_f, CMD_INGAME },
	{ "ui_cache", UI_Cache_f, 0 },
	{ "ui_cinematics", UI_CinematicsMenu_f, 0 },
	{ "ui_teamOrders", UI_TeamOrdersMenu_f, CMD_INGAME },
	{ "iamacheater", UI_SPUnlock_f, 0 },
#ifndef TA_SP
	{ "iamamonkey", UI_SPUnlockMedals_f, 0 }
#endif
};

int ui_numCommands = ARRAY_LEN( ui_commands );

/*
=================
UI_ConsoleCommand

update frame time, commands are executed by CG_ConsoleCommand
=================
*/
void UI_ConsoleCommand( int realTime ) {
	uis.frametime = realTime - uis.realtime;
	uis.realtime = realTime;
}

/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown( void ) {
}

/*
=================
UI_Init
=================
*/
void UI_Init( qboolean inGameLoad, int maxSplitView ) {
	uis.maxSplitView = Com_Clamp(1, MAX_SPLITVIEW, maxSplitView);
	
	UI_RegisterCvars();

#ifdef TA_SP // Save/load
	if ( !inGameLoad ) {
		trap_Cvar_Set("savegame_loading", "0");
		trap_Cvar_Set("savegame_filename", "");
	}
#endif

	UI_InitGameinfo();

	// initialize the menu system
	Menu_Cache();

	uis.activemenu = NULL;
	uis.menusp     = 0;
}

#ifdef TA_DATA
/*
==========
UI_DrawPicFullScreen

Draw shader fullscreen (including in widescreen), in widescreen center shader
and repeat horizontally without changing the aspect.
==========
*/
void UI_DrawPicFullScreen(qhandle_t hShader) {
	float x = 0, y = 0, w = cgs.glconfig.vidWidth, h = cgs.glconfig.vidHeight;
	const float picX = SCREEN_WIDTH;
	const float picY = SCREEN_HEIGHT;
	float scale = h / picY; // scale shader to fit vertically
	float s1, t1, s2, t2;
	float sDelta, tDelta;

	// Get aspect correct coords
	s1 = x/(picX * scale);
	t1 = y/(picY * scale);
	s2 = (x+w)/(picX * scale);
	t2 = (y+h)/(picY * scale);

	// Center pic
	sDelta = (1.0f - s2) / 2.0f;
	tDelta = (1.0f - t2) / 2.0f;
	s1 += sDelta;
	s2 += sDelta;
	t1 += tDelta;
	t2 += tDelta;

	trap_R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, hShader );
}
#endif

/*
=================
UI_Refresh
=================
*/
void UI_Refresh( int realtime )
{
	uis.frametime = realtime - uis.realtime;
	uis.realtime  = realtime;

	if ( !( trap_Key_GetCatcher() & KEYCATCH_UI ) ) {
		return;
	}

	UI_UpdateCvars();

	if ( uis.activemenu )
	{
		if (uis.activemenu->fullscreen)
		{
#ifdef TA_DATA
			// draw the background
			if (trap_Cvar_VariableValue( "cl_paused" )) {
				UI_DrawPicFullScreen( uis.menuBackInGameShader );
			}
			else {
				UI_DrawPicFullScreen( uis.menuBackShader );
			}
#else
			CG_ClearViewport();

			// draw the background
			if( uis.activemenu->showlogo ) {
				CG_DrawPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackShader );
			}
			else {
				CG_DrawPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, uis.menuBackNoLogoShader );
			}
#endif
		}

		if (uis.activemenu->draw)
			uis.activemenu->draw();
		else
			Menu_Draw( uis.activemenu );

		if( uis.firstdraw ) {
			int i;

			for (i = 0; i < UI_MaxSplitView(); ++i) {
				UI_MouseEvent( i, 0, 0 );
			}
			uis.firstdraw = qfalse;
		}
	}

	// draw cursor
	trap_R_SetColor( NULL );
	CG_DrawPic( uis.cursorx-16, uis.cursory-16, 32, 32, uis.cursor);

#ifndef NDEBUG
	if (uis.debug)
	{
		// cursor coordinates
		UI_DrawString( 0, 0, va("(%d,%d)",uis.cursorx,uis.cursory), UI_LEFT|UI_SMALLFONT, colorRed );
	}
#endif

	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	if (m_entersound)
	{
		trap_S_StartLocalSound( menu_in_sound, CHAN_LOCAL_SOUND );
		m_entersound = qfalse;
	}
}

qboolean UI_CursorInRect (int x, int y, int width, int height)
{
	if (uis.cursorx < x ||
		uis.cursory < y ||
		uis.cursorx > x+width ||
		uis.cursory > y+height)
		return qfalse;

	return qtrue;
}
