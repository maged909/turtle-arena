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

/*****************************************************************************
 * name:		ai_char.c
 *
 * desc:		bot characters
 *
 * $Archive: /source/code/game/ai_char.c $
 *
 *****************************************************************************/

#include "g_local.h"
#include "../botlib/botlib.h"
#include "../botlib/be_aas.h"
#include "../botlib/be_ai_chat.h"
//
#include "ai_char.h"
#include "ai_ea.h"
#include "ai_gen.h"
#include "ai_goal.h"
#include "ai_move.h"
#include "ai_weap.h"
#include "ai_weight.h"
//
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_chat.h"
#include "ai_cmd.h"
#include "ai_vcmd.h"
#include "ai_dmnet.h"
#include "ai_team.h"
//
#include "chars.h"				//characteristics
#include "inv.h"				//indexes into the inventory
#include "syn.h"				//synonyms
#include "match.h"				//string matching types and vars

#define MAX_CHARACTERISTICS		80

#define CT_INTEGER				1
#define CT_FLOAT				2
#define CT_STRING				3

#define DEFAULT_CHARACTER		"bots/default_c.c"

//characteristic value
union cvalue
{
	int integer;
	float _float;
	char *string;
};
//a characteristic
typedef struct bot_characteristic_s
{
	char type;						//characteristic type
	union cvalue value;				//characteristic value
} bot_characteristic_t;

//a bot character
typedef struct bot_character_s
{
	char filename[MAX_QPATH];
	float skill;
	bot_characteristic_t c[MAX_CHARACTERISTICS];
} bot_character_t;

bot_character_t botcharacters[MAX_CLIENTS + 1];

//========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//========================================================================
bot_character_t *BotCharacterFromHandle(int handle)
{
	if (handle <= 0 || handle > MAX_CLIENTS)
	{
		BotAI_Print(PRT_FATAL, "character handle %d out of range\n", handle);
		return NULL;
	} //end if
	if (!botcharacters[handle].skill)
	{
		BotAI_Print(PRT_FATAL, "invalid character %d\n", handle);
		return NULL;
	} //end if
	return &botcharacters[handle];
} //end of the function BotCharacterFromHandle
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotDumpCharacter(bot_character_t *ch)
{
	int i;

	G_Printf("%s\n", ch->filename);
	G_Printf("skill %.1f\n", ch->skill);
	G_Printf("{\n");
	for (i = 0; i < MAX_CHARACTERISTICS; i++)
	{
		switch(ch->c[i].type)
		{
			case CT_INTEGER: G_Printf(" %4d %d\n", i, ch->c[i].value.integer); break;
			case CT_FLOAT: G_Printf(" %4d %f\n", i, ch->c[i].value._float); break;
			case CT_STRING: G_Printf(" %4d %s\n", i, ch->c[i].value.string); break;
		} //end case
	} //end for
	G_Printf("}\n");
} //end of the function BotDumpCharacter
//========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//========================================================================
void BotFreeCharacterStrings(bot_character_t *ch)
{
#if 0 // ZTM: FIXME: No support for freeing memory
	int i;

	for (i = 0; i < MAX_CHARACTERISTICS; i++)
	{
		if (ch->c[i].type == CT_STRING)
		{
			FreeMemory(ch->c[i].value.string);
		} //end if
	} //end for
#endif
} //end of the function BotFreeCharacterStrings
//========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//========================================================================
void BotFreeCharacter2(int handle)
{
	if (handle <= 0 || handle > MAX_CLIENTS)
	{
		BotAI_Print(PRT_FATAL, "character handle %d out of range\n", handle);
		return;
	} //end if
	if (!botcharacters[handle].skill)
	{
		BotAI_Print(PRT_FATAL, "invalid character %d\n", handle);
		return;
	} //end if
	BotFreeCharacterStrings(&botcharacters[handle]);
	Com_Memset(&botcharacters[handle], 0, sizeof (bot_character_t));
} //end of the function BotFreeCharacter2
//========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//========================================================================
void BotFreeCharacter(int handle)
{
	if (!BotLibVarGetValue("bot_reloadcharacters")) return;
	BotFreeCharacter2(handle);
} //end of the function BotFreeCharacter
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotDefaultCharacteristics(bot_character_t *ch, bot_character_t *defaultch)
{
	int i;

	for (i = 0; i < MAX_CHARACTERISTICS; i++)
	{
		if (ch->c[i].type) continue;
		//
		if (defaultch->c[i].type == CT_FLOAT)
		{
			ch->c[i].type = CT_FLOAT;
			ch->c[i].value._float = defaultch->c[i].value._float;
		} //end if
		else if (defaultch->c[i].type == CT_INTEGER)
		{
			ch->c[i].type = CT_INTEGER;
			ch->c[i].value.integer = defaultch->c[i].value.integer;
		} //end else if
		else if (defaultch->c[i].type == CT_STRING)
		{
			ch->c[i].type = CT_STRING;
			ch->c[i].value.string = (char *) trap_Alloc(strlen(defaultch->c[i].value.string)+1, NULL);
			strcpy(ch->c[i].value.string, defaultch->c[i].value.string);
		} //end else if
	} //end for
} //end of the function BotDefaultCharacteristics
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
qboolean BotLoadCharacterFromFile(char *charfile, int skill, bot_character_t *ch)
{
	int indent, index, foundcharacter;
	int	source;
	pc_token_t token;

	foundcharacter = qfalse;
	//a bot character is parsed in two phases
	source = trap_PC_LoadSource(charfile, BOTFILESBASEFOLDER);
	if (!source)
	{
		BotAI_Print(PRT_ERROR, "counldn't load %s\n", charfile);
		return qfalse;
	} //end if
	strcpy(ch->filename, charfile);
	while(trap_PC_ReadToken(source, &token))
	{
		if (!strcmp(token.string, "skill"))
		{
			if (!PC_ExpectTokenType(source, TT_NUMBER, 0, &token))
			{
				trap_PC_FreeSource(source);
				BotFreeCharacterStrings(ch);
				return qfalse;
			} //end if
			if (!PC_ExpectTokenString(source, "{"))
			{
				trap_PC_FreeSource(source);
				BotFreeCharacterStrings(ch);
				return qfalse;
			} //end if
			//if it's the correct skill
			if (skill < 0 || token.intvalue == skill)
			{
				foundcharacter = qtrue;
				ch->skill = token.intvalue;
				while(PC_ExpectAnyToken(source, &token))
				{
					if (!strcmp(token.string, "}")) break;
					if (token.type != TT_NUMBER || !(token.subtype & TT_INTEGER))
					{
						PC_SourceError(source, "expected integer index, found %s", token.string);
						trap_PC_FreeSource(source);
						BotFreeCharacterStrings(ch);
						return qfalse;
					} //end if
					index = token.intvalue;
					if (index < 0 || index > MAX_CHARACTERISTICS)
					{
						PC_SourceError(source, "characteristic index out of range [0, %d]", MAX_CHARACTERISTICS);
						trap_PC_FreeSource(source);
						BotFreeCharacterStrings(ch);
						return qfalse;
					} //end if
					if (ch->c[index].type)
					{
						PC_SourceError(source, "characteristic %d already initialized", index);
						trap_PC_FreeSource(source);
						BotFreeCharacterStrings(ch);
						return qfalse;
					} //end if
					if (!PC_ExpectAnyToken(source, &token))
					{
						trap_PC_FreeSource(source);
						BotFreeCharacterStrings(ch);
						return qfalse;
					} //end if
					if (token.type == TT_NUMBER)
					{
						if (token.subtype & TT_FLOAT)
						{
							ch->c[index].value._float = token.floatvalue;
							ch->c[index].type = CT_FLOAT;
						} //end if
						else
						{
							ch->c[index].value.integer = token.intvalue;
							ch->c[index].type = CT_INTEGER;
						} //end else
					} //end if
					else if (token.type == TT_STRING)
					{
						// ZTM: FIXME: ### I think I made this be done in the engine
						//StripDoubleQuotes(token.string);
						ch->c[index].value.string = trap_Alloc(strlen(token.string)+1, NULL);
						strcpy(ch->c[index].value.string, token.string);
						ch->c[index].type = CT_STRING;
					} //end else if
					else
					{
						PC_SourceError(source, "expected integer, float or string, found %s", token.string);
						trap_PC_FreeSource(source);
						BotFreeCharacterStrings(ch);
						return qfalse;
					} //end else
				} //end if
				break;
			} //end if
			else
			{
				indent = 1;
				while(indent)
				{
					if (!PC_ExpectAnyToken(source, &token))
					{
						trap_PC_FreeSource(source);
						BotFreeCharacterStrings(ch);
						return qfalse;
					} //end if
					if (!strcmp(token.string, "{")) indent++;
					else if (!strcmp(token.string, "}")) indent--;
				} //end while
			} //end else
		} //end if
		else
		{
			PC_SourceError(source, "unknown definition %s", token.string);
			trap_PC_FreeSource(source);
			BotFreeCharacterStrings(ch);
			return qfalse;
		} //end else
	} //end while
	trap_PC_FreeSource(source);
	//
	if (!foundcharacter)
	{
		BotFreeCharacterStrings(ch);
		return qfalse;
	} //end if
	return qtrue;
} //end of the function BotLoadCharacterFromFile
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotFindCachedCharacter(char *charfile, float skill)
{
	int handle;

	for (handle = 1; handle <= MAX_CLIENTS; handle++)
	{
		if ( !botcharacters[handle].skill ) continue;
		if ( strcmp( botcharacters[handle].filename, charfile ) == 0 &&
			(skill < 0 || fabs(botcharacters[handle].skill - skill) < 0.01) )
		{
			return handle;
		} //end if
	} //end for
	return 0;
} //end of the function BotFindCachedCharacter
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotLoadCachedCharacter(char *charfile, float skill, int reload)
{
	int handle, cachedhandle, intskill;
#ifdef DEBUG
	int starttime;

	starttime = trap_Milliseconds();
#endif //DEBUG

	//find a free spot for a character
	for (handle = 1; handle <= MAX_CLIENTS; handle++)
	{
		if (!botcharacters[handle].skill) break;
	} //end for
	if (handle > MAX_CLIENTS) return 0;
	//try to load a cached character with the given skill
	if (!reload)
	{
		cachedhandle = BotFindCachedCharacter(charfile, skill);
		if (cachedhandle)
		{
			BotAI_Print(PRT_DEVELOPER, "loaded cached skill %f from %s\n", skill, charfile);
			return cachedhandle;
		} //end if
	} //end else
	//
	intskill = (int) (skill + 0.5);
	//try to load the character with the given skill
	if (BotLoadCharacterFromFile(charfile, intskill, &botcharacters[handle]))
	{
		//
		BotAI_Print(PRT_DEVELOPER, "loaded skill %d from %s\n", intskill, charfile);
#ifdef DEBUG
		BotAI_Print(PRT_DEVELOPER, "skill %d loaded in %d msec from %s\n", intskill, trap_Milliseconds() - starttime, charfile);
#endif //DEBUG
		return handle;
	} //end if
	//
	BotAI_Print(PRT_WARNING, "couldn't find skill %d in %s\n", intskill, charfile);
	//
	if (!reload)
	{
		//try to load a cached default character with the given skill
		cachedhandle = BotFindCachedCharacter(DEFAULT_CHARACTER, skill);
		if (cachedhandle)
		{
			BotAI_Print(PRT_MESSAGE, "loaded cached default skill %d from %s\n", intskill, charfile);
			return cachedhandle;
		} //end if
	} //end if
	//try to load the default character with the given skill
	if (BotLoadCharacterFromFile(DEFAULT_CHARACTER, intskill, &botcharacters[handle]))
	{
		BotAI_Print(PRT_MESSAGE, "loaded default skill %d from %s\n", intskill, charfile);
		return handle;
	} //end if
	//
	if (!reload)
	{
		//try to load a cached character with any skill
		cachedhandle = BotFindCachedCharacter(charfile, -1);
		if (cachedhandle)
		{
			BotAI_Print(PRT_DEVELOPER, "loaded cached skill %f from %s\n", botcharacters[cachedhandle].skill, charfile);
			return cachedhandle;
		} //end if
	} //end if
	//try to load a character with any skill
	if (BotLoadCharacterFromFile(charfile, -1, &botcharacters[handle]))
	{
		BotAI_Print(PRT_DEVELOPER, "loaded skill %f from %s\n", botcharacters[handle].skill, charfile);
		return handle;
	} //end if
	//
	if (!reload)
	{
		//try to load a cached character with any skill
		cachedhandle = BotFindCachedCharacter(DEFAULT_CHARACTER, -1);
		if (cachedhandle)
		{
			BotAI_Print(PRT_MESSAGE, "loaded cached default skill %f from %s\n", botcharacters[cachedhandle].skill, charfile);
			return cachedhandle;
		} //end if
	} //end if
	//try to load a character with any skill
	if (BotLoadCharacterFromFile(DEFAULT_CHARACTER, -1, &botcharacters[handle]))
	{
		BotAI_Print(PRT_MESSAGE, "loaded default skill %f from %s\n", botcharacters[handle].skill, charfile);
		return handle;
	} //end if
	//
	BotAI_Print(PRT_WARNING, "couldn't load any skill from %s\n", charfile);
	//couldn't load any character
	return 0;
} //end of the function BotLoadCachedCharacter
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotLoadCharacterSkill(char *charfile, float skill)
{
	int ch, defaultch;

	defaultch = BotLoadCachedCharacter(DEFAULT_CHARACTER, skill, qfalse);
	ch = BotLoadCachedCharacter(charfile, skill, BotLibVarGetValue("bot_reloadcharacters"));

	if (defaultch && ch)
	{
		BotDefaultCharacteristics(&botcharacters[ch], &botcharacters[defaultch]);
	} //end if

	return ch;
} //end of the function BotLoadCharacterSkill
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotInterpolateCharacters(int handle1, int handle2, float desiredskill)
{
	bot_character_t *ch1, *ch2, *out;
	int i, handle;
	float scale;

	ch1 = BotCharacterFromHandle(handle1);
	ch2 = BotCharacterFromHandle(handle2);
	if (!ch1 || !ch2)
		return 0;
	//find a free spot for a character
	for (handle = 0; handle < MAX_CLIENTS; handle++)
	{
		if (!botcharacters[handle].skill) break;
	} //end for
	if (handle > MAX_CLIENTS) return 0;
	out = &botcharacters[handle];
	out->skill = desiredskill;
	strcpy(out->filename, ch1->filename);

	scale = (float) (desiredskill - ch1->skill) / (ch2->skill - ch1->skill);
	for (i = 0; i < MAX_CHARACTERISTICS; i++)
	{
		//
		if (ch1->c[i].type == CT_FLOAT && ch2->c[i].type == CT_FLOAT)
		{
			out->c[i].type = CT_FLOAT;
			out->c[i].value._float = ch1->c[i].value._float +
								(ch2->c[i].value._float - ch1->c[i].value._float) * scale;
		} //end if
		else if (ch1->c[i].type == CT_INTEGER)
		{
			out->c[i].type = CT_INTEGER;
			out->c[i].value.integer = ch1->c[i].value.integer;
		} //end else if
		else if (ch1->c[i].type == CT_STRING)
		{
			out->c[i].type = CT_STRING;
			out->c[i].value.string = (char *) trap_Alloc(strlen(ch1->c[i].value.string)+1, NULL);
			strcpy(out->c[i].value.string, ch1->c[i].value.string);
		} //end else if
	} //end for
	return handle;
} //end of the function BotInterpolateCharacters
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotLoadCharacter(char *charfile, float skill)
{
	int firstskill, secondskill, handle;

	//make sure the skill is in the valid range
	if (skill < 1.0) skill = 1.0;
	else if (skill > 5.0) skill = 5.0;
	//skill 1, 4 and 5 should be available in the character files
	if (skill == 1.0 || skill == 4.0 || skill == 5.0)
	{
		return BotLoadCharacterSkill(charfile, skill);
	} //end if
	//check if there's a cached skill
	handle = BotFindCachedCharacter(charfile, skill);
	if (handle)
	{
		BotAI_Print(PRT_DEVELOPER, "loaded cached skill %f from %s\n", skill, charfile);
		return handle;
	} //end if
	if (skill < 4.0)
	{
		//load skill 1 and 4
		firstskill = BotLoadCharacterSkill(charfile, 1);
		if (!firstskill) return 0;
		secondskill = BotLoadCharacterSkill(charfile, 4);
		if (!secondskill) return firstskill;
	} //end if
	else
	{
		//load skill 4 and 5
		firstskill = BotLoadCharacterSkill(charfile, 4);
		if (!firstskill) return 0;
		secondskill = BotLoadCharacterSkill(charfile, 5);
		if (!secondskill) return firstskill;
	} //end else
	//interpolate between the two skills
	handle = BotInterpolateCharacters(firstskill, secondskill, skill);
	if (!handle) return 0;
#if 0 // ZTM: FIXME: add new bot logfile for game to write to?
	//write the character to the log file
	BotDumpCharacter(&botcharacters[handle]);
#endif
	//
	return handle;
} //end of the function BotLoadCharacter
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int CheckCharacteristicIndex(int character, int index)
{
	bot_character_t *ch;

	ch = BotCharacterFromHandle(character);
	if (!ch) return qfalse;
	if (index < 0 || index >= MAX_CHARACTERISTICS)
	{
		BotAI_Print(PRT_ERROR, "characteristic %d does not exist\n", index);
		return qfalse;
	} //end if
	if (!ch->c[index].type)
	{
		BotAI_Print(PRT_ERROR, "characteristic %d is not initialized\n", index);
		return qfalse;
	} //end if
	return qtrue;
} //end of the function CheckCharacteristicIndex
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
float Characteristic_Float(int character, int index)
{
	bot_character_t *ch;

	ch = BotCharacterFromHandle(character);
	if (!ch) return 0;
	//check if the index is in range
	if (!CheckCharacteristicIndex(character, index)) return 0;
	//an integer will be converted to a float
	if (ch->c[index].type == CT_INTEGER)
	{
		return (float) ch->c[index].value.integer;
	} //end if
	//floats are just returned
	else if (ch->c[index].type == CT_FLOAT)
	{
		return ch->c[index].value._float;
	} //end else if
	//cannot convert a string pointer to a float
	else
	{
		BotAI_Print(PRT_ERROR, "characteristic %d is not a float\n", index);
		return 0;
	} //end else if
//	return 0;
} //end of the function Characteristic_Float
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float Characteristic_BFloat(int character, int index, float min, float max)
{
	float value;
	bot_character_t *ch;

	ch = BotCharacterFromHandle(character);
	if (!ch) return 0;
	if (min > max)
	{
		BotAI_Print(PRT_ERROR, "cannot bound characteristic %d between %f and %f\n", index, min, max);
		return 0;
	} //end if
	value = Characteristic_Float(character, index);
	if (value < min) return min;
	if (value > max) return max;
	return value;
} //end of the function Characteristic_BFloat
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int Characteristic_Integer(int character, int index)
{
	bot_character_t *ch;

	ch = BotCharacterFromHandle(character);
	if (!ch) return 0;
	//check if the index is in range
	if (!CheckCharacteristicIndex(character, index)) return 0;
	//an integer will just be returned
	if (ch->c[index].type == CT_INTEGER)
	{
		return ch->c[index].value.integer;
	} //end if
	//floats are casted to integers
	else if (ch->c[index].type == CT_FLOAT)
	{
		return (int) ch->c[index].value._float;
	} //end else if
	else
	{
		BotAI_Print(PRT_ERROR, "characteristic %d is not an integer\n", index);
		return 0;
	} //end else if
//	return 0;
} //end of the function Characteristic_Integer
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int Characteristic_BInteger(int character, int index, int min, int max)
{
	int value;
	bot_character_t *ch;

	ch = BotCharacterFromHandle(character);
	if (!ch) return 0;
	if (min > max)
	{
		BotAI_Print(PRT_ERROR, "cannot bound characteristic %d between %d and %d\n", index, min, max);
		return 0;
	} //end if
	value = Characteristic_Integer(character, index);
	if (value < min) return min;
	if (value > max) return max;
	return value;
} //end of the function Characteristic_BInteger
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void Characteristic_String(int character, int index, char *buf, int size)
{
	bot_character_t *ch;

	ch = BotCharacterFromHandle(character);
	if (!ch) return;
	//check if the index is in range
	if (!CheckCharacteristicIndex(character, index)) return;
	//an integer will be converted to a float
	if (ch->c[index].type == CT_STRING)
	{
		strncpy(buf, ch->c[index].value.string, size-1);
		buf[size-1] = '\0';
	} //end if
	else
	{
		BotAI_Print(PRT_ERROR, "characteristic %d is not a string\n", index);
	} //end else if
} //end of the function Characteristic_String
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotShutdownCharacters(void)
{
	int handle;

	for (handle = 1; handle <= MAX_CLIENTS; handle++)
	{
		if (botcharacters[handle].skill)
		{
			BotFreeCharacter2(handle);
		} //end if
	} //end for
} //end of the function BotShutdownCharacters

