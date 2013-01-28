/*
===========================================================================
Copyright (C) 2009-2010 Zack Middleton

This file is part of Turtle Arena source code.

Turtle Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Turtle Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Turtle Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"

#ifdef TA_ENTSYS
// Misc_object uses s.legsAnim
// NPC uses ps.legsAnim due to Pmove currently.
//    But it is copied to ent->s.legsAnim after Pmove.
void G_SetMiscAnim(gentity_t *ent, int anim)
{
	ent->s.legsAnim = ((ent->s.legsAnim & ANIM_TOGGLEBIT)^ANIM_TOGGLEBIT) | anim;
#ifdef TA_NPCSYS
	ent->bgNPC.npc_ps.legsAnim = ent->s.legsAnim;
#endif
}
#endif

#ifdef TA_NPCSYS
qboolean	npcRegistered[MAX_NPCS];

/*
==============
ClearRegisteredNPCs
==============
*/
void ClearRegisteredNPCs( void ) {
	memset( npcRegistered, 0, sizeof( npcRegistered ) );
}

/*
===============
RegisterNPC

The NPC will be added to the precache list
===============
*/
void RegisterNPC( bg_npcinfo_t *npc ) {
	if ( !npc ) {
		G_Error( "RegisterNPC: NULL" );
	}

	if (npcRegistered[ npc - bg_npcinfo ]) {
		return;
	}
	npcRegistered[ npc - bg_npcinfo ] = qtrue;

	if (npc->weaponGroup) {
		RegisterItem( BG_FindItemForWeapon( npc->weaponGroup ) );
	}
}

/*
===============
SaveRegisteredNPCs

Write the needed NPCs to a config string
so the client will know which ones to precache
===============
*/
void SaveRegisteredNPCs( void ) {
	char	string[MAX_NPCS+1];
	int		i;
	int		count;

	count = 0;
	for ( i = 0 ; i < MAX_NPCS ; i++ )
	{
		if ( npcRegistered[i] ) {
			count++;
			string[i] = '1';
		} else {
			string[i] = '0';
		}
	}
	string[ MAX_NPCS ] = 0;

	G_DPrintf( "%i NPCs registered\n", count );
	trap_SetConfigstring(CS_NPCS, string);
}

/*
===============
NPC_PlayerStateToEntityState
===============
*/
void NPC_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap ) {
	BG_PlayerStateToEntityState(ps, s, snap);

	s->eType = ET_NPC;
	s->legsAnim = ps->legsAnim;
}

#ifndef TA_PATHSYS
void SP_npcpath( gentity_t *self )
{
	if ( !self->targetname ) {
		G_Printf ("npcpath with no targetname at %s\n", vtos(self->s.origin));
		G_FreeEntity( self );
		return;
	}
}
#endif

void G_NPC_StartAction(gentity_t *self, gentity_t *other, npcAction_e action)
{
	switch (action)
	{
		case NACT_IDLE:
			break;

		case NACT_FOLLOW:
			break;

		case NACT_ATTACK:
			break;

		case NACT_GO_TO:
			break;

		default:
			break;
	}

	self->bgNPC.action = action;
	self->bgNPC.actionTime = level.time;
	self->chain = other;
	if (other) {
		VectorCopy(other->r.currentOrigin, self->bgNPC.actionPos);
	}
}

#define CORPSEWAIT 6500 // 6.5 sec wait before start of sinking
#define SINKTIME   3000 // 3 sec to sink

void NPCBodySink( gentity_t *ent ) {
	if ( level.time - ent->timestamp > (CORPSEWAIT + SINKTIME) ) {
		G_FreeEntity( ent );
		return;
	}
	ent->nextthink = level.time + 100;
	ent->s.pos.trBase[2] -= 1;
}

static void G_NPC_Die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	if (!self->takedamage) {
		return;
	}
	self->takedamage = qfalse;

	G_SetMiscAnim(self, OBJECT_DEATH1+rand()%3);
#ifdef IOQ3ZTM_NO_COMPAT // DAMAGE_SKINS
	self->s.skinFraction = 1.0f;
#endif

	AddScore(attacker, self->r.currentOrigin, 1);

	//if (self->activator->spawnflags & MOBJF_UNSOLIDDEATH)
	{
		self->bgNPC.npc_ps.contents = 0;
		trap_LinkEntity( self );
		//G_Printf("    unsolid misc_object\n");
	}

	// Drop weapon
	if ((self->s.weapon > WP_NONE && self->s.weapon < BG_NumWeaponGroups())
		&& !(self->bgNPC.info->flags & NPCF_NODROPWEAPON))
	{
		gitem_t *item = BG_FindItemForWeapon( self->s.weapon );
		if (item) {
			Drop_Item(self, item, 0);
		}
	}

	// Death types
	switch (self->bgNPC.info->deathType)
	{
		default:
		case NPCD_NONE:
			break; // Do nothing.
		case NPCD_SINK:
			self->nextthink = level.time + CORPSEWAIT;
			self->think = NPCBodySink;
			self->timestamp = level.time;
			self->physicsObject = qtrue;
			self->physicsBounce = 0;
			break;
		// Robots explode
		case NPCD_EXPLODE:
			break; // TODO
		// Ninjas dissapear in smoke?
		case NPCD_SMOKE:
			break; // TODO
	}
}

void G_NPC_Pain( gentity_t *self, gentity_t *attacker, int damage ) {

	qboolean self_evil;
	qboolean attacker_evil;

	G_SetMiscAnim(self, OBJECT_PAIN);
	self->bgNPC.npc_ps.legsTimer = BG_AnimationTime(&self->bgNPC.info->animations[OBJECT_PAIN]);
#ifdef IOQ3ZTM_NO_COMPAT // DAMAGE_SKINS
	self->s.skinFraction = 1.0f - ((float)self->health / (float)self->bgNPC.info->health);
#endif

	if (!attacker || (!attacker->client && !attacker->bgNPC.info)) {
		return;
	}

	// Clients and allies are "good guys", everyone else is "evil"...
	self_evil = !(self->bgNPC.info && (self->bgNPC.info->flags & NPCF_ALLY));
	attacker_evil = !attacker->client && !(attacker->bgNPC.info && (attacker->bgNPC.info->flags & NPCF_ALLY));

#if 1
	// Monster infighting!
	// self good and atkr evil OR self evil and atkr good *or* evil!
	if ((!self_evil && attacker_evil) || self_evil)
#else
	// self good and atkr evil OR self evil and atkr good.
	if (self_evil != attacker_evil)
#endif
	{
		// Tell NPC to kill attacker!
		G_NPC_StartAction(self, attacker, NACT_ATTACK);
	}
}

/*
================
FinishSpawningNPC

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
void FinishSpawningNPC( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		dest;

	VectorCopy(ent->bgNPC.info->mins, ent->bgNPC.npc_ps.mins);
	VectorCopy(ent->bgNPC.info->maxs, ent->bgNPC.npc_ps.maxs);

	ent->s.eType = ET_NPC;
	ent->s.modelindex = ent->bgNPC.info - bg_npcinfo;		// store item number in modelindex
	ent->s.modelindex2 = 0; // zero indicates this isn't a dropped item

	ent->bgNPC.npc_ps.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	//ent->touch = Touch_Item;

	if ( ent->spawnflags & 1 ) {
		// suspended
		G_SetOrigin( ent, ent->s.origin );
	} else {
		// drop to floor
		VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
		trap_Trace( &tr, ent->s.origin, ent->s.mins, ent->s.maxs, dest, ent->s.number, MASK_SOLID );
		if ( tr.startsolid ) {
			G_Printf ("FinishSpawningNPC: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
			G_FreeEntity( ent );
			return;
		}

		// allow to ride movers
		ent->s.groundEntityNum = tr.entityNum;

		G_SetOrigin( ent, tr.endpos );
	}

	ent->health = ent->bgNPC.info->health;
	if (ent->health) {
		ent->takedamage = qtrue;
	}

	ent->die = G_NPC_Die;
	ent->pain = G_NPC_Pain;

	G_SetMiscAnim(ent, OBJECT_IDLE);
#ifdef IOQ3ZTM_NO_COMPAT // DAMAGE_SKINS
	ent->s.skinFraction = 0.0f;
#endif

	ent->bgNPC.npc_ps.viewheight = ent->bgNPC.info->viewheight;

	VectorCopy(ent->s.pos.trBase, ent->bgNPC.npc_ps.origin);
	VectorCopy(ent->s.pos.trDelta, ent->bgNPC.npc_ps.velocity);
	VectorCopy(ent->s.apos.trBase, ent->bgNPC.npc_ps.viewangles);
	ent->bgNPC.npc_ps.commandTime = level.time;

	// NPCs can target a path to follow
	if (ent->target)
	{
#ifdef TA_PATHSYS
		if (G_SetupPath(ent, ent->target) != PATH_ERROR)
		{
			// Path is okay to use.
			G_NPC_StartAction(ent, ent->nextTrain, NACT_GO_TO);
		}
#endif
	}

	NPC_PlayerStateToEntityState( &ent->bgNPC.npc_ps, &ent->s, qtrue );

	trap_LinkEntity (ent);
}

/*
================
Use_NPC

Spawn the NPC
================
*/
void Use_NPC( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ((ent->spawnflags & 2)
		&& ent->s.eType != ET_NPC)
	{
		// some movers spawn on the second frame, so delay item
		// spawns until the third frame so they can ride trains
		ent->nextthink = level.time + FRAMETIME * 2;
		ent->think = FinishSpawningNPC;
	}
}

/*
============
G_SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void G_SpawnNPC (gentity_t *ent, bg_npcinfo_t *npc) {
	//G_SpawnFloat( "random", "0", &ent->random );
	//G_SpawnFloat( "wait", "0", &ent->wait );

	RegisterNPC( npc );
	ent->bgNPC.info = npc;

	ent->use = Use_NPC;
	if ( !(ent->spawnflags & 2) )
	{
		// some movers spawn on the second frame, so delay item
		// spawns until the third frame so they can ride trains
		ent->nextthink = level.time + FRAMETIME * 2;
		ent->think = FinishSpawningNPC;
	}
	VectorCopy( ent->s.angles, ent->s.apos.trBase );

	ent->s.weapon = ent->bgNPC.info->weaponGroup;
}

/*
================
G_RunNPC
================
*/
void G_RunNPC( gentity_t *ent )
{
	pmove_t		pm;
	usercmd_t	ucmd;
	int oldEventSequence;

	G_RunThink( ent );

	// Return if dead?

	Com_Memset(&ucmd, 0, sizeof (ucmd));
	ucmd.serverTime = level.time;

	// Think here (setup ucmd)
	switch (ent->bgNPC.action)
	{
		case NACT_IDLE:
			break;

		case NACT_FOLLOW:
			break;

		case NACT_ATTACK:
			break;

		case NACT_GO_TO:
			break;

		default:
			break;
	}

	// Don't accel if OBJECT_PAIN or OBJECTLAND.

	if ( ent->health <= 0 ) {
		ent->bgNPC.npc_ps.pm_type = PM_DEAD;
	} else {
		ent->bgNPC.npc_ps.pm_type = PM_NORMAL;
	}

	ent->bgNPC.npc_ps.gravity = g_gravity.value;
	//ent->bgNPC.npc_ps.clientNum = ent->s.number; // ZTM: Why?

	ent->bgNPC.npc_ps.speed = g_speed.value;

	// set up for pmove
	oldEventSequence = ent->bgNPC.npc_ps.eventSequence;

	memset (&pm, 0, sizeof(pm));

	pm.ps = &ent->bgNPC.npc_ps;
	pm.cmd = ucmd;

#ifdef TURTLEARENA // NO_BODY_TRACE
	if ( pm.ps->pm_type == PM_DEAD ) {
		pm.ps->powerups[PW_FLIGHT] = 0;
	}

	pm.tracemask = MASK_PLAYERSOLID;
#else
	if ( pm.ps->pm_type == PM_DEAD ) {
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
		pm.ps->powerups[PW_FLIGHT] = 0;
	}
	else {
		pm.tracemask = MASK_PLAYERSOLID;
	}
#endif
	pm.trace = trap_Trace;
	pm.pointcontents = trap_PointContents;
	pm.debugLevel = g_debugMove.integer;
	pm.noFootsteps = ( g_dmflags.integer & DF_NO_FOOTSTEPS ) > 0;

	pm.pmove_fixed = pmove_fixed.integer;
	pm.pmove_msec = pmove_msec.integer;
	pm.npc = &ent->bgNPC;

	Pmove (&pm);

	// save results of pmove
	if ( ent->bgNPC.npc_ps.eventSequence != oldEventSequence ) {
		ent->eventTime = level.time;
	}
	NPC_PlayerStateToEntityState( &ent->bgNPC.npc_ps, &ent->s, qtrue );

	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );

	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;

	trap_LinkEntity (ent);

	VectorCopy( ent->bgNPC.npc_ps.origin, ent->r.currentOrigin );
}
#endif
