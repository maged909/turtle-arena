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
// cg_drawtools.c -- helper functions called by cg_draw, cg_scoreboard, cg_info, etc
#include "cg_local.h"

static screenPlacement_e cg_horizontalPlacement = PLACE_CENTER;
static screenPlacement_e cg_verticalPlacement = PLACE_CENTER;
static screenPlacement_e cg_lastHorizontalPlacement = PLACE_CENTER;
static screenPlacement_e cg_lastVerticalPlacement = PLACE_CENTER;

/*
================
CG_SetScreenPlacement
================
*/
void CG_SetScreenPlacement(screenPlacement_e hpos, screenPlacement_e vpos)
{
	cg_lastHorizontalPlacement = cg_horizontalPlacement;
	cg_lastVerticalPlacement = cg_verticalPlacement;

	cg_horizontalPlacement = hpos;
	cg_verticalPlacement = vpos;
}

/*
================
CG_PopScreenPlacement
================
*/
void CG_PopScreenPlacement(void)
{
	cg_horizontalPlacement = cg_lastHorizontalPlacement;
	cg_verticalPlacement = cg_lastVerticalPlacement;
}

/*
================
CG_GetScreenHorizontalPlacement
================
*/
screenPlacement_e CG_GetScreenHorizontalPlacement(void)
{
	return cg_horizontalPlacement;
}

/*
================
CG_GetScreenVerticalPlacement
================
*/
screenPlacement_e CG_GetScreenVerticalPlacement(void)
{
	return cg_verticalPlacement;
}

/*
================
CG_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void CG_AdjustFrom640( float *x, float *y, float *w, float *h ) {
	int viewXBias = 0;

	if (cg.numViewports != 1 && cg.snap) {
		qboolean right = qfalse;
		qboolean down = qfalse;

		if (cg.numViewports == 2) {
			if (cg.viewport == 1) {
				down = qtrue;
			}
		}
		else if (cg.numViewports == 3 && cg.viewport == 2) {
			down = qtrue;
		}
		else if (cg.numViewports <= 4) {
			if (cg.viewport == 1 || cg.viewport == 3) {
				right = qtrue;
			}
			if (cg.viewport == 2 || cg.viewport == 3) {
				down = qtrue;
			}
		}

		if (cg.viewport != 0 && (cg.numViewports == 2 || cg.numViewports == 3) && cg_splitviewVertical.integer) {
			right = !right;
			down = !down;
		}

		if (right) {
			viewXBias = 2;
			*x += SCREEN_WIDTH;
		}
		if (down) {
			*y += SCREEN_HEIGHT;
		}
	}

	if (cg_horizontalPlacement == PLACE_STRETCH) {
		// scale for screen sizes (not aspect correct in wide screen)
		*x *= cgs.screenXScaleStretch;
		*w *= cgs.screenXScaleStretch;
	} else {
		// scale for screen sizes
		*x *= cgs.screenXScale;
		*w *= cgs.screenXScale;

		// Screen Placement
		if (cg_horizontalPlacement == PLACE_CENTER) {
			*x += cgs.screenXBias;
		} else if (cg_horizontalPlacement == PLACE_RIGHT) {
			*x += cgs.screenXBias*2;
		}

		// Offset for widescreen
		*x += cgs.screenXBias*(viewXBias);
	}

	if (cg_verticalPlacement == PLACE_STRETCH) {
		*y *= cgs.screenYScaleStretch;
		*h *= cgs.screenYScaleStretch;
	} else {
		*y *= cgs.screenYScale;
		*h *= cgs.screenYScale;

		if (cg_verticalPlacement == PLACE_CENTER) {
			*y += cgs.screenYBias;
		} else if (cg_verticalPlacement == PLACE_BOTTOM) {
			*y += cgs.screenYBias*2;
		}
	}
}

/*
================
CG_FillRect

Coordinates are 640*480 virtual values
=================
*/
void CG_FillRect( float x, float y, float width, float height, const float *color ) {
	trap_R_SetColor( color );

	CG_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 0, 0, cgs.media.whiteShader );

	trap_R_SetColor( NULL );
}

/*
================
CG_DrawSides

Coords are virtual 640x480
================
*/
void CG_DrawSides(float x, float y, float w, float h, float size) {
	CG_AdjustFrom640( &x, &y, &w, &h );
	if (cg_horizontalPlacement == PLACE_STRETCH) {
		size *= cgs.screenXScaleStretch;
	} else {
		size *= cgs.screenXScale;
	}
	trap_R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
	trap_R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, cgs.media.whiteShader );
}

void CG_DrawTopBottom(float x, float y, float w, float h, float size) {
	CG_AdjustFrom640( &x, &y, &w, &h );
	if (cg_verticalPlacement == PLACE_STRETCH) {
		size *= cgs.screenYScaleStretch;
	} else {
		size *= cgs.screenYScale;
	}
	trap_R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
	trap_R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, cgs.media.whiteShader );
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawRect( float x, float y, float width, float height, float size, const float *color ) {
	trap_R_SetColor( color );

  CG_DrawTopBottom(x, y, width, height, size);
  CG_DrawSides(x, y, width, height, size);

	trap_R_SetColor( NULL );
}



/*
================
CG_DrawPic

Coordinates are 640*480 virtual values
=================
*/
void CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader ) {
	CG_AdjustFrom640( &x, &y, &width, &height );
	trap_R_DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}

/*
================
CG_SetClipRegion
=================
*/
void CG_SetClipRegion( float x, float y, float w, float h ) {
	vec4_t clip;

	CG_AdjustFrom640( &x, &y, &w, &h );

	clip[ 0 ] = x;
	clip[ 1 ] = y;
	clip[ 2 ] = x + w;
	clip[ 3 ] = y + h;

	trap_R_SetClipRegion( clip );
}

/*
================
CG_ClearClipRegion
=================
*/
void CG_ClearClipRegion( void ) {
	trap_R_SetClipRegion( NULL );
}


#ifdef IOQ3ZTM // FONT_REWRITE
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
		CG_DrawFontStringColor( font, x+2, y+2, str, drawcolor );
	}

	if ( style & UI_INVERSE ) {
		drawcolor[0] = color[0] * 0.8;
		drawcolor[1] = color[1] * 0.8;
		drawcolor[2] = color[2] * 0.8;
		drawcolor[3] = color[3];
		CG_DrawFontStringColor( font, x, y, str, drawcolor );
		return;
	}

	if ( style & UI_PULSE ) {
		drawcolor[0] = color[0] * 0.8;
		drawcolor[1] = color[1] * 0.8;
		drawcolor[2] = color[2] * 0.8;
		drawcolor[3] = color[3];
		CG_DrawFontStringColor( font, x, y, str, color );

#ifdef TURTLEARENA // ZTM: This is like the UI Main menu text drawing, but its not.
        // ZTM: hack-ish thing to do?...
        // text_color_highlight is UI local...
		drawcolor[0] = 1.00f;//text_color_highlight[0];
		drawcolor[1] = 0.43f;//text_color_highlight[1];
		drawcolor[2] = 0.00f;//text_color_highlight[2];
#else
		drawcolor[0] = color[0];
		drawcolor[1] = color[1];
		drawcolor[2] = color[2];
#endif
		drawcolor[3] = 0.5 + 0.5 * sin( cg.time / PULSE_DIVISOR );
#ifdef TA_DATA
		CG_DrawFontStringColor( font, x, y, str, drawcolor );
#else
		CG_DrawFontStringColor( fontGlow, x, y, str, drawcolor );
#endif
		return;
	}

	CG_DrawFontStringColor( font, x, y, str, color );
}

qboolean CG_LoadFont(font_t *font, const char *ttfName, const char *shaderName, int pointSize,
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
** CG_DrawFontChar
** Characters are drawn at native screen resolution, unless adjustFrom640 is set to qtrue
*/
void CG_DrawFontChar( font_t *font, float scale, float x, float y, int ch, qboolean adjustFrom640 )
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

	useScale = Com_FontScale( font, scale );

    if (font->fontInfo.name[0]) {
		glyphInfo_t *glyph;
		float yadj;
		float xadj;

		y += Com_FontCharHeight( font, scale );

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
void CG_DrawFontStringExt( font_t *font, float scale, float x, float y, const char *string, const float *setColor, qboolean forceColor,
		qboolean noColorEscape, int drawShadow, qboolean adjustFrom640, float adjust, int limit, float *maxX )
{
	vec4_t		color;
	vec4_t		black;
	const char	*s;
	float		xx;
	int			cnt;
	int			maxChars;
	float		max;

	if (!font || !string || !*string) {
		return;
	}

	if (maxX) {
		max = *maxX;
	} else {
		max = 0;
	}

	if (adjust <= 0) {
		adjust = 0;
	}

	maxChars = strlen(string);
	if (limit > 0 && maxChars > limit) {
		maxChars = limit;
	}

	if (scale <= 0) {
		// Use auto scale based on font size
		scale = font->pointSize / 48.0f;
	}

	if (x == CENTER_X) {
		// center aligned
		float w;

		w = Com_FontStringWidthExt(font, string, scale, limit, qtrue);
		x = (SCREEN_WIDTH - w) * 0.5f;
	} else if (x < 0) {
		// right aligned, offset by x
		float w;

		w = Com_FontStringWidthExt(font, string, scale, limit, qtrue);
		x = SCREEN_WIDTH + x - w;
	}

	// draw the colored text
	s = string;
	xx = x;
	cnt = 0;
	Vector4Copy(setColor, color);
	trap_R_SetColor( color );
	while ( *s && cnt < maxChars) {
#if 1 // SCR_DrawStringExt // Is one way better then the other?
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

		if (maxX && x + Com_FontCharWidth(font, *s, 0 ) > max) {
			*maxX = 0;
			break;
		}

		if (drawShadow != 0)
		{
			float offset = drawShadow;

			// draw the drop shadow
			black[0] = black[1] = black[2] = 0;
			black[3] = color[3];
			trap_R_SetColor( black );

			CG_DrawFontChar( font, scale, xx+offset, y+offset, *s, adjustFrom640 );

			trap_R_SetColor( color );
		}

        CG_DrawFontChar( font, scale, xx, y, *s, adjustFrom640 );
        xx += Com_FontCharWidth( font, *s, scale ) + adjust * scale;
        if (maxX) {
			*maxX = xx;
        }
        cnt++;
		s++;
	}
	trap_R_SetColor( NULL );
}

void CG_DrawStringExt( int x, int y, const char *string, const float *setColor, 
		qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars )
{
	font_t *font;
	float scale;
	float maxX = x + maxChars * charWidth;

	if (charWidth >= GIANTCHAR_WIDTH && charHeight >= GIANTCHAR_HEIGHT) {
		font = &cgs.media.fontGiant;
	} else if (charWidth >= BIGCHAR_WIDTH && charHeight >= BIGCHAR_HEIGHT) {
		font = &cgs.media.fontBig;
	} else if (charWidth >= SMALLCHAR_WIDTH && charHeight >= SMALLCHAR_HEIGHT) {
		font = &cgs.media.fontSmall;
	} else {
		font = &cgs.media.fontTiny;
	}

	scale = charHeight / 48.0f;

	CG_DrawFontStringExt( font, scale, x, y, string, setColor, forceColor,
		qfalse, shadow ? 2 : 0, qtrue, 0, maxChars, maxX > x ? &maxX : NULL );
}

void CG_DrawFontString( font_t *font, int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawFontStringExt( font, 0, x, y, s, color, qfalse, qfalse, 2, qtrue, 0, 0, NULL );
}

void CG_DrawFontStringColor( font_t *font, int x, int y, const char *s, vec4_t color ) {
	CG_DrawFontStringExt( font, 0, x, y, s, color, qtrue, qfalse, 2, qtrue, 0, 0, NULL );
}

void CG_DrawGiantString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawFontStringExt( &cgs.media.fontGiant, 0, x, y, s, color, qfalse, qfalse, 2, qtrue, 0, 0, NULL );
}

void CG_DrawGiantStringColor( int x, int y, const char *s, vec4_t color ) {
	CG_DrawFontStringExt( &cgs.media.fontGiant, 0, x, y, s, color, qtrue, qfalse, 2, qtrue, 0, 0, NULL );
}

// Keep big and small for compatiblity
void CG_DrawBigString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawFontStringExt( &cgs.media.fontBig, 0, x, y, s, color, qfalse, qfalse, 2, qtrue, 0, 0, NULL );
}

void CG_DrawBigStringColor( int x, int y, const char *s, vec4_t color ) {
	CG_DrawFontStringExt( &cgs.media.fontBig, 0, x, y, s, color, qtrue, qfalse, 2, qtrue, 0, 0, NULL );
}

void CG_DrawSmallString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawFontStringExt( &cgs.media.fontSmall, 0, x, y, s, color, qfalse, qfalse, 0, qtrue, 0, 0, NULL );
}

void CG_DrawSmallStringColor( int x, int y, const char *s, vec4_t color ) {
	CG_DrawFontStringExt( &cgs.media.fontSmall, 0, x, y, s, color, qtrue, qfalse, 0, qtrue, 0, 0, NULL );
}

void CG_DrawTinyString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawFontStringExt( &cgs.media.fontTiny, 0, x, y, s, color, qfalse, qfalse, 0, qtrue, 0, 0, NULL );
}

void CG_DrawTinyStringColor( int x, int y, const char *s, vec4_t color ) {
	CG_DrawFontStringExt( &cgs.media.fontTiny, 0, x, y, s, color, qtrue, qfalse, 0, qtrue, 0, 0, NULL );
}
#else
/*
===============
CG_DrawChar

Coordinates and size in 640*480 virtual screen size
===============
*/
void CG_DrawChar( int x, int y, int width, int height, int ch ) {
	int row, col;
	float frow, fcol;
	float size;
	float	ax, ay, aw, ah;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	ax = x;
	ay = y;
	aw = width;
	ah = height;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	row = ch>>4;
	col = ch&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	trap_R_DrawStretchPic( ax, ay, aw, ah,
					   fcol, frow, 
					   fcol + size, frow + size, 
					   cgs.media.charsetShader );
}


/*
==================
CG_DrawStringExt

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void CG_DrawStringExt( int x, int y, const char *string, const float *setColor, 
		qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars ) {
	vec4_t		color;
	const char	*s;
	int			xx;
	int			cnt;

	if (maxChars <= 0)
		maxChars = 32767; // do them all!

	if (x == CENTER_X) {
		float w;

		w = CG_DrawStrlen(string) * charWidth;
		x = (SCREEN_WIDTH - w) * 0.5f;
	} else if (x < 0) {
		// right aligned, offset by x
		float w;

		w = CG_DrawStrlen(string) * charWidth;
		x = SCREEN_WIDTH + x - w;
	}

	// draw the drop shadow
	if (shadow) {
		color[0] = color[1] = color[2] = 0;
		color[3] = setColor[3];
		trap_R_SetColor( color );
		s = string;
		xx = x;
		cnt = 0;
		while ( *s && cnt < maxChars) {
			if ( Q_IsColorString( s ) ) {
				s += 2;
				continue;
			}
			CG_DrawChar( xx + 2, y + 2, charWidth, charHeight, *s );
			cnt++;
			xx += charWidth;
			s++;
		}
	}

	// draw the colored text
	s = string;
	xx = x;
	cnt = 0;
	trap_R_SetColor( setColor );
	while ( *s && cnt < maxChars) {
		if ( Q_IsColorString( s ) ) {
			if ( !forceColor ) {
				memcpy( color, g_color_table[ColorIndex(*(s+1))], sizeof( color ) );
				color[3] = setColor[3];
				trap_R_SetColor( color );
			}
			s += 2;
			continue;
		}
		CG_DrawChar( xx, y, charWidth, charHeight, *s );
		xx += charWidth;
		cnt++;
		s++;
	}
	trap_R_SetColor( NULL );
}

void CG_DrawGiantString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawStringExt( x, y, s, color, qfalse, qtrue, GIANTCHAR_WIDTH, GIANTCHAR_HEIGHT, 0 );
}

void CG_DrawGiantStringColor( int x, int y, const char *s, vec4_t color ) {
	CG_DrawStringExt( x, y, s, color, qtrue, qtrue, GIANTCHAR_WIDTH, GIANTCHAR_HEIGHT, 0 );
}

void CG_DrawBigString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawStringExt( x, y, s, color, qfalse, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0 );
}

void CG_DrawBigStringColor( int x, int y, const char *s, vec4_t color ) {
	CG_DrawStringExt( x, y, s, color, qtrue, qtrue, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0 );
}

void CG_DrawSmallString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawStringExt( x, y, s, color, qfalse, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
}

void CG_DrawSmallStringColor( int x, int y, const char *s, vec4_t color ) {
	CG_DrawStringExt( x, y, s, color, qtrue, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
}

void CG_DrawTinyString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	CG_DrawStringExt( x, y, s, color, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
}

void CG_DrawTinyStringColor( int x, int y, const char *s, vec4_t color ) {
	CG_DrawStringExt( x, y, s, color, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
}
#endif

/*
=================
CG_DrawStrlen

Returns character count, skiping color escape codes
=================
*/
int CG_DrawStrlen( const char *str ) {
	const char *s = str;
	int count = 0;

	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			s += 2;
		} else {
			count++;
			s++;
		}
	}

	return count;
}

/*
=============
CG_TileClearBox

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
static void CG_TileClearBox( int x, int y, int w, int h, qhandle_t hShader ) {
	float	s1, t1, s2, t2;

	s1 = x/64.0;
	t1 = y/64.0;
	s2 = (x+w)/64.0;
	t2 = (y+h)/64.0;
	trap_R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, hShader );
}



/*
==============
CG_TileClear

Clear around a sized down screen
==============
*/
void CG_TileClear( void ) {
	int		top, bottom, left, right;
	float		x, y, w, h;

	if (cg.cur_ps->pm_type == PM_INTERMISSION || cg_viewsize.integer >= 100) {
		return;		// full screen rendering
	}

	CG_SetScreenPlacement(PLACE_STRETCH, PLACE_STRETCH);

	// viewport coords
	x = y = 0;
	w = SCREEN_WIDTH;
	h = SCREEN_HEIGHT;
	CG_AdjustFrom640(&x, &y, &w, &h);

	// view screen coords
	top = cg.refdef.y;
	bottom = top + cg.refdef.height-1;
	left = cg.refdef.x;
	right = left + cg.refdef.width-1;

	// clear above view screen
	CG_TileClearBox( x, y, w, top, cgs.media.backTileShader );

	// clear below view screen
	CG_TileClearBox( x, bottom, w, h - bottom, cgs.media.backTileShader );

	// clear left of view screen
	CG_TileClearBox( x, top, left, bottom - top + 1, cgs.media.backTileShader );

	// clear right of view screen
	CG_TileClearBox( right, top, w - right, bottom - top + 1, cgs.media.backTileShader );

	CG_PopScreenPlacement();
}



/*
================
CG_FadeColor
================
*/
float *CG_FadeColor( int startMsec, int totalMsec ) {
	static vec4_t		color;
	int			t;

	if ( startMsec == 0 ) {
		return NULL;
	}

	t = cg.time - startMsec;

	if ( t >= totalMsec ) {
		return NULL;
	}

	// fade out
	if ( totalMsec - t < FADE_TIME ) {
		color[3] = ( totalMsec - t ) * 1.0/FADE_TIME;
	} else {
		color[3] = 1.0;
	}
	color[0] = color[1] = color[2] = 1;

	return color;
}

#ifdef TURTLEARENA // NIGHTS_ITEMS
/*
================
CG_ColorForChain
================
*/
void CG_ColorForChain(int chain, vec3_t color)
{
	int index;

	// Minus one to skip 0 so that 1,2,3,4,5 are one color, instead of 5 being a different color
	//  5 numbers in a row are the same color,
	//  limit to colors 0 though 11
	index = ((chain-1) / 5) % 12;

	switch (index)
	{
		case 0: // orange
			VectorSet( color, 1, 0.5f, 0 );
			break;
		case 1: // light blue
			VectorSet( color, 0, 0.5f, 1 );
			break;
		case 2: // green
			VectorSet( color, 0, 1, 0 );
			break;
		case 3: // cyen
			VectorSet( color, 0, 1, 1 );
			break;
		case 4: // pink
			VectorSet( color, 1, 0, 0.5f );
			break;

		case 5: // red
			VectorSet( color, 1, 0, 0 );
			break;
		case 6: // blue
			VectorSet( color, 0, 0, 1 );
			break;
		case 7: // Lime
			VectorSet( color, 0.5f, 1, 0 );
			break;
		case 8: // Vivid green
			VectorSet( color, 0, 1, 0.5f );
			break;
		case 9: // purple
			VectorSet( color, 0.5f, 0, 1 );
			break;

		case 10: // yellow
			VectorSet( color, 1, 1, 0 );
			break;
		case 11: // magenta
			VectorSet( color, 1, 0, 1 );
			break;

		default: // white
			VectorSet( color, 1, 1, 1 );
			break;
	}
}
#endif

/*
================
CG_TeamColor
================
*/
float *CG_TeamColor( int team ) {
	static vec4_t	red = {1, 0.2f, 0.2f, 1};
	static vec4_t	blue = {0.2f, 0.2f, 1, 1};
	static vec4_t	other = {1, 1, 1, 1};
	static vec4_t	spectator = {0.7f, 0.7f, 0.7f, 1};

	switch ( team ) {
	case TEAM_RED:
		return red;
	case TEAM_BLUE:
		return blue;
	case TEAM_SPECTATOR:
		return spectator;
	default:
		return other;
	}
}



/*
=================
CG_GetColorForHealth
=================
*/
#ifdef TURTLEARENA // NOARMOR
void CG_GetColorForHealth( int health, vec4_t hcolor ) {
#else
void CG_GetColorForHealth( int health, int armor, vec4_t hcolor ) {
	int		count;
	int		max;
#endif

	// calculate the total points of damage that can
	// be sustained at the current health / armor level
	if ( health <= 0 ) {
		VectorClear( hcolor );	// black
		hcolor[3] = 1;
		return;
	}
#ifndef TURTLEARENA // NOARMOR
	count = armor;
	max = health * ARMOR_PROTECTION / ( 1.0 - ARMOR_PROTECTION );
	if ( max < count ) {
		count = max;
	}
	health += count;
#endif

	// set the color based on health
	hcolor[0] = 1.0;
	hcolor[3] = 1.0;
	if ( health >= 100 ) {
		hcolor[2] = 1.0;
	} else if ( health < 66 ) {
		hcolor[2] = 0;
	} else {
		hcolor[2] = ( health - 66 ) / 33.0;
	}

	if ( health > 60 ) {
		hcolor[1] = 1.0;
	} else if ( health < 30 ) {
		hcolor[1] = 0;
	} else {
		hcolor[1] = ( health - 30 ) / 30.0;
	}
}

/*
=================
CG_ColorForHealth
=================
*/
void CG_ColorForHealth( vec4_t hcolor ) {

#ifdef TURTLEARENA // NOARMOR
	CG_GetColorForHealth( cg.cur_ps->stats[STAT_HEALTH], hcolor );
#else
	CG_GetColorForHealth( cg.cur_ps->stats[STAT_HEALTH], 
		cg.cur_ps->stats[STAT_ARMOR], hcolor );
#endif
}


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

#ifndef IOQ3ZTM // FONT_REWRITE
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
	float	frow;
	float	fcol;
	float	fwidth;
	float	fheight;

	// draw the colored text
	trap_R_SetColor( color );
	
	ax = x * cgs.screenXScale + cgs.screenXBias;
	ay = y * cgs.screenYScale;

	s = str;
	while ( *s )
	{
		ch = *s & 127;
		if ( ch == ' ' ) {
			ax += ((float)PROPB_SPACE_WIDTH + (float)PROPB_GAP_WIDTH)* cgs.screenXScale;
		}
		else if ( ch >= 'A' && ch <= 'Z' ) {
			ch -= 'A';
			fcol = (float)propMapB[ch][0] / 256.0f;
			frow = (float)propMapB[ch][1] / 256.0f;
			fwidth = (float)propMapB[ch][2] / 256.0f;
			fheight = (float)PROPB_HEIGHT / 256.0f;
			aw = (float)propMapB[ch][2] * cgs.screenXScale;
			ah = (float)PROPB_HEIGHT * cgs.screenYScale;
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, cgs.media.charsetPropB );
			ax += (aw + (float)PROPB_GAP_WIDTH * cgs.screenXScale);
		}
		s++;
	}

	trap_R_SetColor( NULL );
}

void UI_DrawBannerString( int x, int y, const char* str, int style, vec4_t color ) {
	const char *	s;
	int				ch;
	int				width;
	vec4_t			drawcolor;

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
#endif


int UI_ProportionalStringWidth( const char* str ) {
	const char *	s;
	int				ch;
	int				charWidth;
	int				width;

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
	return width;
}

static void UI_DrawProportionalString2( int x, int y, const char* str, vec4_t color, float sizeScale, qhandle_t charset )
{
	const char* s;
	unsigned char	ch;
	float	ax;
	float	ay;
	float	aw;
	float	ah;
	float	frow;
	float	fcol;
	float	fwidth;
	float	fheight;

	// draw the colored text
	trap_R_SetColor( color );
	
	ax = x * cgs.screenXScale + cgs.screenXBias;
	ay = y * cgs.screenYScale;

	s = str;
	while ( *s )
	{
		ch = *s & 127;
		if ( ch == ' ' ) {
			aw = (float)PROP_SPACE_WIDTH * cgs.screenXScale * sizeScale;
		} else if ( propMap[ch][2] != -1 ) {
			fcol = (float)propMap[ch][0] / 256.0f;
			frow = (float)propMap[ch][1] / 256.0f;
			fwidth = (float)propMap[ch][2] / 256.0f;
			fheight = (float)PROP_HEIGHT / 256.0f;
			aw = (float)propMap[ch][2] * cgs.screenXScale * sizeScale;
			ah = (float)PROP_HEIGHT * cgs.screenYScale * sizeScale;
			trap_R_DrawStretchPic( ax, ay, aw, ah, fcol, frow, fcol+fwidth, frow+fheight, charset );
		} else {
			aw = 0;
		}

		ax += (aw + (float)PROP_GAP_WIDTH * cgs.screenXScale * sizeScale);
		s++;
	}

	trap_R_SetColor( NULL );
}

/*
=================
UI_ProportionalSizeScale
=================
*/
float UI_ProportionalSizeScale( int style ) {
#ifdef IOQ3ZTM // FONT_REWRITE
	if (style & UI_SMALLFONT) {
		return (float)cgs.media.fontPropSmall.pointSize / (float)cgs.media.fontPropBig.pointSize;
	}
#else
	if(  style & UI_SMALLFONT ) {
		return 0.75;
	}
#endif

	return 1.00;
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

	if (style & UI_SMALLFONT) {
		font = &cgs.media.fontPropSmall;
#ifndef TA_DATA
		fontGlow = &cgs.media.fontPropGlowSmall;
#endif
	} else {
		font = &cgs.media.fontPropBig;
#ifndef TA_DATA
		fontGlow = &cgs.media.fontPropGlowBig;
#endif
	}

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
	charsetProp = cgs.media.charsetProp;
#endif

	sizeScale = UI_ProportionalSizeScale( style );

	switch( style & UI_FORMATMASK ) {
		case UI_CENTER:
			width = UI_ProportionalStringWidth( str ) * sizeScale;
			x -= width / 2;
			break;

		case UI_RIGHT:
			width = UI_ProportionalStringWidth( str ) * sizeScale;
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
		drawcolor[0] = color[0] * 0.8;
		drawcolor[1] = color[1] * 0.8;
		drawcolor[2] = color[2] * 0.8;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2( x, y, str, drawcolor, sizeScale, charsetProp );
		return;
	}

	if ( style & UI_PULSE ) {
		drawcolor[0] = color[0] * 0.8;
		drawcolor[1] = color[1] * 0.8;
		drawcolor[2] = color[2] * 0.8;
		drawcolor[3] = color[3];
		UI_DrawProportionalString2( x, y, str, color, sizeScale, charsetProp );

#ifdef TURTLEARENA // ZTM: This is like the UI Main menu text drawing, but its not.
        // ZTM: hack-ish thing to do?...
        // text_color_highlight is UI local...
		drawcolor[0] = 1.00f;//text_color_highlight[0];
		drawcolor[1] = 0.43f;//text_color_highlight[1];
		drawcolor[2] = 0.00f;//text_color_highlight[2];
#else
		drawcolor[0] = color[0];
		drawcolor[1] = color[1];
		drawcolor[2] = color[2];
#endif
		drawcolor[3] = 0.5 + 0.5 * sin( cg.time / PULSE_DIVISOR );
#ifdef TA_DATA
		UI_DrawProportionalString2( x, y, str, drawcolor, sizeScale, charsetProp );
#elif defined IOQ3ZTM // FONT_REWRITE
		UI_DrawProportionalString2( x, y, str, drawcolor, sizeScale, fontGlow->fontShader );
#else
		UI_DrawProportionalString2( x, y, str, drawcolor, sizeScale, cgs.media.charsetPropGlow );
#endif
		return;
	}

	UI_DrawProportionalString2( x, y, str, color, sizeScale, charsetProp );
}
