// GameCam v1.02 - camera proxy game module for Quake II
// Copyright (c) 1998-99, by Avi Rozen (Zung!)
// e-mail: zungbang@telefragged.com

#include "gamecam.h"

#ifdef _WIN32
	HMODULE hGameDLL;
#else
	void *hGameDLL;
#endif

game_import_t	gci; // original import functions (used by proxy)
game_export_t	*gce;// export functions returned to server
#ifdef _WIN32
game_import_t   gi;  // import functions to be used by game/next proxy module
game_export_t   ge;  // export functions returned from game/next proxy module
#else
game_import_t   qi;  // import functions to be used by game/next proxy module
game_export_t   qe;  // export functions returned from game/next proxy module
#endif

cvar_t *gc_version;
cvar_t *gc_flags;		
cvar_t *gc_count;	
cvar_t *gc_password;	
cvar_t *gc_maxcameras;
cvar_t *gc_motd;		
cvar_t *gc_autocam;	
cvar_t *gc_maxplayers;
cvar_t *gc_ticker;	
cvar_t *gc_maxscores;
cvar_t *gc_teams;
cvar_t *gc_update;

cvar_t  *proxy;
cvar_t	*nextproxy;

cvar_t  *basedir;
cvar_t  *game;
cvar_t	*maxclients;
cvar_t	*dedicated;
cvar_t  *sv_gravity;
cvar_t	*timelimit;
cvar_t	*dmflags;
cvar_t	*maxspectators;
cvar_t	*deathmatch;
cvar_t	*spectator_password;
cvar_t	*needpass;
cvar_t	*flood_msgs;
cvar_t	*flood_persecond;
cvar_t	*flood_waitdelay;
cvar_t	*gc_set_fov;
cvar_t	*gc_rename_client;


// GameCam Data
clients_t *clients;
int cam_count = 0;
qboolean ctf_game;


void ShutdownGameCam (void)
{
	ticker_shutdown ();
	camera_fixed_free ();
	gci.dprintf ("==== Shutdown (GameCam) ====\n");
	// remove gc_count from server info
	gci.cvar_forceset("gc_count","");
	ge.Shutdown(); // shutdown game DLL
#ifdef _WIN32
	FreeLibrary(hGameDLL);
#else
	dlclose(hGameDLL);
#endif
}


void InitGameCam (void)
{
	/***
	int i;
	for (i=0; i<256; i++)
	gci.dprintf ("[%.3d]=%c\n",i,i);
	***/
	gci.dprintf ("==== Init (GameCam "GAMECAMVERNUM" "__DATE__") ====\n");
	ClearBuffer ();
	clients = gci.TagMalloc (sizeof(clients_t) * ((int) maxclients->value), TAG_GAME);
	memset (clients, 0, sizeof(clients_t) * ((int) maxclients->value));
	gci.cvar_forceset("gc_count","0");
	ParsePriorityList();
	ctf_game = (strstr (game->string, "ctf") || strstr (game->string, "CTF"));
	ge.Init();
}


void LoadGameModule(char *game_basedir, char *game_dir)
{
	char GameLibPath[MAX_OSPATH];

	// if no game dir or bad game dir then load game from baseq2
	if (!game_dir || game_dir[0] == '\0' || 
		strchr (game_dir, '\\') || strchr (game_dir, '/') || strstr (game_dir, "..")) 
	{
		gci.cvar_forceset ("nextproxy","");
		sprintf (GameLibPath,"%s/baseq2/" GAME_MODULE, basedir->string);
		gci.dprintf ("...loading default game module \"%s\": ", GameLibPath);
	} 
	else if (game_basedir[0] != '\0') 
	{
		// load next proxy module
		sprintf (GameLibPath, "%s/%s/%s/" PROXY_MODULE, basedir->string, game_basedir, game_dir);
		gci.dprintf ("...loading proxy module \"%s\": ", GameLibPath);
	}
	else 
	{
		// load real game module
		sprintf (GameLibPath,"%s/%s/" GAME_MODULE, basedir->string, game_dir);
		gci.dprintf ("...loading game module \"%s\": ", GameLibPath);
	}

#ifdef _WIN32
	hGameDLL = LoadLibrary (GameLibPath);
#else
	hGameDLL = dlopen (GameLibPath, RTLD_LAZY);
#endif

	if (hGameDLL)
		gci.dprintf ("ok\n");
	else 
	{
		gci.dprintf("failed\n");
		if (game_basedir[0] != '\0') 
		{ 
			// failed to load proxy in chain - attempt to load game
			gci.cvar_forceset ("nextproxy", "");
			LoadGameModule ("", game->string);
			return;
		}
		if (game_dir[0] == '\0')  // failed to load default game module
			return;
		LoadGameModule ("", "");   // attempt to load default module
	}
	return;
}


typedef game_export_t *(*GetGameAPI_t) (game_import_t *);

game_export_t *GetGameAPI (game_import_t *gimport)
{
	char CurrentProxy[MAX_OSPATH];
	char NextProxy[MAX_OSPATH];
	char *LoopProxy;
	char *colon;
	int nextcolon;
	GetGameAPI_t GetGameAPI_f;

	memcpy(&gci,gimport,sizeof(game_import_t)); 

	//gci.dprintf("\nGameCam v" GAMECAMVERNUM " " GAMECAMVERSTATUS "\n(C) 1998-99, Avi \"Zung!\" Rozen\ne-mail: zungbang@telefragged.com\n\n");

	gc_version		= gci.cvar ("gc_version", GAMECAMVERSION, CVAR_NOSET);
	gc_flags		= gci.cvar ("gc_flags", "18147", CVAR_ARCHIVE);
	gc_count		= gci.cvar ("gc_count","0", CVAR_NOSET);
	gc_password		= gci.cvar ("gc_password","", 0);
	gc_maxcameras	= gci.cvar ("gc_maxcameras","4", CVAR_ARCHIVE);
	gc_motd 		= gci.cvar ("gc_motd", MOTD_STRING, 0);
	gc_autocam		= gci.cvar ("gc_autocam","0", CVAR_LATCH);
	gc_maxplayers	= gci.cvar ("gc_maxplayers","0", 0);
	gc_ticker		= gci.cvar ("gc_ticker","", CVAR_ARCHIVE);
	gc_maxscores	= gci.cvar ("gc_maxscores","5", CVAR_ARCHIVE);
	gc_teams		= gci.cvar ("gc_teams","", CVAR_ARCHIVE);
	gc_update		= gci.cvar ("gc_update","5", CVAR_ARCHIVE);

	proxy		= gci.cvar("proxy", "", CVAR_SERVERINFO | CVAR_LATCH);
	nextproxy	= gci.cvar("nextproxy", "", CVAR_NOSET);

	basedir		= gci.cvar("basedir", ".", CVAR_NOSET);
	game		= gci.cvar("game", "", CVAR_SERVERINFO | CVAR_LATCH);
	maxclients  = gci.cvar ("maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH);
	dedicated	= gci.cvar ("dedicated", "0", CVAR_NOSET);
	sv_gravity	= gci.cvar ("sv_gravity", "800", 0);
	timelimit	= gci.cvar ("timelimit", "0", CVAR_SERVERINFO);
	dmflags		= gci.cvar ("dmflags", "0", CVAR_SERVERINFO);
	maxspectators = gci.cvar ("maxspectators", "4", CVAR_SERVERINFO);
	deathmatch	= gci.cvar ("deathmatch", "0", CVAR_LATCH);
	spectator_password = gci.cvar ("spectator_password", "", CVAR_USERINFO);
	needpass	= gci.cvar ("needpass", "0", CVAR_SERVERINFO);
	flood_msgs	= gci.cvar ("flood_msgs", "4", 0);
	flood_persecond = gci.cvar ("flood_persecond", "4", 0);
	flood_waitdelay = gci.cvar ("flood_waitdelay", "10", 0);
	//QwazyWabbit//
	gc_set_fov = gci.cvar ("gc_set_fov", "0", 0);
	gc_rename_client = gci.cvar ("gc_rename_client", "0", 0);

	gci.cvar_forceset("gc_version",GAMECAMVERSION);

	CurrentProxy[0]='\0';
	NextProxy[0]='\0';

	if ((proxy->string[0]!='\0') && (nextproxy->string[0]=='\0'))
		gci.cvar_forceset("nextproxy",proxy->string);
	if (nextproxy->string[0]!='\0' && strcmp(nextproxy->string,":")!=0) 
	{
		if ((colon=strchr(nextproxy->string, ':'))==NULL)
			strcpy(CurrentProxy, nextproxy->string);
		else 
		{
			strncpy(CurrentProxy,nextproxy->string,colon - nextproxy->string);
			CurrentProxy[MIN(MAX_OSPATH-1,colon - nextproxy->string)]='\0';
			// prevent infinite loops
			if ((LoopProxy=strstr(colon+1,CurrentProxy))!=NULL) 
			{
				// weird stuff: win95 considers "folder  " and "folder" to be the same
				// while "  folder" and "folder" are different (should be different in
				// both cases!)
				nextcolon=strlen(CurrentProxy);
				while (LoopProxy[nextcolon] && isspace(LoopProxy[nextcolon]))
					nextcolon++;
				if ((LoopProxy[nextcolon]==':' ||
					LoopProxy[nextcolon]=='\0') &&
					(NextProxy[LoopProxy-NextProxy-1]==':')) 
				{
					gci.dprintf("Warning: found a loop in proxy chain!\n");
					colon--; // hack: this will cause next proxy load to fail
				}
			} 
			strcpy(NextProxy,colon+1);
		}
		if (NextProxy[0]!='\0')
			gci.cvar_forceset("nextproxy",NextProxy);
		else
			gci.cvar_forceset("nextproxy",":");
	}
	if ((CurrentProxy[0]!='\0') && strcmp(CurrentProxy,".."))
		LoadGameModule("proxy",CurrentProxy);
	else 
	{
		gci.cvar_forceset("nextproxy","");
		LoadGameModule("",game->string);
	}


	if (hGameDLL == NULL) 
	{
		gci.error("game module not found");
		return NULL;
	}

#ifdef _WIN32
	GetGameAPI_f = (GetGameAPI_t) GetProcAddress (hGameDLL, "GetGameAPI");
#else
	GetGameAPI_f = (GetGameAPI_t) dlsym (hGameDLL, "GetGameAPI");
#endif

	if (GetGameAPI_f == NULL) 
	{
		gci.error("can't get game API");
		return NULL;
	}

	memcpy(&gi,gimport,sizeof(game_import_t));

	// to capture API calls from game module change gi to point
	// to local functions (i.e. gi.dprintf = proxy_dprintf;)

	gi.WriteChar = WriteChar;
	gi.WriteByte = WriteByte;
	gi.WriteShort = WriteShort;
	gi.WriteLong = WriteLong;
	gi.WriteFloat = WriteFloat;
	gi.WriteString = WriteString;
	gi.WritePosition = WritePosition;
	gi.WriteDir = WriteDir;
	gi.WriteAngle = WriteAngle;
	gi.multicast = multicast;
	gi.unicast = unicast;
	gi.cprintf = cprintf;
	gi.centerprintf = centerprintf;
	gi.configstring = configstring;
	gi.setmodel = setmodel;
	gi.modelindex = modelindex;
	//gi.soundindex = soundindex;
	//gi.imageindex = imageindex;
	gi.Pmove = Pmove;
	gi.argc = gc_argc;
	gi.argv = gc_argv;
	gi.args = gc_args;

	gce = GetGameAPI_f(&gi);

	if (strcmp(nextproxy->string,":")==0)
		gci.cvar_forceset("nextproxy","");

	memcpy(&ge,gce,sizeof(game_export_t));

	// to capture API calls from server change gce to point
	// to local functions (i.e. gce->ClientConnect = proxy_ClientConnect;)

	gce->Init = InitGameCam;
	gce->Shutdown = ShutdownGameCam;
	gce->ClientCommand = ClientCommand;
	gce->ServerCommand = ServerCommand;
	gce->ClientConnect = ClientConnect;
	gce->ClientUserinfoChanged = ClientUserinfoChanged;
	gce->ClientBegin = ClientBegin;
	gce->ClientDisconnect = ClientDisconnect;
	gce->ClientThink = ClientThink;
	gce->RunFrame = RunFrame;
	gce->SpawnEntities = SpawnEntities;

	return gce;
}

