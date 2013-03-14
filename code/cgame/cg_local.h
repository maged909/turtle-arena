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
#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_types.h"
#include "../game/bg_misc.h"
#include "cg_public.h"

#ifdef IOQ3ZTM // FONT_REWRITE
//#undef TINYCHAR_WIDTH
#undef TINYCHAR_HEIGHT
#define TINYCHAR_HEIGHT (Com_FontCharHeight(&cgs.media.fontTiny, 0))

//#undef SMALLCHAR_WIDTH
#undef SMALLCHAR_HEIGHT
#define SMALLCHAR_HEIGHT (Com_FontCharHeight(&cgs.media.fontSmall, 0))

//#undef BIGCHAR_WIDTH
#undef BIGCHAR_HEIGHT
#define BIGCHAR_HEIGHT (Com_FontCharHeight(&cgs.media.fontBig, 0))

//#undef GIANTCHAR_WIDTH
#undef GIANTCHAR_HEIGHT
#define GIANTCHAR_HEIGHT (Com_FontCharHeight(&cgs.media.fontGiant, 0))
#endif

// The entire cgame module is unloaded and reloaded on each level change,
// so there is NO persistant data between levels on the client side.
// If you absolutely need something stored, it can either be kept
// by the server in the server stored userinfos, or stashed in a cvar.

#ifdef MISSIONPACK
#define CG_FONT_THRESHOLD 0.1
#endif

#define	POWERUP_BLINKS		5

#define	POWERUP_BLINK_TIME	1000
#define	FADE_TIME			200
#define	PULSE_TIME			200
#define	DAMAGE_DEFLECT_TIME	100
#define	DAMAGE_RETURN_TIME	400
#define DAMAGE_TIME			500
#define	LAND_DEFLECT_TIME	150
#define	LAND_RETURN_TIME	300
#define	STEP_TIME			200
#define	DUCK_TIME			100
#define	PAIN_TWITCH_TIME	200
#define	WEAPON_SELECT_TIME	1400
#define	ITEM_SCALEUP_TIME	1000
#ifndef TURTLEARENA // NOZOOM
#define	ZOOM_TIME			150
#endif
#define	ITEM_BLOB_TIME		200
#define	MUZZLE_FLASH_TIME	20
#define	SINK_TIME			1000		// time for fragments to sink into ground before going away
#define	ATTACKER_HEAD_TIME	10000
#define	REWARD_TIME			3000

#define	PULSE_SCALE			1.5			// amount to scale up the icons when activating

#define	MAX_STEP_CHANGE		32

#define	MAX_VERTS_ON_POLY	10
#define	MAX_MARK_POLYS		256

#define STAT_MINUS			10	// num frame for '-' stats digit

#define	ICON_SIZE			48
#define	CHAR_WIDTH			32
#define	CHAR_HEIGHT			48
#define	TEXT_ICON_SPACE		4

#define	TEAMCHAT_WIDTH		80
#define TEAMCHAT_HEIGHT		8

// very large characters
#define	GIANT_WIDTH			32
#define	GIANT_HEIGHT		48

#ifdef TA_DATA
#define	NUM_CROSSHAIRS		4
#else
#define	NUM_CROSSHAIRS		10
#endif

#define TEAM_OVERLAY_MAXNAME_WIDTH	12
#define TEAM_OVERLAY_MAXLOCATION_WIDTH	16

#ifndef TA_PLAYERSYS // Moved to bg_misc.h
typedef enum {
	FOOTSTEP_NORMAL,
	FOOTSTEP_BOOT,
	FOOTSTEP_FLESH,
	FOOTSTEP_MECH,
	FOOTSTEP_ENERGY,
	FOOTSTEP_METAL,
	FOOTSTEP_SPLASH,

	FOOTSTEP_TOTAL
} footstep_t;
#endif

typedef enum {
	IMPACTSOUND_DEFAULT,
	IMPACTSOUND_METAL,
	IMPACTSOUND_FLESH
#ifdef TA_WEAPSYS
	,IMPACTSOUND_LIGHTNING_PREDICT
#endif
} impactSound_t;

#ifdef TA_MISC // MATERIALS
// Models Per Material type
#define NUM_MATERIAL_MODELS		5
#endif

//=================================================

// player entities need to track more information
// than any other type of entity.

// note that not every player entity is a client entity,
// because corpses after respawn are outside the normal
// client numbering range

#ifndef IOQ3ZTM // LERP_FRAME_CLIENT_LESS
// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
typedef struct {
	int			oldFrame;
	int			oldFrameTime;		// time when ->oldFrame was exactly on

	int			frame;
	int			frameTime;			// time when ->frame will be exactly on

	float		backlerp;

	float		yawAngle;
	qboolean	yawing;
	float		pitchAngle;
	qboolean	pitching;

	int			animationNumber;	// may include ANIM_TOGGLEBIT
	animation_t	*animation;
	int			animationTime;		// time when the first frame of the animation will be exact
} lerpFrame_t;
#endif

#ifdef TA_WEAPSYS // MELEE_TRAIL
typedef struct
{
	float yawAngle;
	qboolean yawing;
} meleeTrail_t;

// Currently limited to one trail per-weapon
//   It would be nice for Sais to have three.
#define MAX_WEAPON_TRAILS MAX_HANDS
#endif

typedef struct {
	lerpFrame_t		legs, torso, flag;
#ifdef TA_WEAPSYS
	lerpFrame_t		barrel[MAX_HANDS];
#endif
	int				painTime;
	int				painDirection;	// flip from 0 to 1
	int				lightningFiring;

#ifdef TA_WEAPSYS // MELEE_TRAIL
	// melee weapon trails
	meleeTrail_t weaponTrails[MAX_WEAPON_TRAILS];
#endif

#ifndef TA_WEAPSYS
	int				railFireTime;
#endif

	// machinegun spinning
	float			barrelAngle;
	int				barrelTime;
	qboolean		barrelSpinning;

#ifdef TA_WEAPSYS
	// Hook grapple chain to flash origin
	vec3_t flashOrigin[MAX_HANDS];
#elif defined IOQ3ZTM
	// Hook grapple chain to flash origin
	vec3_t flashOrigin;
#endif
} playerEntity_t;

//=================================================


#ifdef TA_ENTSYS // MISC_OBJECT
// misc_object/NPC data
enum
{
	MOF_SETUP			= 1, // true if did one time setup.
};

typedef struct
{
	qhandle_t		model;
	qhandle_t		skin;

	lerpFrame_t		lerp;
	int				anim; // current animation ( may have ANIM_TOGGLEBIT )
	float			speed; // Allow speeding up the animations?

	// Sounds
	int				lastSoundFrame;
	bg_sounds_t		sounds;

	int				flags; // Special flags.
} objectEntity_t;
#endif

// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
typedef struct centity_s {
	entityState_t	currentState;	// from cg.frame
	entityState_t	nextState;		// from cg.nextFrame, if available
	qboolean		interpolate;	// true if next is valid to interpolate to
	qboolean		currentValid;	// true if cg.frame holds this entity

	int				muzzleFlashTime;	// move to playerEntity?
	int				previousEvent;
	int				teleportFlag;

	int				trailTime;		// so missile trails can handle dropped initial packets
	int				dustTrailTime;
	int				miscTime;

	int				snapShotTime;	// last time this entity was found in a snapshot

	playerEntity_t	pe;
#ifdef TA_ENTSYS // MISC_OBJECT
	bg_objectcfg_t	*objectcfg;
	objectEntity_t	oe; // misc_object/NPC data
#endif

	int				errorTime;		// decay the error from this time
	vec3_t			errorOrigin;
	vec3_t			errorAngles;
	
	qboolean		extrapolated;	// false if origin / angles is an interpolation
	vec3_t			rawOrigin;
	vec3_t			rawAngles;

	vec3_t			beamEnd;

	// exact interpolated position of entity on this frame
	vec3_t			lerpOrigin;
	vec3_t			lerpAngles;
} centity_t;


//======================================================================

// local entities are created as a result of events or predicted actions,
// and live independantly from all server transmitted entities

typedef struct markPoly_s {
	struct markPoly_s	*prevMark, *nextMark;
	int			time;
	qhandle_t	markShader;
	qboolean	alphaFade;		// fade alpha instead of rgb
	float		color[4];
	poly_t		poly;
	polyVert_t	verts[MAX_VERTS_ON_POLY];
} markPoly_t;


typedef enum {
	LE_MARK,
	LE_EXPLOSION,
	LE_SPRITE_EXPLOSION,
	LE_FRAGMENT,
	LE_MOVE_SCALE_FADE,
	LE_FALL_SCALE_FADE,
	LE_FADE_RGB,
	LE_SCALE_FADE,
	LE_SCOREPLUM,
#ifdef TURTLEARENA // NIGHTS_ITEMS
	LE_CHAINPLUM,
#endif
#ifdef IOQ3ZTM // BUBBLES
	LE_BUBBLE,
#endif
#ifdef MISSIONPACK
#ifndef TURTLEARENA // NO_KAMIKAZE_ITEM POWERS
	LE_KAMIKAZE,
	LE_INVULIMPACT,
	LE_INVULJUICED,
#endif
	LE_SHOWREFENTITY
#endif
} leType_t;

typedef enum {
	LEF_PUFF_DONT_SCALE  = 0x0001,			// do not scale size over time
	LEF_TUMBLE			 = 0x0002,			// tumble over time, used for ejecting shells
#ifndef TURTLEARENA // NO_KAMIKAZE_ITEM
	LEF_SOUND1			 = 0x0004,			// sound 1 for kamikaze
	LEF_SOUND2			 = 0x0008			// sound 2 for kamikaze
#endif
} leFlag_t;

typedef enum {
	LEMT_NONE,
	LEMT_BURN
#ifndef NOTRATEDM // No gibs.
	,LEMT_BLOOD
#endif
} leMarkType_t;			// fragment local entities can leave marks on walls

typedef enum {
	LEBS_NONE,
#ifndef NOTRATEDM // No gibs.
	LEBS_BLOOD,
#endif
	LEBS_BRASS
} leBounceSoundType_t;	// fragment local entities can make sounds on impacts

typedef struct localEntity_s {
	struct localEntity_s	*prev, *next;
	leType_t		leType;
	int				leFlags;

	int				startTime;
	int				endTime;
	int				fadeInTime;

	float			lifeRate;			// 1.0 / (endTime - startTime)

	trajectory_t	pos;
	trajectory_t	angles;

	float			bounceFactor;		// 0.0 = no bounce, 1.0 = perfect

	float			color[4];

	float			radius;

	float			light;
	vec3_t			lightColor;

	leMarkType_t		leMarkType;		// mark to leave on fragment impact
	leBounceSoundType_t	leBounceSoundType;

	refEntity_t		refEntity;		

	int				localClients; // 0 means all, else check if localClients & (1<<lc)
} localEntity_t;

//======================================================================


typedef struct {
	int				client;
	int				score;
	int				ping;
	int				time;
	int				scoreFlags;
	int				powerUps;
	int				accuracy;
#ifndef TURTLEARENA // AWARDS
	int				impressiveCount;
	int				excellentCount;
	int				guantletCount;
#endif
	int				defendCount;
	int				assistCount;
	int				captures;
	qboolean	perfect;
	int				team;
} score_t;

// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a client's configstring changes,
// usually as a result of a userinfo (name, model, etc) change
#define	MAX_CUSTOM_SOUNDS	32

#ifdef TA_WEAPSYS
// Support using "tag_weapon" for the primary weapon,
//           and "tag_flag" for the secondary weapon / flag.
// So it will allow team arena models to be ported easier?
//   and allow Turtle Arena players in Quake3/Team Arena?
enum
{
#ifdef TA_SUPPORTQ3
	TI_TAG_WEAPON = 1,
	TI_TAG_FLAG = 2,
#endif
	TI_TAG_HAND_PRIMARY = 4,
	TI_TAG_HAND_SECONDARY = 8,

	TI_TAG_WP_AWAY_PRIMARY = 16,
	TI_TAG_WP_AWAY_SECONDARY = 32,
};
#endif

#ifdef IOQ3ZTM // GHOST
#define NUM_GHOST_REFS 10
typedef struct
{
	int time;

	vec3_t		axis[3];			// rotation vectors
	qboolean	nonNormalizedAxes;	// axis are not normalized, i.e. they have scale
	int			frame;				// also used as MODEL_BEAM's diameter

	// previous data for frame interpolation
	int			oldframe;
	float		backlerp;			// 0.0 = current, 1.0 = old

	float		shaderTime;
	vec3_t		origin;

} ghostRefData_t;
#endif

typedef struct {
	qboolean		infoValid;

	char			name[MAX_QPATH];
	team_t			team;

	int				botSkill;		// 0 = not bot, 1-5 = bot

	vec3_t			color1;
	vec3_t			color2;
	
	byte c1RGBA[4];
	byte c2RGBA[4];

#ifdef TA_PLAYERSYS
	vec3_t			prefcolor2;
#endif

	int				score;			// updated by score servercmds
	int				location;		// location index for team mode
	int				health;			// you only get this info about your teammates
#ifndef TURTLEARENA // NOARMOR
	int				armor;
#endif
	int				curWeapon;

	int				handicap;
	int				wins, losses;	// in tourney mode

	int				teamTask;		// task in teamplay (offence/defence)
	qboolean		teamLeader;		// true when this is a team leader

	int				powerups;		// so can display quad/flag status

	int				medkitUsageTime;
#ifndef TURTLEARENA // POWERS
	int				invulnerabilityStartTime;
	int				invulnerabilityStopTime;
#endif
#ifdef IOQ3ZTM // GHOST
	int				ghostTime[MAX_HANDS];
	ghostRefData_t	ghostWeapon[MAX_HANDS][NUM_GHOST_REFS];
#endif

	int				breathPuffTime;

	// when clientinfo is changed, the loading of models/skins/sounds
	// can be deferred until you are dead, to prevent hitches in
	// gameplay
	char			modelName[MAX_QPATH];
	char			skinName[MAX_QPATH];
	char			headModelName[MAX_QPATH];
	char			headSkinName[MAX_QPATH];
#ifdef MISSIONPACK
	char			redTeam[MAX_TEAMNAME];
	char			blueTeam[MAX_TEAMNAME];
#endif
	qboolean		deferred;

#ifdef TA_WEAPSYS
	int				tagInfo;
#else
	qboolean		newAnims;		// true if using the new mission pack animations
#endif
#ifndef TA_PLAYERSYS
	qboolean		fixedlegs;		// true if legs yaw is always the same as torso yaw
	qboolean		fixedtorso;		// true if torso never changes yaw

	vec3_t			headOffset;		// move head in icon views
	footstep_t		footsteps;
	gender_t		gender;			// from model
#endif

	qhandle_t		legsModel;
	qhandle_t		legsSkin;

	qhandle_t		torsoModel;
	qhandle_t		torsoSkin;

	qhandle_t		headModel;
	qhandle_t		headSkin;

	qhandle_t		modelIcon;

#ifdef IOQ3ZTM // BONES
	// single model player
	qhandle_t		playerModel;
	qhandle_t		playerSkin;
#endif

#ifdef TA_PLAYERSYS
	bg_playercfg_t  playercfg;
#else
	animation_t		animations[MAX_TOTALANIMATIONS];
#endif

	sfxHandle_t		sounds[MAX_CUSTOM_SOUNDS];
} clientInfo_t;


#ifdef TA_WEAPSYS
typedef struct projectileInfo_s {
	qboolean		registered;

	qhandle_t		missileModel;
	sfxHandle_t		missileSound;
	void			(*missileTrailFunc)( centity_t *, const struct projectileInfo_s *wi );
	float			missileDlight;
	vec3_t			missileDlightColor;
	int				missileRenderfx;

	qhandle_t		trailShader[2];
	float			trailRadius;
	float			wiTrailTime;

	//
	qhandle_t		missileModelBlue;
	qhandle_t		missileModelRed;
	qhandle_t		spriteShader;
	int				spriteRadius;


	// Hit mark and sounds
	qhandle_t		hitMarkShader;
	int				hitMarkRadius;
	sfxHandle_t		hitSound[3]; // Normal hit sounds, random select
	sfxHandle_t		hitPlayerSound;
	sfxHandle_t		hitMetalSound;

	// Impact mark and sounds
	qhandle_t		impactMarkShader;
	int				impactMarkRadius;
	sfxHandle_t		impactSound[3]; // Impact sounds, random select
	sfxHandle_t		impactPlayerSound;
	sfxHandle_t		impactMetalSound;

	// PE_PROX trigger sound
	sfxHandle_t		triggerSound;

} projectileInfo_t;

typedef struct weaponInfo_s {
	qboolean		registered;

	qhandle_t		weaponModel;
	qhandle_t		barrelModel;
	qhandle_t		flashModel;

	float			flashDlight;
	vec3_t			flashDlightColor;
	sfxHandle_t		flashSound[4];		// fast firing weapons randomly choose

	void			(*ejectBrassFunc)( centity_t * );

	// Impact mark and impact sounds for melee weapons
	qhandle_t		impactMarkShader;
	int				impactMarkRadius;
	sfxHandle_t		impactSound[3]; // Impact sounds, random select
	sfxHandle_t		impactPlayerSound;
	sfxHandle_t		impactMetalSound;

} weaponInfo_t;

// each WP_* weapon enum has an associated weaponGroupInfo_t
// that contains media references necessary to present the
// weapon and its effects
typedef struct weaponGroupInfo_s {
	qboolean		registered;

	qhandle_t		handsModel;			// the hands don't actually draw, they just position the weapon

	vec3_t			weaponMidpoint;		// so it will rotate centered instead of by tag

	qhandle_t		weaponIcon;
#ifdef TURTLEARENA // NOAMMO
	qhandle_t		weaponModel; // Pickup model, only used by new UI.
#else
	qhandle_t		ammoIcon;

	qhandle_t		ammoModel;
#endif

	// loopped sounds
	sfxHandle_t		readySound;
	sfxHandle_t		firingSound;

	// sounds played once
	sfxHandle_t		firingStoppedSound; // gun barrel stopped spining

} weaponGroupInfo_t;
#else
// each WP_* weapon enum has an associated weaponInfo_t
// that contains media references necessary to present the
// weapon and its effects
typedef struct weaponInfo_s {
	qboolean		registered;
	bg_iteminfo_t	*item;

	qhandle_t		handsModel;			// the hands don't actually draw, they just position the weapon
	qhandle_t		weaponModel;
	qhandle_t		barrelModel;
	qhandle_t		flashModel;

	vec3_t			weaponMidpoint;		// so it will rotate centered instead of by tag

	float			flashDlight;
	vec3_t			flashDlightColor;
	sfxHandle_t		flashSound[4];		// fast firing weapons randomly choose

	qhandle_t		weaponIcon;
	qhandle_t		ammoIcon;

	qhandle_t		ammoModel;

	qhandle_t		missileModel;
	sfxHandle_t		missileSound;
	void			(*missileTrailFunc)( centity_t *, const struct weaponInfo_s *wi );
	float			missileDlight;
	vec3_t			missileDlightColor;
	int				missileRenderfx;

	void			(*ejectBrassFunc)( centity_t * );

	float			trailRadius;
	float			wiTrailTime;

	sfxHandle_t		readySound;
	sfxHandle_t		firingSound;
} weaponInfo_t;
#endif

// each IT_* item has an associated itemInfo_t
// that constains media references necessary to present the
// item and its effects
typedef struct {
	qboolean		registered;
	qhandle_t		models[MAX_ITEM_MODELS];
	qhandle_t		icon;
#ifdef IOQ3ZTM // FLAG_MODEL
	qhandle_t		skin;
#endif
} itemInfo_t;

#ifdef TA_NPCSYS
typedef struct {
	qboolean		registered;

	qhandle_t		model;
	qhandle_t		skin;
} npcInfo_t;
#endif

typedef struct {
	int				itemNum;
} powerupInfo_t;


#ifdef MISSIONPACK_HARVESTER
#define MAX_SKULLTRAIL		10

typedef struct {
	vec3_t positions[MAX_SKULLTRAIL];
	int numpositions;
} skulltrail_t;
#endif


typedef enum {
	LEAD_NONE,

	LEAD_LOST,
	LEAD_TIED,
	LEAD_TAKEN,

	LEAD_IGNORE // a client played a reward sound, so no lead change sfx this frame.
} leadChange_t;


#define MAX_REWARDSTACK		10
#define MAX_SOUNDBUFFER		20

//======================================================================

typedef struct
{
  int time;
  int length;
} consoleLine_t;

#define MAX_CONSOLE_LINES 4
#define MAX_CONSOLE_TEXT  ( 256 * MAX_CONSOLE_LINES )

// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

#define MAX_PREDICTED_EVENTS	16
 
typedef struct {

	int			clientNum;

	// prediction state
	qboolean	hyperspace;				// true if prediction has hit a trigger_teleport
	playerState_t	predictedPlayerState;
	centity_t		predictedPlayerEntity;
	qboolean	validPPS;				// clear until the first call to CG_PredictPlayerState
	int			predictedErrorTime;
	vec3_t		predictedError;

	int			eventSequence;
	int			predictableEvents[MAX_PREDICTED_EVENTS];

	float		stepChange;				// for stair up smoothing
	int			stepTime;

	float		duckChange;				// for duck viewheight smoothing
	int			duckTime;

	float		landChange;				// for landing hard
	int			landTime;

	// input state sent to server
#ifndef TA_WEAPSYS_EX
	int			weaponSelect;
#endif
#ifdef TA_HOLDSYS/*2*/
	int			holdableSelect;
#endif


	// centerprinting
	int			centerPrintTime;
	float		centerPrintCharScale;
	int			centerPrintY;
	char		centerPrint[1024];
	int			centerPrintLines;

#ifdef TA_MISC // COMIC_ANNOUNCER
#define MAX_ANNOUNCEMENTS 8
	int			announcement;
	int			announcementTime[MAX_ANNOUNCEMENTS];
	char		announcementMessage[MAX_ANNOUNCEMENTS][128];
#endif

#ifndef TURTLEARENA // NO_AMMO_WARNINGS
	// low ammo warning state
	int			lowAmmoWarning;		// 1 = low, 2 = empty
#endif

	// crosshair client ID
	int			crosshairClientNum;
	int			crosshairClientTime;

	// powerup active flashing
	int			powerupActive;
	int			powerupTime;

	// attacking player
	int			attackerTime;
	int			voiceTime;
	int			currentVoiceClient;

	// orders
	int			currentOrder;
	qboolean	orderPending;
	int			orderTime;
	int			acceptOrderTime;
	int			acceptTask;
	int			acceptLeader;
	char		acceptVoice[MAX_NAME_LENGTH];

	// reward medals
	int			rewardStack;
	int			rewardTime;
	int			rewardCount[MAX_REWARDSTACK];
	qhandle_t	rewardShader[MAX_REWARDSTACK];
#ifdef TA_MISC // COMIC_ANNOUNCER
	int			rewardAnnoucement[MAX_REWARDSTACK];
#else
	qhandle_t	rewardSound[MAX_REWARDSTACK];
#endif

#ifdef TURTLEARENA // LOCKON
	// lockon key
	qboolean	lockedOn;
	int			lockonTime;
#endif

#ifndef TURTLEARENA // NOZOOM
	// zoom key
	qboolean	zoomed;
	int			zoomTime;
	float		zoomSensitivity;
#endif

	int			itemPickup;
	int			itemPickupTime;
	int			itemPickupBlendTime;	// the pulse around the crosshair is timed seperately

#ifdef TURTLEARENA // NIGHTS_ITEMS
	int			scorePickupTime;
#endif

#ifndef TA_WEAPSYS_EX
	int			weaponSelectTime;
#endif
	int			weaponAnimation;
	int			weaponAnimationTime;

	// blend blobs
	float		damageTime;
	float		damageX, damageY, damageValue;

	// status bar head
	float		headYaw;
	float		headEndPitch;
	float		headEndYaw;
	int			headEndTime;
	float		headStartPitch;
	float		headStartYaw;
	int			headStartTime;

	// view movement
	float		v_dmg_time;
	float		v_dmg_pitch;
	float		v_dmg_roll;

	vec3_t		kick_angles;	// weapon kicks
	vec3_t		kick_origin;

#ifdef IOQ3ZTM // NEW_CAM
	float camZoomDir;
	qboolean camZoomIn;
	qboolean camZoomOut;
	float camRotDir;
	qboolean camLeft;
	qboolean camRight;
	qboolean camReseting;
	float camDistance; // Distance from client to put camera
#endif

	qboolean	renderingThirdPerson;		// during deaths, chasecams, etc

	//qboolean cameraMode;		// if rendering from a loaded camera

	vec3_t		lastViewPos;
	vec3_t		lastViewAngles;

#ifdef IOQ3ZTM // LETTERBOX
	// Use CG_ToggleLetterbox to change letterbox mode
	qboolean letterbox;	// qtrue if moving onto the screen, or is done moving on.
						// qfalse if moving off, or is off
	int		letterboxTime; // Time that the letter box move was started, or -1 if instant.
#endif

	// scoreboard
	qboolean	showScores;
	qboolean	scoreBoardShowing;
	int			scoreFadeTime;
	char		killerName[MAX_NAME_LENGTH];

#ifdef TURTLEARENA
	int			waterlevel;
	int			airBarFadeTime; // Air bar start fade time
	qboolean	airBarDrawn; // Drew air bar last frame
#endif

	// console
	char			consoleText[ MAX_CONSOLE_TEXT ];
	consoleLine_t	consoleLines[ MAX_CONSOLE_LINES ];
	int				numConsoleLines;

} cglc_t;
 
#define MAX_SPAWN_VARS          64
#define MAX_SPAWN_VARS_CHARS    2048
 
typedef struct {
	int			clientFrame;		// incremented each frame

	qboolean	demoPlayback;
	qboolean	levelShot;			// taking a level menu screenshot
	int			deferredPlayerLoading;
	qboolean	loading;			// don't defer players at initial startup
	qboolean	intermissionStarted;	// don't play voice rewards, because game will end shortly

	// there are only one or two snapshot_t that are relevent at a time
	int			latestSnapshotNum;	// the number of snapshots the client system has received
	int			latestSnapshotTime;	// the time from latestSnapshotNum, so we don't need to read the snapshot yet

	snapshot_t	*snap;				// cg.snap->serverTime <= cg.time
	snapshot_t	*nextSnap;			// cg.nextSnap->serverTime > cg.time, or NULL
	snapshot_t	activeSnapshots[2];

	float		frameInterpolation;	// (float)( cg.time - cg.frame->serverTime ) / (cg.nextFrame->serverTime - cg.frame->serverTime)

	qboolean	thisFrameTeleport;
	qboolean	nextFrameTeleport;

	int			frametime;		// cg.time - cg.oldTime

	int			time;			// this is the time value that the client
								// is rendering at.
	int			oldTime;		// time at last frame, used for missile trails and prediction checking

	int			physicsTime;	// either cg.snap->time or cg.nextSnap->time

	leadChange_t bestLeadChange;
	int			timelimitWarnings;	// 5 min, 1 min, overtime
	int			fraglimitWarnings;

	qboolean	mapRestart;			// set on a map restart to set back the weapon

	// auto rotating items
	vec3_t		autoAngles;
	vec3_t		autoAxis[3];
	vec3_t		autoAnglesFast;
	vec3_t		autoAxisFast[3];

	// view rendering
	refdef_t	refdef;
	vec3_t		refdefViewAngles;		// will be converted to refdef.viewaxis
	float		fov;					// either range checked cg_fov or forced value

	// spawn variables
	qboolean spawning;                  // the CG_Spawn*() functions are valid
	int numSpawnVars;
	char        *spawnVars[MAX_SPAWN_VARS][2];  // key / value pairs
	int numSpawnVarChars;
	char spawnVarChars[MAX_SPAWN_VARS_CHARS];

	vec2_t mapcoordsMins;
	vec2_t mapcoordsMaxs;
	qboolean mapcoordsValid;

	int			numMiscGameModels;

	int			numViewports;
	int			viewport;
	qboolean	singleCamera; // Rending multiple clients using one viewport
#ifdef TURTLEARENA
	qboolean	allLocalClientsAtIntermission;
#endif

	// information screen text during loading
	char		infoScreenText[MAX_STRING_CHARS];

	// global centerprinting (drawn over all viewports)
	int			centerPrintTime;
	float		centerPrintCharScale;
	int			centerPrintY;
	char		centerPrint[1024];
	int			centerPrintLines;

	// scoreboard
	int			scoresRequestTime;
	int			numScores;
	int			selectedScore;
	int			teamScores[2];
	score_t		scores[MAX_CLIENTS];
#ifdef MISSIONPACK
	char			spectatorList[MAX_STRING_CHARS];		// list of names
	int				spectatorTime;							// last time offset
	float			spectatorOffset;						// current offset from start
#endif

	// sound buffer mainly for announcer sounds
	int			soundBufferIn;
	int			soundBufferOut;
	int			soundTime;
	qhandle_t	soundBuffer[MAX_SOUNDBUFFER];

#ifdef MISSIONPACK
	// for voice chat buffer
	int			voiceChatTime;
	int			voiceChatBufferIn;
	int			voiceChatBufferOut;
#endif

	// warmup countdown
	int			warmup;
	int			warmupCount;

	//==========================

#ifdef MISSIONPACK_HARVESTER
	// skull trails
	skulltrail_t	skulltrails[MAX_CLIENTS];
#endif

	// temp working variables for player view
	float		bobfracsin;
	int			bobcycle;
	float		xyspeed;
#ifndef IOQ3ZTM // NEW_CAM
	int     	nextOrbitTime;
#endif

	// development tool
	refEntity_t		testModelEntity;
	char			testModelName[MAX_QPATH];
	qboolean		testGun;

	// Local client data, from events and such
	cglc_t			*cur_lc;	// Current local client data we are working with
	playerState_t	*cur_ps; // Like cur_lc, but for player state
	int				cur_localClientNum;
	cglc_t			localClients[MAX_SPLITVIEW];

} cg_t;

#ifdef IOQ3ZTM // FLAG_ANIMATIONS
typedef enum
{
	FLAG_RUN,
	FLAG_STAND,
	FLAG_STAND2RUN,
	//FLAG_RUNUP,
	//FLAG_RUNDOWN,

	MAX_FLAG_ANIMATIONS
} flagAnimNumber_t;
#endif

// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t, and powerupInfo_t
typedef struct {
#ifdef IOQ3ZTM // FONT_REWRITE
	font_t		fontGiant;
	font_t		fontBig;
	font_t		fontSmall;
	font_t		fontTiny;
	font_t		fontPropSmall;
	font_t		fontPropBig;
#ifndef TA_DATA
	font_t		fontPropGlowSmall;
	font_t		fontPropGlowBig;
#endif
#else
	qhandle_t	charsetShader;
	qhandle_t	charsetProp;
#ifndef TA_DATA
	qhandle_t	charsetPropGlow;
#endif
	qhandle_t	charsetPropB;
#endif
	qhandle_t	whiteShader;

#ifdef MISSIONPACK
	qhandle_t	redCubeModel;
	qhandle_t	blueCubeModel;
	qhandle_t	redCubeIcon;
	qhandle_t	blueCubeIcon;
#endif
#ifndef TA_DATA // FLAG_MODEL
	qhandle_t	redFlagModel;
	qhandle_t	blueFlagModel;
	qhandle_t	neutralFlagModel;
#endif
	qhandle_t	redFlagShader[3];
	qhandle_t	blueFlagShader[3];
	qhandle_t	flagShader[4];

#ifndef TA_DATA // FLAG_MODEL
	qhandle_t	flagPoleModel;
	qhandle_t	flagFlapModel;

	qhandle_t	redFlagFlapSkin;
	qhandle_t	blueFlagFlapSkin;
	qhandle_t	neutralFlagFlapSkin;
#elif defined TA_WEAPSYS // MELEE_TRAIL
	qhandle_t	flagFlapModel;
#endif

	qhandle_t	redFlagBaseModel;
	qhandle_t	blueFlagBaseModel;
	qhandle_t	neutralFlagBaseModel;

#ifdef IOQ3ZTM // FLAG_ANIMATIONS
	// CTF Flag Animations
	animation_t flag_animations[MAX_FLAG_ANIMATIONS];
#endif

#ifdef MISSIONPACK
	qhandle_t	overloadBaseModel;
	qhandle_t	overloadTargetModel;
	qhandle_t	overloadLightsModel;
	qhandle_t	overloadEnergyModel;

	qhandle_t	harvesterModel;
	qhandle_t	harvesterRedSkin;
	qhandle_t	harvesterBlueSkin;
	qhandle_t	harvesterNeutralModel;
#endif

#ifndef TURTLEARENA // NOARMOR
	qhandle_t	armorModel;
	qhandle_t	armorIcon;
#endif

	qhandle_t	teamStatusBar;

	qhandle_t	deferShader;

#ifndef NOTRATEDM // No gibs.
	// gib explosions
	qhandle_t	gibAbdomen;
	qhandle_t	gibArm;
	qhandle_t	gibChest;
	qhandle_t	gibFist;
	qhandle_t	gibFoot;
	qhandle_t	gibForearm;
	qhandle_t	gibIntestine;
	qhandle_t	gibLeg;
	qhandle_t	gibSkull;
	qhandle_t	gibBrain;

	qhandle_t	smoke2;
#endif

	qhandle_t	machinegunBrassModel;
	qhandle_t	shotgunBrassModel;

#ifndef TA_WEAPSYS
	qhandle_t	railRingsShader;
	qhandle_t	railCoreShader;

	qhandle_t	lightningShader;
#endif

#ifdef IOQ3ZTM // SHOW_TEAM_FRIENDS
	qhandle_t	blueFriendShader;
#endif
	qhandle_t	friendShader;
#ifdef TURTLEARENA // LOCKON
	qhandle_t	targetShader;
#endif

	qhandle_t	balloonShader;
	qhandle_t	connectionShader;

	qhandle_t	selectShader;
#ifndef NOBLOOD
	qhandle_t	viewBloodShader;
#endif
	qhandle_t	tracerShader;
	qhandle_t	crosshairShader[NUM_CROSSHAIRS];
	qhandle_t	lagometerShader;
	qhandle_t	backTileShader;
	qhandle_t	noammoShader;

	qhandle_t	smokePuffShader;
	qhandle_t	smokePuffRageProShader;
	qhandle_t	shotgunSmokePuffShader;
#ifndef TA_WEAPSYS
	qhandle_t	plasmaBallShader;
#endif
	qhandle_t	waterBubbleShader;
#ifndef NOTRATEDM // No gibs.
	qhandle_t	bloodTrailShader;
#endif
#ifdef MISSIONPACK
#ifndef TA_WEAPSYS
	qhandle_t	nailPuffShader;
	qhandle_t	blueProxMine;
#endif
#endif

	qhandle_t	numberShaders[11];

	qhandle_t	shadowMarkShader;

	qhandle_t	botSkillShaders[5];

	// wall mark shaders
	qhandle_t	wakeMarkShader;
#ifndef NOTRATEDM // No gibs.
	qhandle_t	bloodMarkShader;
#endif
	qhandle_t	bulletMarkShader;
	qhandle_t	burnMarkShader;
	qhandle_t	holeMarkShader;
	qhandle_t	energyMarkShader;

	// powerup shaders
#ifndef TURTLEARENA // POWERS
	qhandle_t	quadShader;
	qhandle_t	redQuadShader;
#endif
	qhandle_t	quadWeaponShader;
	qhandle_t	invisShader;
#ifndef TURTLEARENA // POWERS
	qhandle_t	regenShader;
#endif
	qhandle_t	battleSuitShader;
#ifndef TURTLEARENA // POWERS
	qhandle_t	battleWeaponShader;
#endif
	qhandle_t	hastePuffShader;
#ifdef MISSIONPACK
#ifndef TURTLEARENA // NO_KAMIKAZE_ITEM
	qhandle_t	redKamikazeShader;
	qhandle_t	blueKamikazeShader;
#endif
#endif
#ifdef TURTLEARENA // POWERS // PW_FLASHING
	qhandle_t	playerTeleportShader;
#endif

	// weapon effect models
	qhandle_t	bulletFlashModel;
	qhandle_t	ringFlashModel;
	qhandle_t	dishFlashModel;
	qhandle_t	lightningExplosionModel;

	// weapon effect shaders
	qhandle_t	railExplosionShader;
	qhandle_t	plasmaExplosionShader;
	qhandle_t	bulletExplosionShader;
#ifdef TA_WEAPSYS
	qhandle_t	bulletExplosionColorizeShader;
#endif
	qhandle_t	rocketExplosionShader;
	qhandle_t	grenadeExplosionShader;
	qhandle_t	bfgExplosionShader;
#ifndef NOBLOOD
	qhandle_t	bloodExplosionShader;
#endif
#ifdef TURTLEARENA // WEAPONS
	qhandle_t	meleeHitShader[2];
	qhandle_t	missileHitShader[2];
#endif

	// special effects models
	qhandle_t	teleportEffectModel;
#if !defined MISSIONPACK && !defined TURTLEARENA // ZTM: MP removes loading and using...
	qhandle_t	teleportEffectShader;
#endif
#ifdef MISSIONPACK
	qhandle_t	kamikazeEffectModel;
	qhandle_t	kamikazeShockWave;
	qhandle_t	kamikazeHeadModel;
	qhandle_t	kamikazeHeadTrail;
	qhandle_t	guardPowerupModel;
	qhandle_t	scoutPowerupModel;
	qhandle_t	doublerPowerupModel;
	qhandle_t	ammoRegenPowerupModel;
#ifndef TURTLEARENA // POWERS
	qhandle_t	invulnerabilityPowerupModel;
	qhandle_t	invulnerabilityImpactModel;
	qhandle_t	invulnerabilityJuicedModel;
#endif
	qhandle_t	medkitUsageModel;
	qhandle_t	dustPuffShader;
	qhandle_t	heartShader;
#endif

#ifdef TA_DATA // EXP_SCALE
	qhandle_t	smokeModel;
#endif

	// scoreboard headers
	qhandle_t	scoreboardName;
	qhandle_t	scoreboardPing;
	qhandle_t	scoreboardScore;
	qhandle_t	scoreboardTime;

	// medals shown during gameplay
#ifndef TURTLEARENA // AWARDS
	qhandle_t	medalImpressive;
	qhandle_t	medalExcellent;
	qhandle_t	medalGauntlet;
#endif
	qhandle_t	medalDefend;
	qhandle_t	medalAssist;
	qhandle_t	medalCapture;

#ifdef TA_MISC // MATERIALS
	qhandle_t	matModels[NUM_MATERIAL_TYPES][NUM_MATERIAL_MODELS];
	int			matNumModels[NUM_MATERIAL_TYPES];
#endif

	// sounds
	sfxHandle_t	quadSound;
	sfxHandle_t	tracerSound;
	sfxHandle_t	selectSound;
	sfxHandle_t	useNothingSound;
	sfxHandle_t	wearOffSound;
	sfxHandle_t	footsteps[FOOTSTEP_TOTAL][4];
#ifdef TA_MISC // MATERIALS
	sfxHandle_t matExplode[NUM_MATERIAL_TYPES];
#endif
#ifndef TA_WEAPSYS
	sfxHandle_t	sfx_lghit1;
	sfxHandle_t	sfx_lghit2;
	sfxHandle_t	sfx_lghit3;
#endif
	sfxHandle_t	sfx_ric1;
	sfxHandle_t	sfx_ric2;
	sfxHandle_t	sfx_ric3;
	//sfxHandle_t	sfx_railg;
	sfxHandle_t	sfx_rockexp;
	sfxHandle_t	sfx_plasmaexp;
#ifdef MISSIONPACK
#ifndef TURTLEARENA // WEAPONS NO_KAMIKAZE_ITEM POWERS
	sfxHandle_t	sfx_proxexp;
	sfxHandle_t	sfx_nghit;
	sfxHandle_t	sfx_nghitflesh;
	sfxHandle_t	sfx_nghitmetal;
	sfxHandle_t	sfx_chghit;
	sfxHandle_t	sfx_chghitflesh;
	sfxHandle_t	sfx_chghitmetal;
	sfxHandle_t kamikazeExplodeSound;
	sfxHandle_t kamikazeImplodeSound;
	sfxHandle_t kamikazeFarSound;
	sfxHandle_t useInvulnerabilitySound;
	sfxHandle_t invulnerabilityImpactSound1;
	sfxHandle_t invulnerabilityImpactSound2;
	sfxHandle_t invulnerabilityImpactSound3;
	sfxHandle_t invulnerabilityJuicedSound;
#endif
	sfxHandle_t obeliskHitSound1;
	sfxHandle_t obeliskHitSound2;
	sfxHandle_t obeliskHitSound3;
	sfxHandle_t	obeliskRespawnSound;
#ifndef TA_SP
	sfxHandle_t	winnerSound;
	sfxHandle_t	loserSound;
#endif
#endif
#ifndef NOTRATEDM // No gibs.
	sfxHandle_t	gibSound;
	sfxHandle_t	gibBounce1Sound;
	sfxHandle_t	gibBounce2Sound;
	sfxHandle_t	gibBounce3Sound;
#endif
	sfxHandle_t	teleInSound;
	sfxHandle_t	teleOutSound;
	sfxHandle_t	noAmmoSound;
	sfxHandle_t	respawnSound;
	sfxHandle_t talkSound;
#ifndef IOQ3ZTM // MORE_PLAYER_SOUNDS
	sfxHandle_t landSound;
	sfxHandle_t fallSound;
#endif
	sfxHandle_t jumpPadSound;

#ifdef IOQ3ZTM // LETTERBOX
	sfxHandle_t letterBoxOnSound;
	sfxHandle_t letterBoxOffSound;
#endif

	sfxHandle_t oneMinuteSound;
	sfxHandle_t fiveMinuteSound;
	sfxHandle_t suddenDeathSound;

	sfxHandle_t threeFragSound;
	sfxHandle_t twoFragSound;
	sfxHandle_t oneFragSound;

	sfxHandle_t hitSound;
#ifndef TURTLEARENA // NOARMOR
	sfxHandle_t hitSoundHighArmor;
	sfxHandle_t hitSoundLowArmor;
#endif
	sfxHandle_t hitTeamSound;
#ifndef TURTLEARENA // AWARDS
	sfxHandle_t impressiveSound;
	sfxHandle_t excellentSound;
	sfxHandle_t deniedSound;
	sfxHandle_t humiliationSound;
#endif
	sfxHandle_t assistSound;
	sfxHandle_t defendSound;
#ifndef TURTLEARENA // AWARDS
	sfxHandle_t firstImpressiveSound;
	sfxHandle_t firstExcellentSound;
	sfxHandle_t firstHumiliationSound;
#endif

	sfxHandle_t takenLeadSound;
	sfxHandle_t tiedLeadSound;
	sfxHandle_t lostLeadSound;

	sfxHandle_t voteNow;
	sfxHandle_t votePassed;
	sfxHandle_t voteFailed;

	sfxHandle_t watrInSound;
	sfxHandle_t watrOutSound;
	sfxHandle_t watrUnSound;

	sfxHandle_t flightSound;
	sfxHandle_t medkitSound;
#ifdef TURTLEARENA // HOLDABLE
	sfxHandle_t shurikenSound;
#endif

#ifdef MISSIONPACK
	sfxHandle_t weaponHoverSound;
#endif

	// teamplay sounds
	sfxHandle_t captureAwardSound;
	sfxHandle_t redScoredSound;
	sfxHandle_t blueScoredSound;
	sfxHandle_t redLeadsSound;
	sfxHandle_t blueLeadsSound;
	sfxHandle_t teamsTiedSound;

#ifdef TA_DATA
	sfxHandle_t	captureFlagSound;
	sfxHandle_t	returnFlagSound;
#else
	sfxHandle_t	captureYourTeamSound;
	sfxHandle_t	captureOpponentSound;
	sfxHandle_t	returnYourTeamSound;
	sfxHandle_t	returnOpponentSound;
	sfxHandle_t	takenYourTeamSound;
	sfxHandle_t	takenOpponentSound;
#endif

	sfxHandle_t redFlagReturnedSound;
	sfxHandle_t blueFlagReturnedSound;
#ifdef MISSIONPACK
	sfxHandle_t neutralFlagReturnedSound;
#endif
#ifdef TA_DATA
	sfxHandle_t redTeamTookBlueFlagSound;
	sfxHandle_t blueTeamTookRedFlagSound;
	sfxHandle_t	youHaveFlagSound;
	sfxHandle_t	playerHasFlagSound[MAX_SPLITVIEW];
#ifdef MISSIONPACK
	sfxHandle_t	redTeamTookTheFlagSound;
	sfxHandle_t	blueTeamTookTheFlagSound;
	sfxHandle_t redBaseIsUnderAttackSound;
	sfxHandle_t blueBaseIsUnderAttackSound;
#endif
#else
	sfxHandle_t	enemyTookYourFlagSound;
	sfxHandle_t yourTeamTookEnemyFlagSound;
	sfxHandle_t	youHaveFlagSound;
#ifdef MISSIONPACK
	sfxHandle_t	enemyTookTheFlagSound;
	sfxHandle_t yourTeamTookTheFlagSound;
	sfxHandle_t yourBaseIsUnderAttackSound;
#endif
#endif
#ifndef NOTRATEDM // Disable strong lang.
	sfxHandle_t holyShitSound;
#endif

	// tournament sounds
	sfxHandle_t	count3Sound;
	sfxHandle_t	count2Sound;
	sfxHandle_t	count1Sound;
	sfxHandle_t	countFightSound;
	sfxHandle_t	countPrepareSound;

#ifdef MISSIONPACK
	// new stuff
	qhandle_t patrolShader;
	qhandle_t assaultShader;
	qhandle_t campShader;
	qhandle_t followShader;
	qhandle_t defendShader;
	qhandle_t teamLeaderShader;
	qhandle_t retrieveShader;
	qhandle_t escortShader;
	qhandle_t flagShaders[3];
	sfxHandle_t	countPrepareTeamSound;

#ifndef TURTLEARENA // POWERS
	sfxHandle_t ammoregenSound;
	sfxHandle_t doublerSound;
	sfxHandle_t guardSound;
	sfxHandle_t scoutSound;
#endif

	qhandle_t cursor;
	qhandle_t selectCursor;
	qhandle_t sizeCursor;
#endif

	sfxHandle_t	regenSound;
	sfxHandle_t	protectSound;
#ifndef TURTLEARENA // POWERS
	sfxHandle_t	n_healthSound;
#endif
#ifndef TA_WEAPSYS
	sfxHandle_t	hgrenb1aSound;
	sfxHandle_t	hgrenb2aSound;
	sfxHandle_t	wstbimplSound;
	sfxHandle_t	wstbimpmSound;
	sfxHandle_t	wstbimpdSound;
	sfxHandle_t	wstbactvSound;
#endif
#ifdef TA_WEAPSYS // MELEE_TRAIL
	qhandle_t	weaponTrailShader;
#endif

#ifdef TURTLEARENA
	qhandle_t	hudHeadBackgroundShader;
	qhandle_t	hudBarShader;
	qhandle_t	hudBar2Shader;
	qhandle_t	hudBarBackgroundShader;
#endif

} cgMedia_t;

#define MAX_STATIC_GAMEMODELS   1024

typedef struct cg_gamemodel_s {
	qhandle_t model;
	vec3_t org;
	vec3_t axes[3];
	vec_t radius;
} cg_gamemodel_t;

// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
typedef struct {
	gameState_t		gameState;			// gamestate from server
	glconfig_t		glconfig;			// rendering configuration
	float			screenXScale;		// derived from glconfig
	float			screenYScale;
	float			screenXBias;
	float			screenYBias;
	float			screenXScaleStretch;
	float			screenYScaleStretch;

	int				maxSplitView;

	int				serverCommandSequence;	// reliable command stream counter
	int				processedSnapshotNum;// the number of snapshots cgame has requested

	qboolean		localServer;		// detected on startup by checking sv_running

	// parsed from serverinfo
	gametype_t		gametype;
	int				dmflags;
	int				teamflags;
	int				fraglimit;
	int				capturelimit;
	int				timelimit;
	int				maxclients;
	char			mapname[MAX_QPATH];

	int				voteTime;
	int				voteYes;
	int				voteNo;
	qboolean		voteModified;			// beep whenever changed
	char			voteString[MAX_STRING_TOKENS];

	int				teamVoteTime[2];
	int				teamVoteYes[2];
	int				teamVoteNo[2];
	qboolean		teamVoteModified[2];	// beep whenever changed
	char			teamVoteString[2][MAX_STRING_TOKENS];

	int				levelStartTime;

	int				scores1, scores2;		// from configstrings
	int				redflag, blueflag;		// flag status from configstrings
	int				flagStatus;

	qboolean  newHud;

	//
	// locally derived information from gamestate
	//
	qhandle_t		gameModels[MAX_MODELS];
	sfxHandle_t		gameSounds[MAX_SOUNDS];

	int				numInlineModels;
	qhandle_t		inlineDrawModel[MAX_MODELS];
	vec3_t			inlineModelMidpoints[MAX_MODELS];

	clientInfo_t	clientinfo[MAX_CLIENTS];

	// teamchat width is *3 because of embedded color codes
	char			teamChatMsgs[TEAMCHAT_HEIGHT][TEAMCHAT_WIDTH*3+1];
	int				teamChatMsgTimes[TEAMCHAT_HEIGHT];
	int				teamChatPos;
	int				teamLastChatPos;

	int cursorX;
	int cursorY;
	qboolean eventHandling;
	qboolean mouseCaptured;
	qboolean sizingHud;
	void *capturedItem;
	qhandle_t activeCursor;

	// default global fog from bsp or fogvars in a shader
	fogType_t	globalFogType;
	vec3_t		globalFogColor;
	float		globalFogDepthForOpaque;
	float		globalFogDensity;

	cg_gamemodel_t miscGameModels[MAX_STATIC_GAMEMODELS];

	// media
	cgMedia_t		media;

} cgs_t;

//==============================================================================

extern	cgs_t			cgs;
extern	cg_t			cg;
extern	centity_t		cg_entities[MAX_GENTITIES];
#ifdef TA_WEAPSYS
extern	projectileInfo_t	cg_projectiles[MAX_BG_PROJ];
extern	weaponInfo_t		cg_weapons[MAX_BG_WEAPONS];
extern	weaponGroupInfo_t	cg_weapongroups[MAX_BG_WEAPON_GROUPS];
#else
extern	weaponInfo_t	cg_weapons[MAX_WEAPONS];
#endif
extern	itemInfo_t		cg_items[MAX_ITEMS];
#ifdef TA_NPCSYS
extern	npcInfo_t		cg_npcs[MAX_NPCS];
#endif
extern	markPoly_t		cg_markPolys[MAX_MARK_POLYS];

extern	vmCvar_t		cg_centertime;
extern	vmCvar_t		cg_runpitch;
extern	vmCvar_t		cg_runroll;
extern	vmCvar_t		cg_bobup;
extern	vmCvar_t		cg_bobpitch;
extern	vmCvar_t		cg_bobroll;
extern	vmCvar_t		cg_swingSpeed;
extern	vmCvar_t		cg_shadows;
#ifndef NOTRATEDM // No gibs.
extern	vmCvar_t		cg_gibs;
#endif
#ifdef IOQ3ZTM // DRAW_SPEED
extern	vmCvar_t		cg_drawSpeed;
#endif
extern	vmCvar_t		cg_drawTimer;
#ifdef TA_MISC // COMIC_ANNOUNCER
extern	vmCvar_t		cg_announcerText;
extern	vmCvar_t		cg_announcerVoice;
#endif
extern	vmCvar_t		cg_drawFPS;
extern	vmCvar_t		cg_drawSnapshot;
extern	vmCvar_t		cg_draw3dIcons;
extern	vmCvar_t		cg_drawIcons;
#ifndef TURTLEARENA // NO_AMMO_WARNINGS
extern	vmCvar_t		cg_drawAmmoWarning;
#endif
extern	vmCvar_t		cg_drawCrosshair;
extern	vmCvar_t		cg_drawCrosshairNames;
extern	vmCvar_t		cg_drawRewards;
extern	vmCvar_t		cg_drawTeamOverlay;
extern	vmCvar_t		cg_teamOverlayUserinfo;
extern	vmCvar_t		cg_crosshairX;
extern	vmCvar_t		cg_crosshairY;
extern	vmCvar_t		cg_crosshairSize;
extern	vmCvar_t		cg_crosshairHealth;
extern	vmCvar_t		cg_drawStatus;
extern	vmCvar_t		cg_draw2D;
#ifndef IOQ3ZTM // LERP_FRAME_CLIENT_LESS
extern	vmCvar_t		cg_animSpeed;
extern	vmCvar_t		cg_debugAnim;
#endif
extern	vmCvar_t		cg_debugPosition;
extern	vmCvar_t		cg_debugEvents;
extern	vmCvar_t		cg_railTrailTime;
extern	vmCvar_t		cg_errorDecay;
extern	vmCvar_t		cg_nopredict;
extern	vmCvar_t		cg_noPlayerAnims;
extern	vmCvar_t		cg_showmiss;
extern	vmCvar_t		cg_footsteps;
extern	vmCvar_t		cg_addMarks;
extern	vmCvar_t		cg_brassTime;
extern	vmCvar_t		cg_gun_frame;
extern	vmCvar_t		cg_gun_x;
extern	vmCvar_t		cg_gun_y;
extern	vmCvar_t		cg_gun_z;
extern	vmCvar_t		cg_drawGun[MAX_SPLITVIEW];
extern	vmCvar_t		cg_viewsize;
extern	vmCvar_t		cg_tracerChance;
extern	vmCvar_t		cg_tracerWidth;
extern	vmCvar_t		cg_tracerLength;
#ifndef TA_WEAPSYS_EX
extern	vmCvar_t		cg_autoswitch[MAX_SPLITVIEW];
#endif
extern	vmCvar_t		cg_ignore;
extern	vmCvar_t		cg_simpleItems;
extern	vmCvar_t		cg_fov;
#ifndef TURTLEARENA // NOZOOM
extern	vmCvar_t		cg_zoomFov;
#endif
extern	vmCvar_t		cg_thirdPersonRange[MAX_SPLITVIEW];
extern	vmCvar_t		cg_thirdPersonAngle[MAX_SPLITVIEW];
extern	vmCvar_t		cg_thirdPerson[MAX_SPLITVIEW];
extern	vmCvar_t		cg_splitviewVertical;
extern	vmCvar_t		cg_lagometer;
extern	vmCvar_t		cg_drawAttacker;
extern	vmCvar_t		cg_synchronousClients;
extern	vmCvar_t		cg_teamChatTime;
extern	vmCvar_t		cg_teamChatHeight;
extern	vmCvar_t		cg_stats;
#ifndef TURTLEARENA // NO_CGFORCEMODLE
extern	vmCvar_t 		cg_forceModel;
#endif
extern	vmCvar_t 		cg_buildScript;
extern	vmCvar_t		cg_paused;
#ifndef NOBLOOD
extern	vmCvar_t		cg_blood;
#endif
extern	vmCvar_t		cg_predictItems;
extern	vmCvar_t		cg_deferPlayers;
extern	vmCvar_t		cg_drawFriend;
extern	vmCvar_t		cg_teamChatsOnly;
#ifdef MISSIONPACK
extern	vmCvar_t		cg_noVoiceChats;
extern	vmCvar_t		cg_noVoiceText;
#endif
extern  vmCvar_t		cg_scorePlum;
extern	vmCvar_t		cg_smoothClients;
extern	vmCvar_t		pmove_fixed;
extern	vmCvar_t		pmove_msec;
//extern	vmCvar_t		cg_pmove_fixed;
extern	vmCvar_t		cg_cameraOrbit;
#ifndef IOQ3ZTM // NEW_CAM
extern	vmCvar_t		cg_cameraOrbitDelay;
#endif
extern	vmCvar_t		cg_timescaleFadeEnd;
extern	vmCvar_t		cg_timescaleFadeSpeed;
extern	vmCvar_t		cg_timescale;
extern	vmCvar_t		cg_cameraMode;
extern  vmCvar_t		cg_smallFont;
extern  vmCvar_t		cg_bigFont;
extern	vmCvar_t		cg_noTaunt;
extern	vmCvar_t		cg_noProjectileTrail;
extern	vmCvar_t		cg_oldRail;
extern	vmCvar_t		cg_oldRocket;
extern	vmCvar_t		cg_oldPlasma;
extern	vmCvar_t		cg_trueLightning;
extern	vmCvar_t		cg_atmosphericEffects;
extern	vmCvar_t		cg_teamDmLeadAnnouncements;
extern	vmCvar_t		cg_voipShowMeter;
extern	vmCvar_t		cg_voipShowCrosshairMeter;
extern	vmCvar_t		cg_consoleLatency;
#if !defined MISSIONPACK && defined IOQ3ZTM // Support MissionPack players.
extern	vmCvar_t		cg_redTeamName;
extern	vmCvar_t		cg_blueTeamName;
#endif
#ifdef MISSIONPACK
extern	vmCvar_t		cg_redTeamName;
extern	vmCvar_t		cg_blueTeamName;
extern	vmCvar_t		cg_currentSelectedPlayer[MAX_SPLITVIEW];
extern	vmCvar_t		cg_currentSelectedPlayerName[MAX_SPLITVIEW];
#ifndef IOQ3ZTM
extern	vmCvar_t		cg_singlePlayer;
#endif
extern	vmCvar_t		cg_enableDust;
extern	vmCvar_t		cg_enableBreath;
extern	vmCvar_t		cg_singlePlayerActive;
extern  vmCvar_t		cg_recordSPDemo;
extern  vmCvar_t		cg_recordSPDemoName;
extern	vmCvar_t		cg_obeliskRespawnDelay;
#endif
#ifdef TA_WEAPSYS // MELEE_TRAIL
extern	vmCvar_t		cg_drawMeleeWeaponTrails;
#endif
#ifdef TA_MISC // MATERIALS 
extern	vmCvar_t		cg_impactDebris;
#endif
#ifdef IOQ3ZTM // LASERTAG
extern	vmCvar_t		cg_laserTag;
#endif
#ifdef TA_PATHSYS // 2DMODE
extern vmCvar_t			cg_2dmode;
extern vmCvar_t			cg_2dmodeOverride;
#endif

//
// cg_main.c
//
const char *CG_ConfigString( int index );
const char *CG_Argv( int arg );

int CG_MaxSplitView(void);

void QDECL CG_DPrintf( const char *msg, ... ) __attribute__ ((format (printf, 1, 2)));
void QDECL CG_Printf( const char *msg, ... ) __attribute__ ((format (printf, 1, 2)));
void QDECL CG_Error( const char *msg, ... ) __attribute__ ((noreturn, format (printf, 1, 2)));

void QDECL CG_NotifyPrintf( int localClientNum, const char *msg, ... ) __attribute__ ((format (printf, 2, 3)));
void QDECL CG_NotifyBitsPrintf( int localClientNum, const char *msg, ... ) __attribute__ ((format (printf, 2, 3)));

void CG_LocalClientAdded(int localClientNum, int clientNum);
void CG_LocalClientRemoved(int localClientNum);

void CG_StartMusic( void );

void CG_UpdateCvars( void );

int CG_CrosshairPlayer( int localClientNum );
int CG_LastAttacker( int localClientNum );
void CG_LoadMenus(const char *menuFile);
void CG_KeyEvent(int key, qboolean down);
void CG_MouseEvent(int localClientNum, int x, int y);
void CG_EventHandling(int type);
void CG_RankRunFrame( void );
void CG_SetScoreSelection(void *menu);
score_t *CG_GetSelectedScore( void );
void CG_BuildSpectatorString( void );

void CG_RemoveNotifyLine( cglc_t *localClient );
void CG_AddNotifyText( void );


//
// cg_view.c
//
void CG_TestModel_f (void);
void CG_TestGun_f (void);
void CG_TestModelNextFrame_f (void);
void CG_TestModelPrevFrame_f (void);
void CG_TestModelNextSkin_f (void);
void CG_TestModelPrevSkin_f (void);
#ifndef TURTLEARENA // NOZOOM
void CG_ZoomUp_f( int localClient );
void CG_ZoomDown_f( int localClient );
#endif
void CG_AddBufferedSound( sfxHandle_t sfx);

#ifdef TA_MISC // COMIC_ANNOUNCER
typedef enum
{
	ANNOUNCE_PREPAREYOURSELFS,
	ANNOUNCE_PREPAREYOURTEAM,

	ANNOUNCE_VOTINGBEGUN,
	ANNOUNCE_VOTEPASS,
	ANNOUNCE_VOTEFAIL,

	ANNOUNCE_YOUHAVETAKENTHELEAD,
	ANNOUNCE_YOURTIEDFORTHELEAD,
	ANNOUNCE_YOULOSTTHELEAD,

	ANNOUNCE_CAPTURE,
	ANNOUNCE_ASSIST,
	ANNOUNCE_DEFENSE,

	// TEAM/CTF/1FCTF/Overload
	ANNOUNCE_REDLEADS,
	ANNOUNCE_BLUELEADS,
	ANNOUNCE_TEAMSTIED,

	// CTF and one flag CTF
	ANNOUNCE_YOUHAVETHEFLAG,
	ANNOUNCE_REDSCORES,
	ANNOUNCE_BLUESCORES,
	ANNOUNCE_TEAMCAPTURE,
	ANNOUNCE_ENEMYCAPTURE,
	ANNOUNCE_YOURFLAGRETURNED,
	ANNOUNCE_ENEMYFLAGRETURNED,
	ANNOUNCE_YOURTEAMHASTAKENTHEFLAG,
	ANNOUNCE_THEENEMYHASTAKENTHEFLAG,

	// CTF
	ANNOUNCE_REDFLAGRETURNED,
	ANNOUNCE_BLUEFLAGRETURNED,
	ANNOUNCE_ENEMYHASYOURFLAG,
	ANNOUNCE_TEAMHASENEMYFLAG,

	// One flag CTF
	ANNOUNCE_TEAMHASTHEFLAG,
	ANNOUNCE_ENEMYHASTHEFLAG,

	// Overload
	ANNOUNCE_BASEUNDERATTACK,

	ANNOUNCE_MAX
} announcement_e;

void CG_AddAnnouncement(int announcement, int localClientNumber);
void CG_AddAnnouncementEx(cglc_t *lc, qhandle_t sfx, qboolean bufferedSfx, const char *message);
#endif

void CG_SetupFrustum( void );
qboolean CG_CullPoint( vec3_t pt );
qboolean CG_CullPointAndRadius( const vec3_t pt, vec_t radius );

void CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );


//
// cg_drawtools.c
//
typedef enum {
	PLACE_STRETCH,
	PLACE_CENTER,

	// horizontal only
	PLACE_LEFT,
	PLACE_RIGHT,

	// vertical only
	PLACE_TOP,
	PLACE_BOTTOM
} screenPlacement_e;

void CG_SetScreenPlacement(screenPlacement_e hpos, screenPlacement_e vpos);
void CG_PopScreenPlacement(void);
screenPlacement_e CG_GetScreenHorizontalPlacement(void);
screenPlacement_e CG_GetScreenVerticalPlacement(void);
void CG_AdjustFrom640( float *x, float *y, float *w, float *h );
void CG_FillRect( float x, float y, float width, float height, const float *color );
void CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader );
void CG_SetClipRegion( float x, float y, float w, float h );
void CG_ClearClipRegion( void );

#define CENTER_X -640
#ifdef IOQ3ZTM // FONT_REWRITE
qboolean CG_LoadFont(font_t *font, const char *ttfName, const char *shaderName, int pointSize,
			int shaderCharWidth, float fontKerning);
void CG_DrawFontStringExt( font_t *font, float scale, float x, float y, const char *string, const float *setColor, qboolean forceColor,
		qboolean noColorEscape, int drawShadow, qboolean adjustFrom640, float adjust, int limit, float *maxX );

void CG_Text_Paint_Limit(float *maxX, float x, float y, float scale, vec4_t color, const char* text, float adjust, int limit);

void CG_DrawFontString( font_t *font, int x, int y, const char *s, float alpha );
void CG_DrawFontStringColor( font_t *font, int x, int y, const char *s, vec4_t color );
#endif
void CG_DrawString( float x, float y, const char *string, 
				   float charWidth, float charHeight, const float *modulate );
void CG_DrawStringExt( int x, int y, const char *string, const float *setColor, 
		qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars );

void CG_DrawGiantString( int x, int y, const char *s, float alpha );
void CG_DrawGiantStringColor( int x, int y, const char *s, vec4_t color );
void CG_DrawBigString( int x, int y, const char *s, float alpha );
void CG_DrawBigStringColor( int x, int y, const char *s, vec4_t color );
void CG_DrawSmallString( int x, int y, const char *s, float alpha );
void CG_DrawSmallStringColor( int x, int y, const char *s, vec4_t color );
void CG_DrawTinyString( int x, int y, const char *s, float alpha );
void CG_DrawTinyStringColor( int x, int y, const char *s, vec4_t color );

int CG_DrawStrlen( const char *str );

float	*CG_FadeColor( int startMsec, int totalMsec );
#ifdef TURTLEARENA // NIGHTS_ITEMS
void	CG_ColorForChain(int val, vec3_t color);
#endif
float *CG_TeamColor( int team );
void CG_TileClear( void );
void CG_KeysStringForBinding(const char *binding, char *string, int stringSize );
void CG_ColorForHealth( vec4_t hcolor );
#ifdef TURTLEARENA // NOARMOR
void CG_GetColorForHealth( int health, vec4_t hcolor );
#else
void CG_GetColorForHealth( int health, int armor, vec4_t hcolor );
#endif

void UI_DrawProportionalString( int x, int y, const char* str, int style, vec4_t color );
void CG_DrawRect( float x, float y, float width, float height, float size, const float *color );
void CG_DrawSides(float x, float y, float w, float h, float size);
void CG_DrawTopBottom(float x, float y, float w, float h, float size);


//
// cg_draw.c, cg_newDraw.c
//
typedef enum {
  SYSTEM_PRINT,
  CHAT_PRINT,
  TEAMCHAT_PRINT
} q3print_t;

extern	int sortedTeamPlayers[TEAM_NUM_TEAMS][TEAM_MAXOVERLAY];
extern	int	numSortedTeamPlayers[TEAM_NUM_TEAMS];
extern	int	sortedTeamPlayersTime[TEAM_NUM_TEAMS];
extern	int drawTeamOverlayModificationCount;
extern  char systemChat[256];
extern  char teamChat1[256];
extern  char teamChat2[256];

void CG_AddLagometerFrameInfo( void );
void CG_AddLagometerSnapshotInfo( snapshot_t *snap );
void CG_CenterPrint( int localClientNum, const char *str, int y, float charScale );
void CG_GlobalCenterPrint( const char *str, int y, float charScale );
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles );
void CG_DrawActive( stereoFrame_t stereoView );
void CG_DrawScreen2D( stereoFrame_t stereoView );
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D );
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team, int clientNum );
void CG_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle);
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style);
int CG_Text_Width(const char *text, float scale, int limit);
int CG_Text_Height(const char *text, float scale, int limit);
void CG_SelectPrevPlayer( int localPlayerNum );
void CG_SelectNextPlayer( int localPlayerNum );
float CG_GetValue(int ownerDraw);
qboolean CG_OwnerDrawVisible(int flags);
void CG_RunMenuScript(char **args);
void CG_ShowResponseHead( void );
void CG_SetPrintString(q3print_t type, const char *p);
void CG_InitTeamChat( void );
void CG_GetTeamColor(vec4_t *color);
const char *CG_GetGameStatusText( void );
const char *CG_GetKillerText( void );
void CG_Draw3DModel(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles);
#if defined IOQ3ZTM || defined IOQ3ZTM_NO_COMPAT // DAMAGE_SKINS
void CG_Draw3DHeadModel( int clientNum, float x, float y, float w, float h, vec3_t origin, vec3_t angles );
#endif
#ifndef IOQ3ZTM // FONT_REWRITE
void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader);
#endif
void CG_CheckOrderPending( int localPlayerNum );
const char *CG_GameTypeString( void );
qboolean CG_YourTeamHasFlag( void );
qboolean CG_OtherTeamHasFlag( void );
qhandle_t CG_StatusHandle(int task);
qboolean CG_AnyScoreboardShowing( void );



//
// cg_player.c
//
void CG_Player( centity_t *cent );
void CG_ResetPlayerEntity( centity_t *cent );
void CG_AddRefEntityWithPowerups( refEntity_t *ent, entityState_t *state, int team );
void CG_NewClientInfo( int clientNum );
sfxHandle_t	CG_CustomSound( int clientNum, const char *soundName );

//
// cg_predict.c
//
void CG_BuildSolidList( void );
int	CG_PointContents( const vec3_t point, int passEntityNum );
void CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, 
					 int skipNumber, int mask );
void CG_PredictPlayerState( void );
void CG_LoadDeferredPlayers( void );


//
// cg_events.c
//
void CG_CheckEvents( centity_t *cent );
const char	*CG_PlaceString( int rank );
void CG_EntityEvent( centity_t *cent, vec3_t position );
void CG_PainEvent( centity_t *cent, int health );


//
// cg_ents.c
//
void CG_SetEntitySoundPosition( centity_t *cent );
void CG_AddPacketEntities( void );
void CG_Beam( centity_t *cent );
void CG_AdjustPositionForMover(const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, vec3_t angles_in, vec3_t angles_out);

#ifdef IOQ3ZTM // BONES
qboolean CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							qhandle_t parentModel, const refSkeleton_t *parentSkeleton, char *tagName );
qboolean CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							qhandle_t parentModel, const refSkeleton_t *parentSkeleton, char *tagName );
#else
qboolean CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							qhandle_t parentModel, char *tagName );
qboolean CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							qhandle_t parentModel, char *tagName );
#endif



//
// cg_weapons.c
//
#ifdef TA_HOLDSYS/*2*/
void CG_NextHoldable_f( int localClient );
void CG_PrevHoldable_f( int localClient );
void CG_Holdable_f( int localClient );
#endif

#ifndef TA_WEAPSYS_EX
void CG_NextWeapon_f( int localClient );
void CG_PrevWeapon_f( int localClient );
void CG_Weapon_f( int localClient );
#endif

#ifdef TURTLEARENA // HOLD_SHURIKEN
void CG_RegisterHoldable( int holdableNum );
#endif
#ifdef TA_WEAPSYS
void CG_RegisterProjectile( int projectileNum );
#endif
void CG_RegisterWeapon( int weaponNum );
#ifdef TA_WEAPSYS
void CG_RegisterWeaponGroup( int weaponNum );
#endif
void CG_RegisterItemVisuals( int itemNum );

void CG_FireWeapon( centity_t *cent );
#ifdef TA_MISC // MATERIALS
void CG_ImpactParticles( vec3_t origin, vec3_t dir, float radius, int surfaceFlags, int skipNum );
#endif
void CG_MissileExplode( int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType );
void CG_MissileHitPlayer( int weapon, vec3_t origin, vec3_t dir, int entityNum );
#ifdef TA_WEAPSYS
void CG_MissileImpact( int projnum, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType );
void CG_WeaponImpact( int weaponGroup, int hand, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType );
void CG_WeaponHitPlayer( int weaponGroup, int hand, vec3_t origin, vec3_t dir, int entityNum );
#else
void CG_ShotgunFire( entityState_t *es );
#endif
#ifdef TA_WEAPSYS
void CG_Bullet( vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum, int projnum);
#else
void CG_Bullet( vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum);
#endif

#ifdef TA_WEAPSYS
void CG_RailTrail( clientInfo_t *ci, const projectileInfo_t *wi, vec3_t start, vec3_t end );
void CG_GrappleTrail( centity_t *ent, const projectileInfo_t *wi );
#else
void CG_RailTrail( clientInfo_t *ci, vec3_t start, vec3_t end );
void CG_GrappleTrail( centity_t *ent, const weaponInfo_t *wi );
#endif
void CG_AddViewWeapon (playerState_t *ps);
void CG_AddPlayerWeapon( refEntity_t *parent, refSkeleton_t *parentSkeleton, playerState_t *ps, centity_t *cent, int team );
#ifndef TA_WEAPSYS_EX
void CG_DrawWeaponSelect( void );

void CG_OutOfAmmoChange( int localClientNum );	// should this be in pmove?
#endif
#ifdef IOQ3ZTM // GHOST
void CG_GhostRefEntity(refEntity_t *refEnt, ghostRefData_t *refs, int num, int *ghostTime);
#endif

//
// cg_marks.c
//
void	CG_InitMarkPolys( void );
void	CG_AddMarks( void );
#ifdef TA_WEAPSYS
qboolean	CG_ImpactMark( qhandle_t markShader,
				    const vec3_t origin, const vec3_t dir, 
					float orientation, 
				    float r, float g, float b, float a, 
					qboolean alphaFade, 
					float radius, qboolean temporary );
#else
void	CG_ImpactMark( qhandle_t markShader,
				    const vec3_t origin, const vec3_t dir,
					float orientation,
				    float r, float g, float b, float a,
					qboolean alphaFade,
					float radius, qboolean temporary );
#endif

//
// cg_localents.c
//
void	CG_InitLocalEntities( void );
localEntity_t	*CG_AllocLocalEntity( void );
void	CG_AddLocalEntities( void );

//
// cg_effects.c
//
localEntity_t *CG_SmokePuff( const vec3_t p, 
				   const vec3_t vel, 
				   float radius,
				   float r, float g, float b, float a,
				   float duration,
				   int startTime,
				   int fadeInTime,
				   int leFlags,
				   qhandle_t hShader );
void CG_BubbleTrail( vec3_t start, vec3_t end, float spacing );
#ifdef TA_WEAPSYS
qboolean CG_BulletBubbleTrail( vec3_t start, vec3_t end, int skipNum );
#endif
void CG_SpawnEffect( vec3_t org );
#ifdef MISSIONPACK
#ifndef TURTLEARENA // NO_KAMIKAZE_ITEM
void CG_KamikazeEffect( vec3_t org );
#endif
void CG_ObeliskExplode( vec3_t org, int entityNum );
void CG_ObeliskPain( vec3_t org );
#ifndef TURTLEARENA // POWERS
void CG_InvulnerabilityImpact( vec3_t org, vec3_t angles );
void CG_InvulnerabilityJuiced( vec3_t org );
#ifdef TA_WEAPSYS
void CG_LightningBoltBeam( projectileInfo_t *wi, vec3_t start, vec3_t end );
#else
void CG_LightningBoltBeam( vec3_t start, vec3_t end );
#endif
#endif
#endif
void CG_ScorePlum( int client, vec3_t org, int score );
#ifdef TURTLEARENA // NIGHTS_ITEMS
void CG_ChainPlum( int client, vec3_t org, int score, int chain, qboolean bonus );
#endif

#ifndef NOTRATEDM // No gibs.
void CG_GibPlayer( vec3_t playerOrigin );
void CG_BigExplode( vec3_t playerOrigin );
#endif

#ifndef TA_WEAPSYS
#ifndef NOBLOOD
void CG_Bleed( vec3_t origin, int entityNum );
#endif
#endif

localEntity_t *CG_MakeExplosion( vec3_t origin, vec3_t dir,
								qhandle_t hModel, qhandle_t shader, int msec,
								qboolean isSprite );
#ifdef TA_ENTSYS // EXP_SCALE
void CG_ExplosionEffect(vec3_t origin, int radius, int entity);
#endif
#ifdef IOQ3ZTM // LETTERBOX
void CG_ToggleLetterbox(qboolean onscreen, qboolean instant);
void CG_DrawLetterbox(void);
#endif

//
// cg_snapshot.c
//
void CG_ProcessSnapshots( void );
playerState_t *CG_LocalClientPlayerStateForClientNum( int clientNum );


//
// cg_spawn.c
//
qboolean    CG_SpawnString( const char *key, const char *defaultString, char **out );
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean    CG_SpawnFloat( const char *key, const char *defaultString, float *out );
qboolean    CG_SpawnInt( const char *key, const char *defaultString, int *out );
qboolean    CG_SpawnVector( const char *key, const char *defaultString, float *out );
void        CG_ParseEntitiesFromString( void );

//
// cg_info.c
//
void CG_LoadingString( const char *s );
#ifndef TURTLEARENA // NO_LOADING_ICONS
void CG_LoadingItem( int itemNum );
void CG_LoadingClient( int clientNum );
#endif
void CG_DrawInformation( void );

//
// cg_scoreboard.c
//
qboolean CG_DrawOldScoreboard( void );
void CG_DrawOldTourneyScoreboard( void );

//
// cg_consolecmds.c
//
qboolean CG_ConsoleCommand( void );
void CG_InitConsoleCommands( void );

//
// cg_servercmds.c
//
void CG_ExecuteNewServerCommands( int latestSequence );
void CG_ParseServerinfo( void );
void CG_SetConfigValues( void );
void CG_ShaderStateChanged(void);
#ifdef MISSIONPACK
void CG_LoadVoiceChats( void );
void CG_VoiceChatLocal( int localClientBits, int mode, qboolean voiceOnly, int clientNum, int color, const char *cmd );
void CG_PlayBufferedVoiceChats( void );
#endif

//
// cg_playerstate.c
//
void CG_Respawn( int clientNum );
void CG_TransitionPlayerState( playerState_t *ps, playerState_t *ops );
void CG_CheckChangedPredictableEvents( playerState_t *ps );
void CG_CheckGameSounds( void );

#ifdef TA_NPCSYS
//
// cg_npcs.c
//
void CG_NPC( centity_t *cent );
void CG_RegisterNPCVisuals( int npcNum );
#endif

//
// cg_atmospheric.c
//
void CG_EffectParse(const char *effectstr);
void CG_AddAtmosphericEffects(void);

//
// cg_polybus.c
//
polyBuffer_t* CG_PB_FindFreePolyBuffer( qhandle_t shader, int numVerts, int numIndicies );
void CG_PB_ClearPolyBuffers( void );
void CG_PB_RenderPolyBuffers( void );

//
// cg_particles.c
//
void	CG_ClearParticles (void);
void	CG_AddParticles (void);
void	CG_ParticleSnow (qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum);
void	CG_ParticleSmoke (qhandle_t pshader, centity_t *cent);
void	CG_AddParticleShrapnel (localEntity_t *le);
void	CG_ParticleSnowFlurry (qhandle_t pshader, centity_t *cent);
void	CG_ParticleBulletDebris (vec3_t	org, vec3_t vel, int duration);
void	CG_ParticleSparks (vec3_t org, vec3_t vel, int duration, float x, float y, float speed);
void	CG_ParticleDust (centity_t *cent, vec3_t origin, vec3_t dir);
void	CG_ParticleMisc (qhandle_t pshader, vec3_t origin, int size, int duration, float alpha);
void	CG_ParticleExplosion (char *animStr, vec3_t origin, vec3_t vel, int duration, int sizeStart, int sizeEnd);
extern qboolean		initparticles;
int CG_NewParticleArea ( int num );

//===============================================

//
// system traps
// These functions are how the cgame communicates with the main game system
//

// Additional shared traps in ../game/bg_misc.h

void		trap_GetClipboardData( char *buf, int bufsize );
void		trap_GetGlconfig( glconfig_t *glconfig );
// force a screen update, only used during gamestate load
void		trap_UpdateScreen( void );
int			trap_MemoryRemaining( void );
int			trap_GetVoipTime( int clientNum );
float		trap_GetVoipPower( int clientNum );
float		trap_GetVoipGain( int clientNum );
qboolean	trap_GetVoipMute( int clientNum );
qboolean	trap_GetVoipMuteAll( void );


// The glconfig_t will not change during the life of a cgame.
// If it needs to change, the entire cgame will be restarted, because
// all the qhandle_t are then invalid.
void		trap_GetGlconfig( glconfig_t *glconfig );

// the gamestate should be grabbed at startup, and whenever a
// configstring changes
void		trap_GetGameState( gameState_t *gamestate );

// cgame will poll each frame to see if a newer snapshot has arrived
// that it is interested in.  The time is returned seperately so that
// snapshot latency can be calculated.
void		trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime );

// a snapshot get can fail if the snapshot (or the entties it holds) is so
// old that it has fallen out of the client system queue
qboolean	trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot, void *playerStates, void *entities );

// retrieve a text command from the server stream
// the current snapshot will hold the number of the most recent command
// qfalse can be returned if the client system handled the command
// argc() / argv() can be used to examine the parameters of the command
qboolean	trap_GetServerCommand( int serverCommandNumber );

// returns the most recent command number that can be passed to GetUserCmd
// this will always be at least one higher than the number in the current
// snapshot, and it may be quite a few higher if it is a fast computer on
// a lagged connection
int			trap_GetCurrentCmdNumber( void );	

qboolean	trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd, int localClientNum );

// used for the weapon select and zoom
void		trap_SetUserCmdValue( int stateValue, float sensitivityScale, int localClientNum );

// send a string to the server over the network
void		trap_SendClientCommand( const char *s );

//
void		trap_SetNetFields( int entityStateSize, vmNetField_t *entityStateFields, int numEntityStateFields,
						int playerStateSize, vmNetField_t *playerStateFields, int numPlayerStateFields );

int			trap_GetDemoState( void );
int			trap_GetDemoPos( void );
void		trap_GetDemoName( char *buffer, int size );
int			trap_GetDemoLength( void );

// model collision
void		trap_CM_LoadMap( const char *mapname );
int			trap_CM_NumInlineModels( void );
clipHandle_t trap_CM_InlineModel( int index );		// 0 = world, 1+ = bmodels
clipHandle_t trap_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs, int contents );
clipHandle_t trap_CM_TempCapsuleModel( const vec3_t mins, const vec3_t maxs, int contents );
int			trap_CM_PointContents( const vec3_t p, clipHandle_t model );
int			trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );
void		trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask );
void		trap_CM_CapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask );
void		trap_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask,
					  const vec3_t origin, const vec3_t angles );
void		trap_CM_TransformedCapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
					  const vec3_t mins, const vec3_t maxs,
					  clipHandle_t model, int brushmask,
					  const vec3_t origin, const vec3_t angles );
void          trap_CM_BiSphereTrace( trace_t *results, const vec3_t start,
                const vec3_t end, float startRad, float endRad,
                clipHandle_t model, int mask );
void          trap_CM_TransformedBiSphereTrace( trace_t *results, const vec3_t start,
                const vec3_t end, float startRad, float endRad,
                clipHandle_t model, int mask,
                const vec3_t origin );

// Returns the projection of a polygon onto the solid brushes in the world
int			trap_CM_MarkFragments( int numPoints, const vec3_t *points, 
			const vec3_t projection,
			int maxPoints, vec3_t pointBuffer,
			int maxFragments, markFragment_t *fragmentBuffer );

void		trap_R_LoadWorldMap( const char *mapname );
qboolean	trap_GetEntityToken( char *buffer, int bufferSize );

// all media should be registered during level startup to prevent
// hitches during gameplay
qhandle_t	trap_R_RegisterModel( const char *name );			// returns rgb axis if not found
qhandle_t	trap_R_RegisterSkin( const char *name );			// returns all white if not found
qhandle_t	trap_R_RegisterShader( const char *name );			// returns all white if not found
qhandle_t	trap_R_RegisterShaderNoMip( const char *name );			// returns all white if not found
void		trap_R_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font);

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void		trap_R_ClearScene( void );
void		trap_R_AddRefEntityToScene( const refEntity_t *re );

// polys are intended for simple wall marks, not really for doing
// significant construction
void		trap_R_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts );
void		trap_R_AddPolysToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int numPolys );
void        trap_R_AddPolyBufferToScene( polyBuffer_t* pPolyBuffer );
void		trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void		trap_R_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b );
int			trap_R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
void		trap_R_RenderScene( const refdef_t *fd );
void		trap_R_SetColor( const float *rgba );	// NULL = 1,1,1,1
void		trap_R_SetClipRegion( const float *region );
void		trap_R_DrawStretchPic( float x, float y, float w, float h, 
				float s1, float t1, float s2, float t2, qhandle_t hShader );
void		trap_R_DrawRotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle );
void		trap_R_DrawStretchPicGradient( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader,
				const float *gradientColor, int gradientType );
void		trap_R_Add2dPolys( polyVert_t* verts, int numverts, qhandle_t hShader );
void		trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );
int			trap_R_LerpTag( orientation_t *tag, clipHandle_t mod, int startFrame, int endFrame, 
					   float frac, const char *tagName );
void		trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );
qboolean	trap_R_inPVS( const vec3_t p1, const vec3_t p2 );
void		trap_R_GetGlobalFog( fogType_t *type, vec3_t color, float *depthForOpaque, float *density );
void		trap_R_GetWaterFog( const vec3_t origin, fogType_t *type, vec3_t color, float *depthForOpaque, float *density );

// normal sounds will have their volume dynamically changed as their entity
// moves and the listener moves
void		trap_S_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx );

// a local sound is always played full volume
void		trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum );

void		trap_S_StopLoopingSound(int entnum);
void		trap_S_ClearLoopingSounds( qboolean killall );
void		trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void		trap_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void		trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin );

// respatialize recalculates the volumes of sound as they should be heard by the
// given entityNum and position
void		trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater, qboolean firstPerson );
sfxHandle_t	trap_S_RegisterSound( const char *sample, qboolean compressed );		// returns buzz if not found
int			trap_S_SoundDuration( sfxHandle_t handle );
void		trap_S_StartBackgroundTrack( const char *intro, const char *loop );	// empty name stops music
void		trap_S_StopBackgroundTrack( void );

void			trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
void			trap_Key_GetBindingBuf( int keynum, char *buf, int buflen );
void			trap_Key_SetBinding( int keynum, const char *binding );
qboolean		trap_Key_IsDown( int keynum );
qboolean		trap_Key_GetOverstrikeMode( void );
void			trap_Key_SetOverstrikeMode( qboolean state );
void			trap_Key_ClearStates( void );
int				trap_Key_GetCatcher( void );
void			trap_Key_SetCatcher( int catcher );
int				trap_Key_GetKey( const char *binding, int startKey );

// this returns a handle.  arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to qfalse (do not alter gamestate)
int			trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits);

// stops playing the cinematic and ends it.  should always return FMV_EOF
// cinematics must be stopped in reverse order of when they are started
e_status	trap_CIN_StopCinematic(int handle);

// will run a frame of the cinematic but will not draw it.  Will return FMV_EOF if the end of the cinematic has been reached.
e_status	trap_CIN_RunCinematic (int handle);

// draws the current frame
void		trap_CIN_DrawCinematic (int handle);

// allows you to resize the animation dynamically
void		trap_CIN_SetExtents (int handle, int x, int y, int w, int h);

/*
qboolean	trap_loadCamera(const char *name);
void		trap_startCamera(int time);
qboolean	trap_getCameraInfo(int time, vec3_t *origin, vec3_t *angles);
*/

#ifdef IOQ3ZTM // BONES
void		trap_R_AddRefEntityToScene_CustomSkeleton( const refEntity_t *re, const refSkeleton_t *rs );
int			trap_R_JointIndexForName(qhandle_t handle, const char *jointName);
qboolean	trap_R_SetupSkeleton(qhandle_t handle, refSkeleton_t *refSkel, int frame, int oldframe, float backlerp);
qboolean	trap_R_SetupPlayerSkeleton(qhandle_t handle, refSkeleton_t *refSkel,
							int legsFrame, int legsOldFrame, float legsBacklerp,
							int torsoFrame, int torsoOldFrame, float torsoBacklerp,
							int headFrame, int headOldFrame, float headBacklerp);
void		trap_R_MakeSkeletonAbsolute(const refSkeleton_t *in, refSkeleton_t *out);
#endif

