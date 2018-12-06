// GameCam v1.03 - camera proxy game module for Quake II
// Copyright (c) 1998-99, by Avi Rozen (Zung!)
// e-mail: zungbang@telefragged.com

// gamecam.h -- shared definitions for camera proxy module

#ifdef _WIN32
#pragma warning(disable : 4244)	// C4244 conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable : 4100)	// C4100 unreferenced formal parameter
#if _MSC_VER > 1500
#pragma warning(disable : 4996)	// disable warnings from VS 2010 about deprecated CRT functions (_CRT_SECURE_NO_WARNINGS).
#endif
#endif

#define	GAME_API_VERSION	3

#define GAMECAMVERNUM   "1.05qw"
#define GAMECAMVERSTATUS "FINAL"

#ifdef _WIN32
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
	extern HMODULE hGameDLL;
#else
	extern void *hGameDLL;
#endif

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

// quake2

#define	FRAMETIME		0.1F

#define	TAG_GAME	765
#define	TAG_LEVEL	766

#define	MAX_CLIENTS			256		
#define	MAX_EDICTS			1024	
#define	MAX_LIGHTSTYLES		256
#define	MAX_MODELS			256		
#define	MAX_SOUNDS			256		
#define	MAX_IMAGES			256
#define	MAX_ITEMS			256
#define MAX_GENERAL			(MAX_CLIENTS*2)	
#define	MAX_QPATH			64
#define	MAX_OSPATH			128
#define	MAX_INFO_KEY		64
#define	MAX_INFO_VALUE		64
#define	MAX_INFO_STRING		512
#define	MAX_STRING_CHARS	1024
#define	MAX_STRING_TOKENS	80
#define	MAX_TOKEN_CHARS		128


typedef unsigned char 		byte;
typedef enum bool_n {QFALSE, QTRUE} qboolean;

#ifndef NULL
#define NULL ((void *)0)
#endif

// angle indexes

#define	PITCH				0		
#define	YAW					1		
#define	ROLL				2		

// cvars:

#define	CVAR_ARCHIVE	1	
#define	CVAR_USERINFO	2	
#define	CVAR_SERVERINFO	4	
#define	CVAR_NOSET		8
#define	CVAR_LATCH		16

typedef struct cvar_s
{
	char		*name;
	char		*string;
	char		*latched_string;
	int			flags;
	qboolean	modified;
	float		value;
	struct cvar_s *next;
} cvar_t;


// edicts & clients:

typedef struct edict_s edict_t;
typedef struct gclient_s gclient_t;

typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec5_t[5];

// collision detection
#define	CONTENTS_SOLID			1		
#define	CONTENTS_WINDOW			2		
#define	CONTENTS_AUX			4
#define	CONTENTS_LAVA			8
#define	CONTENTS_SLIME			16
#define	CONTENTS_WATER			32
#define	CONTENTS_MIST			64
#define	LAST_VISIBLE_CONTENTS	64
#define	CONTENTS_AREAPORTAL		0x8000
#define	CONTENTS_PLAYERCLIP		0x10000
#define	CONTENTS_MONSTERCLIP	0x20000
#define	CONTENTS_CURRENT_0		0x40000
#define	CONTENTS_CURRENT_90		0x80000
#define	CONTENTS_CURRENT_180	0x100000
#define	CONTENTS_CURRENT_270	0x200000
#define	CONTENTS_CURRENT_UP		0x400000
#define	CONTENTS_CURRENT_DOWN	0x800000
#define	CONTENTS_ORIGIN			0x1000000	
#define	CONTENTS_MONSTER		0x2000000	
#define	CONTENTS_DEADMONSTER	0x4000000
#define	CONTENTS_DETAIL			0x8000000	
#define	CONTENTS_TRANSLUCENT	0x10000000	
#define	CONTENTS_LADDER			0x20000000

#define	SURF_LIGHT		0x1		
#define	SURF_SLICK		0x2		
#define	SURF_SKY		0x4		
#define	SURF_WARP		0x8		
#define	SURF_TRANS33	0x10
#define	SURF_TRANS66	0x20
#define	SURF_FLOWING	0x40	
#define	SURF_NODRAW		0x80	

// content masks

#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID|CONTENTS_WINDOW)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW)
#define	MASK_MONSTERSOLID		(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEADMONSTER)
#define MASK_CURRENT			(CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)

#define	AREA_SOLID		1
#define	AREA_TRIGGERS	2

typedef struct cplane_s
{
	vec3_t	normal;
	float	dist;
	byte	type;
	byte	signbits;
	byte	pad[2];
} cplane_t;

typedef struct csurface_s
{
	char		name[16];
	int			flags;
	int			value;
} csurface_t;

typedef struct trace_s
{
	qboolean	allsolid;
	qboolean	startsolid;
	float		fraction;
	vec3_t		endpos;
	cplane_t	plane;
	csurface_t	*surface;
	int			contents;
	struct edict_s	*ent;
} trace_t;


typedef enum pmtype_n
{
	PM_NORMAL,
	PM_SPECTATOR,
	PM_DEAD,
	PM_GIB,
	PM_FREEZE
} pmtype_t;


#define	PMF_DUCKED			1
#define	PMF_JUMP_HELD		2
#define	PMF_ON_GROUND		4
#define	PMF_TIME_WATERJUMP	8	
#define	PMF_TIME_LAND		16	
#define	PMF_TIME_TELEPORT	32	
#define PMF_NO_PREDICTION	64	

typedef struct pmove_state_s
{
	pmtype_t	pm_type;
	short		origin[3];
	short		velocity[3];
	byte		pm_flags;
	byte		pm_time;
	short		gravity;
	short		delta_angles[3];
} pmove_state_t;

// sound channels

#define	CHAN_AUTO               0
#define	CHAN_WEAPON             1
#define	CHAN_VOICE              2
#define	CHAN_ITEM               3
#define	CHAN_BODY               4
// modifier flags
#define	CHAN_NO_PHS_ADD			8	
#define	CHAN_RELIABLE			16	

// sound attenuation values
#define	ATTN_NONE               0	
#define	ATTN_NORM               1
#define	ATTN_IDLE               2
#define	ATTN_STATIC             3	

// player stats

#define STAT_HEALTH_ICON		0
#define	STAT_HEALTH				1
#define	STAT_AMMO_ICON			2
#define	STAT_AMMO				3
#define	STAT_ARMOR_ICON			4
#define	STAT_ARMOR				5
#define	STAT_SELECTED_ICON		6
#define	STAT_PICKUP_ICON		7
#define	STAT_PICKUP_STRING		8
#define	STAT_TIMER_ICON			9
#define	STAT_TIMER				10
#define	STAT_HELPICON			11
#define	STAT_SELECTED_ITEM		12
#define	STAT_LAYOUTS			13
#define	STAT_FRAGS				14
#define	STAT_FLASHES			15

#define STAT_CAM_OFFSET			27 // GameCam
#define STAT_CAM_LOCATION		28 // GameCam
#define STAT_TICKER_OFFSET		29 // GameCam
#define STAT_TICKER				30 // GameCam
#define STAT_ID_VIEW			31 // GameCam 

#define	MAX_STATS				32


typedef struct player_state_s
{
	pmove_state_t	pmove;
	vec3_t		viewangles;
	vec3_t		viewoffset;
	vec3_t		kick_angles;
	vec3_t		gunangles;
	vec3_t		gunoffset;
	int			gunindex;
	int			gunframe;
	float		blend[4];
	float		fov;
	int			rdflags;
	short		stats[MAX_STATS];
} player_state_t;

struct gclient_s
{
	player_state_t	ps;
	int	ping;
};


typedef struct entity_state_s
{
	int		number;
	vec3_t	origin;
	vec3_t	angles;
	vec3_t	old_origin;
	int		modelindex;
	int		modelindex2, modelindex3, modelindex4;
	int		frame;
	int		skinnum;
	int		effects;
	int		renderfx;
	int		solid;
	int		sound;
	int		event;
} entity_state_t;


typedef struct link_s
{
	struct link_s	*prev, *next;
} link_t;

// edict->svflags
#define	SVF_NOCLIENT			0x00000001
#define	SVF_DEADMONSTER			0x00000002
#define	SVF_MONSTER				0x00000004

typedef enum solid_n
{
	SOLID_NOT,
	SOLID_TRIGGER,
	SOLID_BBOX,
	SOLID_BSP
} solid_t;

#define	MAX_ENT_CLUSTERS	16

struct edict_s
{
	entity_state_t	s;
	struct gclient_s	*client;
	qboolean	inuse;
	int			linkcount;
	link_t		area;
	int			num_clusters;
	int			clusternums[MAX_ENT_CLUSTERS];
	int			headnode;
	int			areanum, areanum2;
	int			svflags;
	vec3_t		mins, maxs;
	vec3_t		absmin, absmax, size;
	solid_t		solid;
	int			clipmask;
	edict_t		*owner;
};

#define	BUTTON_ATTACK		1
#define	BUTTON_USE			2
#define	BUTTON_ANY			128

typedef struct usercmd_s
{
	byte	msec;
	byte	buttons;
	short	angles[3];
	short	forwardmove, sidemove, upmove;
	byte	impulse;		
	byte	lightlevel;		
} usercmd_t;


#define	MAXTOUCH	32

typedef struct pmove_s
{
	pmove_state_t	s;
	usercmd_t		cmd;
	qboolean		snapinitial;
	int			numtouch;
	struct edict_s	*touchents[MAXTOUCH];
	vec3_t		viewangles;
	float		viewheight;
	vec3_t		mins, maxs;
	struct edict_s	*groundentity;
	int			watertype;
	int			waterlevel;
	trace_t		(*trace) (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
	int			(*pointcontents) (vec3_t point);
} pmove_t;

// player_state_t->refdef flags

#define	RDF_UNDERWATER		1
#define RDF_NOWORLDMODEL	2

// export/import:

#define	PRINT_LOW			0
#define	PRINT_MEDIUM		1
#define	PRINT_HIGH			2
#define	PRINT_CHAT			3

#define	ERR_FATAL			0
#define	ERR_DROP			1
#define	ERR_DISCONNECT		2

#define	PRINT_ALL			0
#define PRINT_DEVELOPER		1
#define PRINT_ALERT			2		

typedef enum multicast_n
{
    MULTICAST_ALL,
    MULTICAST_PHS,
    MULTICAST_PVS,
    MULTICAST_ALL_R,
    MULTICAST_PHS_R,
    MULTICAST_PVS_R
} multicast_t;


typedef struct game_import_s
{
	void	(*bprintf) (int printlevel, char *fmt, ...);
	void	(*dprintf) (char *fmt, ...);
	void	(*cprintf) (edict_t *ent, int printlevel, char *fmt, ...);
	void	(*centerprintf) (edict_t *ent, char *fmt, ...);
	void	(*sound) (edict_t *ent, int channel, int soundindex, float volume, float attenuation, float timeofs);
	void	(*positioned_sound) (vec3_t origin, edict_t *ent, int channel, int soundinedex, float volume, float attenuation, float timeofs);
	void	(*configstring) (int num, char *string);
	void	(*error) (char *fmt, ...);
	int		(*modelindex) (char *name);
	int		(*soundindex) (char *name);
	int		(*imageindex) (char *name);
	void	(*setmodel) (edict_t *ent, char *name);
	trace_t	(*trace) (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passent, int contentmask);
	int		(*pointcontents) (vec3_t point);
	qboolean	(*inPVS) (vec3_t p1, vec3_t p2);
	qboolean	(*inPHS) (vec3_t p1, vec3_t p2);
	void		(*SetAreaPortalState) (int portalnum, qboolean open);
	qboolean	(*AreasConnected) (int area1, int area2);
	void	(*linkentity) (edict_t *ent);
	void	(*unlinkentity) (edict_t *ent);
	int		(*BoxEdicts) (vec3_t mins, vec3_t maxs, edict_t **list,	int maxcount, int areatype);
	void	(*Pmove) (pmove_t *pmove);
	void	(*multicast) (vec3_t origin, multicast_t to);
	void	(*unicast) (edict_t *ent, qboolean reliable);
	void	(*WriteChar) (int c);
	void	(*WriteByte) (int c);
	void	(*WriteShort) (int c);
	void	(*WriteLong) (int c);
	void	(*WriteFloat) (float f);
	void	(*WriteString) (char *s);
	void	(*WritePosition) (vec3_t pos);
	void	(*WriteDir) (vec3_t pos);
	void	(*WriteAngle) (float f);
	void	*(*TagMalloc) (int size, int tag);
	void	(*TagFree) (void *block);
	void	(*FreeTags) (int tag);
	cvar_t	*(*cvar) (char *var_name, char *value, int flags);
	cvar_t	*(*cvar_set) (char *var_name, char *value);
	cvar_t	*(*cvar_forceset) (char *var_name, char *value);
	int		(*argc) (void);
	char	*(*argv) (int n);
	char	*(*args) (void);
	void	(*AddCommandString) (char *text);
	void	(*DebugGraph) (float value, int color);
} game_import_t;


typedef struct game_export_s
{
	int			apiversion;
	void		(*Init) (void);
	void		(*Shutdown) (void);
	void		(*SpawnEntities) (char *mapname, char *entstring, char *spawnpoint);
	void		(*WriteGame) (char *filename, qboolean autosave);
	void		(*ReadGame) (char *filename);
	void		(*WriteLevel) (char *filename);
	void		(*ReadLevel) (char *filename);
	qboolean	(*ClientConnect) (edict_t *ent, char *userinfo);
	void		(*ClientBegin) (edict_t *ent);
	void		(*ClientUserinfoChanged) (edict_t *ent, char *userinfo);
	void		(*ClientDisconnect) (edict_t *ent);
	void		(*ClientCommand) (edict_t *ent);
	void		(*ClientThink) (edict_t *ent, usercmd_t *cmd);
	void		(*RunFrame) (void);
	void		(*ServerCommand) (void);
	struct edict_s	*edicts;
	int			edict_size;
	int			num_edicts;
	int			max_edicts;
} game_export_t;

// network

#define	ANGLE2SHORT(x)	((int)((x)*65536/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*(360.0F/65536))

// config strings

#define	CS_NAME				0
#define	CS_CDTRACK			1
#define	CS_SKY				2
#define	CS_SKYAXIS			3
#define	CS_SKYROTATE		4
#define	CS_STATUSBAR		5
#define CS_AIRACCEL			29
#define	CS_MAXCLIENTS		30
#define	CS_MAPCHECKSUM		31
#define	CS_MODELS			32
#define	CS_SOUNDS			(CS_MODELS+MAX_MODELS)
#define	CS_IMAGES			(CS_SOUNDS+MAX_SOUNDS)
#define	CS_LIGHTS			(CS_IMAGES+MAX_IMAGES)
#define	CS_ITEMS			(CS_LIGHTS+MAX_LIGHTSTYLES)
#define	CS_PLAYERSKINS		(CS_ITEMS+MAX_ITEMS)
#define CS_GENERAL			(CS_PLAYERSKINS+MAX_CLIENTS)
#define	MAX_CONFIGSTRINGS	(CS_GENERAL+MAX_GENERAL)

#define CS_CAM_LOCATION		(MAX_CONFIGSTRINGS - 2)	// GameCam
#define CS_TICKER			(MAX_CONFIGSTRINGS - 1)	// GameCam

typedef char config_strings_t[MAX_CONFIGSTRINGS][MAX_QPATH + 1];

//QW// The 2080 magic number comes from q_shared.h of the original game.
// No game mod can go over this 2080 limit.
#if (MAX_CONFIGSTRINGS > 2080)
	#error MAX_CONFIGSTRINGS > 2080
#endif

// math

#ifndef M_PI
#define M_PI		3.14159265358979323846F
#endif

#define DotProduct(x,y)			(x[0]*y[0]+x[1]*y[1]+x[2]*y[2])
#define VectorSubtract(a,b,c)	(c[0]=a[0]-b[0],c[1]=a[1]-b[1],c[2]=a[2]-b[2])
#define VectorAdd(a,b,c)		(c[0]=a[0]+b[0],c[1]=a[1]+b[1],c[2]=a[2]+b[2])
#define VectorCopy(a,b)			(b[0]=a[0],b[1]=a[1],b[2]=a[2])
#define VectorClear(a)			(a[0]=a[1]=a[2]=0)
#define VectorNegate(a,b)		(b[0]=-a[0],b[1]=-a[1],b[2]=-a[2])
#define VectorSet(v, x, y, z)	(v[0]=(x), v[1]=(y), v[2]=(z))

// dmflags->value flags
#define	DF_NO_HEALTH		0x00000001	// 1
#define	DF_NO_ITEMS			0x00000002	// 2
#define	DF_WEAPONS_STAY		0x00000004	// 4
#define	DF_NO_FALLING		0x00000008	// 8
#define	DF_INSTANT_ITEMS	0x00000010	// 16
#define	DF_SAME_LEVEL		0x00000020	// 32
#define DF_SKINTEAMS		0x00000040	// 64
#define DF_MODELTEAMS		0x00000080	// 128
#define DF_NO_FRIENDLY_FIRE	0x00000100	// 256
#define	DF_SPAWN_FARTHEST	0x00000200	// 512
#define DF_FORCE_RESPAWN	0x00000400	// 1024
#define DF_NO_ARMOR			0x00000800	// 2048
#define DF_ALLOW_EXIT		0x00001000	// 4096
#define DF_INFINITE_AMMO	0x00002000	// 8192
#define DF_QUAD_DROP		0x00004000	// 16384
#define DF_FIXED_FOV		0x00008000	// 32768

// GameCam network tracking

#define	svc_bad					0
#define	svc_muzzleflash			1
#define	svc_muzzleflash2		2
#define	svc_temp_entity			3
#define	svc_layout				4
#define	svc_inventory			5
#define	svc_nop					6
#define	svc_disconnect			7
#define	svc_reconnect			8
#define	svc_sound				9
#define	svc_print				10
#define	svc_stufftext			11
#define	svc_serverdata			12
#define	svc_configstring		13
#define	svc_spawnbaseline		14
#define	svc_centerprint			15
#define	svc_download			16
#define	svc_playerinfo			17
#define	svc_packetentities		18
#define	svc_deltapacketentities	19
#define svc_frame				20

typedef enum write_entry_type_n
{
	WRITE_BUF_CHAR,
	WRITE_BUF_BYTE,
	WRITE_BUF_SHORT,
	WRITE_BUF_LONG,
	WRITE_BUF_FLOAT,
	WRITE_BUF_STRING,
	WRITE_BUF_POSITION,
	WRITE_BUF_DIR,
	WRITE_BUF_ANGLE,
	WRITE_BUF_NULL = 0x8000 // quake2 allows NULL in WriteDir
} write_entry_type_t;

#define MAX_BUF_ENTRIES 0x10000	//same size as lox bigbuffer
#define MAX_BUF_DATA 0x10000

typedef struct write_buffer_s {
	qboolean reliable;
	long length;
	long entries;
	int type[MAX_BUF_ENTRIES];
	byte data[MAX_BUF_DATA];
} write_buffer_t;


// camera mode
#define CAMERA_FREE				0x0002	// same as noclip, but spectator is invisible
#define CAMERA_CHASE			0x0001	// chase cam (similar to CTF)
#define CAMERA_ACTION			0x0000	// action cam (tracks action)

// ticker tape
typedef struct ticker_s
{
	int func;
	int delay, counter;				// delay in frames
	int startspace, endspace;
	int offset;
	int place;
	int place_target;
	int times, remaining;
	qboolean centered;
	char text[MAX_STRING_CHARS];		// text to be displayed
	char store[MAX_STRING_CHARS];	// store the original text line
	//char script[MAX_OSPATH]; 		// path of the script to be chained-to or loaded from this message
	struct ticker_s *loop;			// pointer to the return place in the script (for loops)
	struct ticker_s *next;			// pointer to next script function
} ticker_t;

#define TICKER_MAX_CHARS		39	// chars in tape

// fixed cameras list
typedef struct camera_s
{
	char name[MAX_INFO_VALUE];
	vec3_t origin;
	vec3_t angles;
	float fov;
	struct camera_s *prev;
	struct camera_s *next;
} camera_t;

// menus
enum pmenu_n
{
	PMENU_ALIGN_LEFT,
	PMENU_ALIGN_CENTER,
	PMENU_ALIGN_RIGHT
};

enum pmenu_flags_n
{
	PMENU_FLAG_CREATE,
	PMENU_FLAG_DESTROY,
	PMENU_FLAG_SELECT
};

typedef struct pmenu_s
{
	char *text;
	int align;
	char *option;
	void (*SelectFunc)(edict_t *ent, struct pmenu_s *entry, int flag);
} pmenu_t;

typedef struct pmenuhnd_s {
	pmenu_t *entries;
	int cur;
	int num;
	struct pmenuhnd_s *parent;
} pmenuhnd_t;

// clients data
typedef struct clients_s 
{
	qboolean	inuse;		// connected?
	qboolean	begin;		// entered the game?
	qboolean	spectator;	// spectator?
	qboolean	instant_spectator;	// make client spectator at first ClientBegin?
	qboolean	statusbar_removed;	// statusbar removed?
	qboolean	update_chase; // update chase message?
	qboolean	inven;		// spectator showing inventory?
	qboolean	score;		// spectator showing score?
	qboolean	help;		// spectator showing help?
	qboolean	menu;		// spectator showing menu?
	qboolean	id;			// spectator showing player id?
	qboolean	ticker;		// spectator showing ticker? 
	qboolean	fixed;		// use fixed cameras?
	qboolean	layouts;	// spectator showing layouts ?
	qboolean	welcome;	// was client welcomed?
	qboolean	demo;		// auto-record demo?
	int			limbo_time;	// client enters limbo time upon 'camera off'
	int			target;			// ID of client the spectator is following
	int			mask;			// network message filter
	int			mode;			// camera mode
	char		userinfo[MAX_INFO_STRING];		// client userinfo
	int			item;			// current item
	short		items[MAX_ITEMS]; // all items
	char		layout[2048];
	int			last_score;   // last time layout was sent to spectator
	qboolean	intermission; // was intermission scoreboard updated			
	int		    motddelay;
	int			ticker_frame; // last frame when ticker was updated
	int			ticker_delay; // delay before status bar is displayed
	int			select;		  // for random-access to targets (camera select)
	camera_t	*camera;	  // for manual access to cameras (camera next/prev/auto)
	pmenuhnd_t	*menu_hnd;	  // menu handle

	// chase camera position
	qboolean	chase_auto;	  // creep-cam mode enabled
	float		chase_distance; // distance behind player
	float		chase_height; // height of camera
	float 		chase_angle;  // angle of camera
	float 		chase_yaw;	  // yaw of camera
	float		chase_pitch;  // pitch of camera
	edict_t		*creep_target;// secondry target
	// flood control
	float		flood_locktill;		// locked from talking
	float		flood_when[10];		// when messages were said
	int			flood_whenhead;		// head pointer for when said

	// q2cam begin
	qboolean	reset_layouts;
	pmtype_t	last_pmtype;
	edict_t		*pTarget;
	int			iMode;
    qboolean    bWatchingTheDead;
	qboolean	bWatchingTheWall;
    vec3_t      vDeadOrigin;
    float       fXYLag;
    float       fZLag;
    float       fAngleLag;
	float		last_move_time;
	float		last_switch_time;
	float		fixed_switch_time;
	byte		oldbuttons;
	qboolean	no_priority;
	qboolean	override;		// signal that player must be followed
	int			num_visible;
	qboolean	valid_target;
	// q2cam end

	vec3_t		cmd_angles;			// angles sent over in the last command
	pmove_state_t old_pmove;	// for movement code
	vec3_t		velocity;
	vec3_t		v_angle;
	float		viewheight;
	struct edict_s	*groundentity;
	int			watertype;
	int			waterlevel;
} clients_t;

// edict access macros
#define Edict(i)		((edict_t *) (((byte *) gce->edicts) + (gce->edict_size * (i))))
#define numEdict(ent)	((((byte *) (ent)) - ((byte *) gce->edicts)) / gce->edict_size)
#define	FOFS(x)			(int)&(((edict_t *)0)->x)

// spectator macros
#define isSpectator(clientID)	(clients[clientID].spectator)
#define isMenuOn(clientID)		(clients[clientID].menu)
#define isInvenOn(clientID)		(clients[clientID].inven)
#define isFollowing(clientID)	((clients[clientID].mode & CAMERA_CHASE) != 0)

// GameCam configuration flags

#define GCF_SUICIDE			0x00000001	// default: on		// 1
#define GCF_ALLOW_CHASE		0x00000002	// default: on		// 2
#define GCF_ALLOW_FREE		0x00000004	// default: off		// 4
#define GCF_ALLOW_SAY		0x00000008	// default: off		// 8
#define GCF_VERBOSE			0x00000010	// default: off		// 16 
#define GCF_WELCOME			0x00000020	// default: on		// 32
#define GCF_AVOID_BOTS		0x00000040	// default: on		// 64
#define GCF_DEFAULT_ID		0x00000080	// default: on		// 128
#define GCF_DEFAULT_TICKER	0x00000100	// default: off		// 256
#define GCF_DEFAULT_FIXED	0x00000200	// default: on		// 512
#define GCF_DEFAULT_AUTO	0x00000400	// default: on		// 1024
#define GCF_DEFAULT_DEMO	0x00000800	// default: off	    // 2048
#define GCF_LOCK_SERVER		0x00001000	// default: off		// 4096
#define GCF_TEAMS			0x00002000	// default: off		// 8192
#define GCF_SPECTATOR		0x00004000	// default: on		// 16384
//#define GCF_SCOREBOARD_HACK	0x00008000	// default: off		// 32768
#define GCF_CHASE_OBSERVERS	0x00020000	// default: off		// 131072

// gc_main.c

#ifdef _WIN32

extern game_import_t gi;
extern game_export_t ge;

#else // unix seems to use global symbols so different names are required

extern game_import_t qi;
extern game_export_t qe;
#define gi qi
#define ge qe

#endif

extern game_import_t gci;
extern game_export_t *gce;

extern cvar_t *gc_version;
extern cvar_t *gc_flags;		
extern cvar_t *gc_count;	
extern cvar_t *gc_password;	
extern cvar_t *gc_maxcameras;
extern cvar_t *gc_motd;		
extern cvar_t *gc_autocam;	
extern cvar_t *gc_maxplayers;
extern cvar_t *gc_ticker;	
extern cvar_t *gc_maxscores;
extern cvar_t *gc_teams;
extern cvar_t *gc_update;

extern cvar_t *proxy;
extern cvar_t *nextproxy;

extern cvar_t *basedir;
extern cvar_t *game;
extern cvar_t *maxclients;
extern cvar_t *dedicated;
extern cvar_t *sv_gravity;
extern cvar_t *timelimit;
extern cvar_t *dmflags;
extern cvar_t *maxspectators;
extern cvar_t *deathmatch;
extern cvar_t *spectator_password;
extern cvar_t *needpass;
extern cvar_t *flood_msgs;
extern cvar_t *flood_persecond;
extern cvar_t *flood_waitdelay;
extern cvar_t *gc_set_fov;
extern cvar_t *gc_rename_client;

extern clients_t *clients;
extern int cam_count;
extern qboolean ctf_game;

// gc_net.c

extern write_buffer_t write_buffer;
extern char first_print_message[MAX_STRING_CHARS];

void ClearBuffer(void);
void WriteChar (int c);
void WriteByte (int c);
void WriteShort (int c);
void WriteLong (int c);
void WriteFloat (float f);
void WriteString (char *s);
void WritePosition (vec3_t pos);
void WriteDir (vec3_t pos);
void WriteAngle (float f);
void multicast (vec3_t origin, multicast_t to);
void unicast (edict_t *ent, qboolean reliable);
void cprintf (edict_t *ent, int printlevel, char *fmt, ...);
void centerprintf (edict_t *ent, char *fmt, ...);

// gc_config.c

extern config_strings_t ConfigStrings;
void configstring (int num, char *string);
void setmodel (edict_t *ent, char *name);
int modelindex (char *name);

// gc_frame.c

extern long framenum;
extern float leveltime;
extern long match_startframe;
extern long match_offsetframes;
extern long match_updateframes;
extern qboolean match_started;
extern qboolean match_10sec;
extern qboolean intermission;

void RunFrame (void);
void ClientThink (edict_t *ent, usercmd_t *cmd);
void Pmove (pmove_t *pmove);

// gc_connect.c

extern vec3_t spawn_origin;

qboolean ClientConnect (edict_t *ent, char *userinfo);
void ClientBegin (edict_t *ent);
void ClientUserinfoChanged (edict_t *ent, char *userinfo);
void ClientDisconnect (edict_t *ent);
void SpawnEntities (char *mapname, char *entstring, char *spawnpoint);

// gc_cmd.c

extern edict_t *wait_camera;
extern edict_t *wait_inven;
extern edict_t *wait_score;
extern edict_t *wait_help;
extern edict_t *wait_cprintf;
extern char captured_print_message[MAX_STRING_CHARS];

void demoOFF (edict_t *ent);
void demoON (edict_t *ent);
void ClientCommand (edict_t *ent);
void ServerCommand (void);
void GameCommand (edict_t *ent, char *command);
int gc_argc (void);
char *gc_argv (int n);
char *gc_args (void);
void UpdateScore (int clientID);
void ReturnToGame (edict_t *ent);
void CameraOff (edict_t *ent);
void SpectatorBegin (edict_t *ent, char *password, qboolean validate);
void SpectatorEnd (edict_t *ent, char *password);

// gc_utils.c

extern edict_t	*pm_passent;

trace_t	PM_trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
int SpectatorOf (edict_t *ent, int mask);
int NextClient (int clientID);
int PrevClient (int clientID);
int ClosestClient(int clientID);
qboolean IsVisible(edict_t *pPlayer1, edict_t *pPlayer2, float maxrange);
edict_t *ClosestVisible(edict_t *ent, float maxrange, qboolean pvs);
int numPlayers(void);
int getBestClient (void);
qboolean detect_intermission (void);
char *highlightText (char *text);
char *motd (char *motdstr);
qboolean trimcmp (char *a, char *b);
qboolean blank_or_remark (char *line);
void set_fov (edict_t *ent, float fov, qboolean force);
qboolean sameTeam (edict_t *player1, edict_t *player2);
float anglemod (float angle);
float anglediff (float a, float b);

// gc_id.c

extern vec3_t vec3_origin;

void Com_sprintf (char *dest, int size, char *fmt, ...);
int Q_stricmp (char *s1, char *s2);
int	Q_strcasecmp(const char *s1, const char *s2);
int Q_strncasecmp(const char *s1, const char *s2, size_t n);
void Info_SetValueForKey (char *s, char *key, char *value);
char *Info_ValueForKey (char *s, char *key);
char *COM_Parse (char **data_p);
void SV_CalcBlend (edict_t *ent);
void AngleVectors (vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
vec_t VectorNormalize (vec3_t v);
void VectorMA (vec3_t veca, float scale, vec3_t vecb, vec3_t vecc);
void VectorScale (vec3_t in, vec_t scale, vec3_t out);
float VectorLength(vec3_t v);
void vectoangles (vec3_t value1, vec3_t angles);
float *tv (float x, float y, float z);
char *vtos (vec3_t v);
float vectoyaw (vec3_t vec);
void SetIDView(edict_t *ent, qboolean exclude_bots);

// gc_menu.c

void showMenu (int clientID);
void hideMenu (int clientID);
void PMenu_Open (edict_t *ent, pmenu_t *entries, int cur, int num, pmenuhnd_t *parent);
void PMenu_Close (edict_t *ent);
void PMenu_Parent (edict_t *ent);
void PMenu_Update (edict_t *ent);
void PMenu_Next (edict_t *ent);
void PMenu_Prev (edict_t *ent);
void PMenu_Select (edict_t *ent);

// gc_ticker.c

extern int ticker_flags;

void ticker_clear (edict_t *ent);
void ticker_setup (edict_t *ent);
void ticker_wrapup (edict_t *ent);
void ticker_frame (edict_t *ent);
int  ticker_update (void);
void ticker_init (void);
void ticker_shutdown (void);
void ticker_update_camera (edict_t *ent, camera_t *camera);
void ticker_remove_statusbar (edict_t *ent);
void ticker_restore_statusbar (edict_t *ent);

// gc_fixed.c

extern char current_map[MAX_INFO_VALUE];
extern camera_t *cameras;

void camera_fixed_free (void);
void camera_fixed_load (char *mapname);
void camera_fixed_save (char *mapname);
camera_t *camera_fixed_add (edict_t *ent, char *name);
void camera_fixed_update (camera_t *camera, edict_t *ent, char *name);
void camera_fixed_remove (camera_t *camera);
void camera_fixed_list (edict_t *ent);
camera_t *camera_fixed_find (edict_t *ent);
void camera_fixed_select (camera_t *camera, edict_t *ent, edict_t *target);

// gc_free.c

void camera_free_setup (int clientID);
void camera_free_wrapup (int clientID);
void camera_free_begin (int clientID);
void camera_free_add_target (int other);
void camera_free_remove_target (int other);
void camera_free_frame (int clientID);
void camera_free_think (int clientID, usercmd_t *cmd);

// gc_chase.c

void camera_chase_setup (int clientID);
void camera_chase_wrapup (int clientID);
void camera_chase_begin (int clientID);
void camera_chase_add_target (int other);
void camera_chase_remove_target (int other);
void camera_chase_frame (int clientID);
void camera_chase_think (int clientID, usercmd_t *cmd);

// gc_creep.c 

edict_t *camera_creep_target (int clientID);
void camera_creep_angle (int clientID);
void camera_creep_viewangles (int clientID);

// gc_action.c

#define CAMERA_MIN_RANGE		48
#define CAMERA_MAX_RANGE		800

#define CAMERA_TARGET_MODEL		0x0001
#define CAMERA_TARGET_GUN		0x0002
#define CAMERA_TARGET_MODEL2	0x0004
#define CAMERA_TARGET_MODEL3	0x0008
#define CAMERA_TARGET_MODEL4	0x0010
#define CAMERA_TARGET_SHELL		0x0020

typedef struct priority_list_s {
	int  priority;
	int  type;
	char path[MAX_QPATH];
	int	 effects;
	int	 renderfx;
	struct priority_list_s *next;
} priority_list_t;

extern priority_list_t *priority_list;

void camera_action_setup (int clientID);
void camera_action_wrapup (int clientID);
void camera_action_begin (int clientID);
void camera_action_add_target (int other);
void camera_action_remove_target (int other);
void camera_action_frame (int clientID);
void camera_action_think (int clientID, usercmd_t *cmd);
qboolean ParsePriorityList (void);

// q2cam begin

enum cam_modes_n
{
    CAM_NORMAL_MODE,
    CAM_FOLLOW_MODE
};

#define DAMP_ANGLE_Y				6
#define DAMP_VALUE_XY				6
#define DAMP_VALUE_Z				3
#define MAX_VISIBLE_RANGE			1000
#define CAMERA_SWITCH_TIME			20
#define CAMERA_DEAD_SWITCH_TIME		2
#define CAMERA_MIN_SWITCH_TIME		4
#define CAMERA_FIXED_SWITCH_TIME	2
extern edict_t *pDeadPlayer;

// q2cam end

#define MOTD_STRING "\\!GameCam\\! General Admission\\nEnjoy the Show!\\n\\n\\ntype '\\!camera\\!' to see the menu"

