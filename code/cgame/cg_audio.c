/*
===========================================================================
Copyright (C) 2010-2013 Zack Middleton

This file is part of EBX Source Code.

EBX Source Code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

EBX Source Code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with EBX Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, EBX Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following
the terms and conditions of the GNU General Public License.  If not, please
request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional
terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/

#include "cg_local.h"

typedef struct
{
	char name[MAX_QPATH];

	char intro[MAX_QPATH];
	char loop[MAX_QPATH];

	float volume; // Multiply by normal volume

} musicDef_t;

#define MAX_MUSICDEFS 128
musicDef_t musicDefs[MAX_MUSICDEFS];

void CG_GetMusicForIntro( char *intro, char *loop, float *volume, float *loopVolume )
{
	int i;
	char name[MAX_QPATH];

	if (!intro || !*intro) {
		return;
	}

	// if loop is specified don't use music script?
	//if (loop && *loop && strcmp(intro, loop) != 0) {
	//	return;
	//}

	COM_StripExtension(intro, name, sizeof(name));

	for (i = 0; i < MAX_MUSICDEFS; i++) {
		if (!musicDefs[i].name[0]) {
			break;
		}

		if (Q_stricmp(musicDefs[i].name, name) == 0) {
			Q_strncpyz(intro, musicDefs[i].intro, MAX_QPATH);

			if (musicDefs[i].loop[0]) {
				Q_strncpyz(loop, musicDefs[i].loop, MAX_QPATH);
			} else {
				Q_strncpyz(loop, intro, MAX_QPATH);
			}

			if (volume) {
				*volume = musicDefs[i].volume;
			}
			if (loopVolume) {
				*loopVolume = musicDefs[i].volume;
			}
			break;
		}
	}
}

void CG_LoadMusicFile(const char *filename)
{
	char			*p;
	char			*token;
	musicDef_t		*musicDef;
	int				i;
	char			text[20000];
	int				len;
	fileHandle_t	f;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		Com_Printf("Warning: Couldn't load %s", filename );
		return;
	}
	if ( (unsigned)len >= sizeof( text ) - 1 ) {
		Com_Printf( "File %s too long\n", filename );
		trap_FS_FCloseFile( f );
		return;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	p = text;

	while (1) {
		musicDef = NULL;
		for (i = 0; i < MAX_MUSICDEFS; i++) {
			if (!musicDefs[i].name[0]) {
				musicDef = &musicDefs[i];
				break;
			}
		}

		if (!musicDef) {
			Com_Printf("Warning: Out of musicDefs in %s!\n", filename);
			return;
		}

		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		// token is name
		COM_StripExtension(token, musicDef->name, sizeof(musicDef->name));

		token = COM_ParseExt(&p, qtrue);
		if(token[0] != '{' && token[1] != '\0') {
			Com_Printf("%s:%s Missing begining bracket\n", filename, musicDef->name);
			break;
		}

		// Set defaults
		musicDef->volume = 1.0f;

		// Parse keywords
		while (1) {
			token = COM_ParseExt( &p, qtrue );
			if ( token[0] == 0 ) {
				Com_Printf("%s:%s Missing ending bracket!\n", filename, musicDef->name);
				break;
			}
			else if(token[0] == '}' && token[1] == '\0') {
				// end of musicdef
				break;
			}

			if ( Q_stricmp(token, "intro") == 0 ) {
				token = COM_ParseExt( &p, qtrue );
				if ( token[0] == 0 ) {
					Com_Printf("%s:%s Missing ending bracket!\n", filename, musicDef->name);
					break;
				}

				Q_strncpyz(musicDef->intro, token, sizeof(musicDef->intro));
				continue;
			}
			else if ( Q_stricmp(token, "loop") == 0 ) {
				token = COM_ParseExt( &p, qtrue );
				if ( token[0] == 0 ) {
					Com_Printf("%s:%s Missing ending bracket!\n", filename, musicDef->name);
					break;
				}

				Q_strncpyz(musicDef->loop, token, sizeof(musicDef->loop));
				continue;
			}
			else if ( Q_stricmp(token, "volume") == 0 ) {
				token = COM_ParseExt( &p, qtrue );
				if ( token[0] == 0 ) {
					Com_Printf("%s:%s Missing ending bracket!\n", filename, musicDef->name);
					break;
				}

				// Limit to 0.1 to 3.0...
				musicDef->volume = Com_Clamp(0.1f, 3.0f, atof(token));
				continue;
			}

			Com_Printf("%s:%s Unknown token %s!\n", filename, musicDef->name, token);
		}
		Com_DPrintf("Loaded %s from %s!\n",  musicDef->name, filename);
	}
}

void CG_InitAudio(void)
{
	char		filename[MAX_QPATH];
	char		dirlist[1024];
	char		*dirptr;
	int			numdirs;
	int			i;

	memset( musicDefs, 0, sizeof(musicDefs) );

	// Load from music/musiclist.txt and music/*.music
	CG_LoadMusicFile( "music/musiclist.txt" );

	numdirs = trap_FS_GetFileList( "music", ".music", dirlist, sizeof ( dirlist ) );
	dirptr  = dirlist;
	for ( i = 0; i < numdirs; i++, dirptr += strlen(dirptr)+1 ) {
		Com_sprintf(filename, sizeof ( filename ), "music/%s", dirptr );
		CG_LoadMusicFile( filename );
	}
}

void CG_StopMusic( void )
{
	trap_S_StopBackgroundTrack();
}

void CG_SetMusic( const char *constIntro, const char *constLoop )
{
	char	intro[MAX_QPATH];
	char	loop[MAX_QPATH];
	float	volume;
	float	loopVolume;

	if ( !constIntro || !*constIntro ) {
		CG_StopMusic();
		return;
	}

	if (!constLoop) {
		constLoop = "";
	}

	Q_strncpyz( intro, constIntro, sizeof ( intro ) );
	Q_strncpyz( loop, constLoop, sizeof ( loop ) );

	CG_GetMusicForIntro( intro, loop, &volume, &loopVolume );

	trap_S_StartBackgroundTrack( intro, loop, volume, loopVolume );
}
