// GameCam v1.02 - camera proxy game module for Quake II
// Copyright (c) 1998-99, by Avi Rozen (Zung!)
// e-mail: zungbang@telefragged.com

// gc_connect.c		client connect/disconnect logic

#include "gamecam.h"

vec3_t spawn_origin;

qboolean ClientConnect (edict_t *ent, char *userinfo)
{
	int clientID;
	qboolean client_success = QFALSE;

	if (gc_autocam->value && 
		(((int) gc_flags->value) & GCF_LOCK_SERVER) &&
		match_started)
	{
		Info_SetValueForKey (userinfo, "rejmsg", "match in progress - server is locked");
		return QFALSE;
	}

	clientID = numEdict(ent) - 1;

	clients[clientID].instant_spectator = QFALSE;

	if (!gc_autocam->value && (((int) gc_flags->value) & GCF_SPECTATOR))
	{
		char *value;

		// check for a spectator
		value = Info_ValueForKey (userinfo, "spectator");
		if (*value && strcmp(value, "0")) 
		{
			// password incorrect
			if (*spectator_password->string && 
				strcmp(spectator_password->string, "none") && 
				strcmp(spectator_password->string, value)) 
			{
				Info_SetValueForKey(userinfo, "rejmsg", "spectator password required or incorrect");
				return QFALSE;
			}

			// too many cameras
			if (cam_count >= maxspectators->value) 
			{
				Info_SetValueForKey(userinfo, "rejmsg", "Server spectator limit is full");
				return QFALSE;
			}

			// don't connect to game
			clients[clientID].instant_spectator = QTRUE;
		}
	}
	clients[clientID].spectator = clients[clientID].instant_spectator;
	if (clients[clientID].spectator)
	{
		char spectator[MAX_INFO_STRING];

		// we must connect to the game, or else stuff won't be initialized
		// inside the real game dll
		strcpy (spectator, Info_ValueForKey (userinfo, "spectator"));
		Info_SetValueForKey (userinfo, "spectator", "0");
		client_success = ge.ClientConnect (ent, userinfo);
		Info_SetValueForKey (userinfo, "spectator", spectator);
	}
	else
		client_success = ge.ClientConnect (ent, userinfo);
	clients[clientID].inuse = client_success;
	if (client_success) 
	{
		clients[clientID].id =		   ((((int) gc_flags->value) & GCF_DEFAULT_ID)    != 0);
		clients[clientID].ticker =	   ((((int) gc_flags->value) & GCF_DEFAULT_TICKER)!= 0);
		clients[clientID].fixed =	   ((((int) gc_flags->value) & GCF_DEFAULT_FIXED) != 0);
		clients[clientID].chase_auto = ((((int) gc_flags->value) & GCF_DEFAULT_AUTO)  != 0);
		clients[clientID].demo =	   ((((int) gc_flags->value) & GCF_DEFAULT_DEMO)  != 0);
		clients[clientID].limbo_time = 0;
		clients[clientID].target = -1;
		clients[clientID].welcome = QFALSE;
		clients[clientID].begin = QFALSE;
		strcpy (clients[clientID].userinfo, userinfo);

	}
	return client_success;
}


void InstantSpectator (edict_t *ent)
{
	int clientID;
	char s_cam_count[5];

	clientID = numEdict (ent) - 1;

	ge.ClientDisconnect (ent);
	// misc setup
	clients[clientID].spectator = QTRUE;
	clients[clientID].instant_spectator = QFALSE;
	ent->client->ps.pmove.pm_type = PM_SPECTATOR;
	// reset fov
	if (gc_set_fov->value)
		set_fov (ent, 90, QFALSE);
	// setup action camera
	camera_action_setup (clientID);
	cam_count++;
	sprintf (s_cam_count,"%d", cam_count);
	gci.cvar_forceset ("gc_count", s_cam_count);
	if (cam_count == 1) // first spectator, so setup ticker tape
		ticker_init ();
}


void ClientBegin (edict_t *ent)
{
	int clientID, i;

	clientID = numEdict(ent) - 1;
	clients[clientID].begin = QTRUE;
	clients[clientID].ticker_frame = 0; // force update of ticker
	// reset flood protection
	clients[clientID].flood_locktill = 0;
	clients[clientID].flood_whenhead = 0;
	memset (clients[clientID].flood_when, 0, 10 * sizeof (float));
	// stop/restart demo
	if (clients[clientID].demo)
		demoON (ent);
	// handle instant spectators
	if (gc_autocam->value)
	{
		clients[clientID].motddelay = 10;
		clients[clientID].mode = CAMERA_ACTION;
		if (!clients[clientID].spectator)
			InstantSpectator (ent);
		gci.centerprintf (ent, motd (gc_motd->string));
	}
	else if (clients[clientID].instant_spectator)
	{
		InstantSpectator (ent);
		gci.centerprintf (ent, motd ("type '\\!camera\\!' to see the menu"));
	}
	// put players/spectators in game
	if (clients[clientID].spectator) 
	{ 
		// new map
		gci.bprintf (PRINT_HIGH, "%s entered the game as a spectator\n", 
			Info_ValueForKey (clients[clientID].userinfo, "name"));
		VectorCopy(spawn_origin, ent->s.origin);
		for (i=0 ; i<3 ; i++) 
			ent->client->ps.pmove.origin[i] = (short) spawn_origin[i]*8;

		switch (clients[clientID].mode) 
		{
		case CAMERA_FREE:
			camera_free_begin (clientID);
			break;
		case CAMERA_CHASE:
			camera_chase_begin (clientID);
			break;
		case CAMERA_ACTION:
			camera_action_begin (clientID);
		}
		return;
	} 
	else 
	{
		camera_free_add_target (clientID);
		camera_chase_add_target (clientID);
		camera_action_add_target (clientID);
		ge.ClientBegin (ent);
		if (((int) gc_flags->value & GCF_WELCOME) && !clients[clientID].welcome) 
		{
			clients[clientID].welcome = QTRUE;
			// stufftext looks better (cprintf at first frame shown only on console)
			gci.WriteByte (svc_stufftext);
			gci.WriteString ("echo [GameCam]: type 'camera ?' for help\n");
			gci.unicast (ent, QTRUE);
			//gci.cprintf (ent, PRINT_HIGH, "[GameCam]: type 'camera ?' for help\n");
		}
	}
}


void ClientDisconnect (edict_t *ent)
{
	int clientID;
	char s_cam_count[5];

	clientID = numEdict(ent) - 1;
	clients[clientID].inuse = QFALSE;
	clients[clientID].reset_layouts = QFALSE;
	clients[clientID].welcome = QFALSE;
	if (!clients[clientID].spectator) 
	{
		// make sure spectators are not following a disconnected client
		camera_free_remove_target (clientID);
		camera_chase_remove_target (clientID);
		camera_action_remove_target (clientID);
		ge.ClientDisconnect(ent); 
	} 
	else 
	{ 
		// spectators are already disconnected from the game, so just print a message and wrapup
		gci.bprintf (PRINT_HIGH, "%s disconnected\n", 
			Info_ValueForKey (clients[clientID].userinfo, "name"));
		switch (clients[clientID].mode) 
		{
		case CAMERA_FREE:
			camera_free_wrapup (clientID);
			break;
		case CAMERA_CHASE:
			camera_chase_wrapup (clientID);
			break;
		case CAMERA_ACTION:
			camera_action_wrapup (clientID);
		}
		cam_count--;
		sprintf (s_cam_count,"%d", cam_count);
		gci.cvar_forceset ("gc_count", s_cam_count);
	}
	clients[clientID].begin = QFALSE;
	clients[clientID].spectator = QFALSE;
	clients[clientID].limbo_time = 0;
	clients[clientID].target = -1;
}


void ClientUserinfoChanged (edict_t *ent, char *userinfo)
{
	int clientID;

	clientID = numEdict(ent) - 1;

	if (gc_autocam->value == 0 && 
		(((int) gc_flags->value) & GCF_SPECTATOR) && 
		clients[clientID].begin &&
		clients[clientID].limbo_time == 0)
	{
		char spectator[MAX_INFO_VALUE];

		strcpy (spectator, Info_ValueForKey (userinfo, "spectator"));
		if (strcmp (spectator, Info_ValueForKey (clients[clientID].userinfo, "spectator")))
		{
			if (*spectator && strcmp (spectator, "0") != 0 && !clients[clientID].spectator)
			{
				strcpy (clients[clientID].userinfo, userinfo);
				SpectatorBegin (ent, spectator, QTRUE);
				return;
			}
			else if ((*spectator == '\0' || strcmp (spectator, "0") == 0) &&
				clients[clientID].spectator)
			{
				strcpy (clients[clientID].userinfo, userinfo);
				SpectatorEnd (ent, "");
				return;
			}
		}
	}

	if (!clients[clientID].spectator) 
	{
		ge.ClientUserinfoChanged (ent, userinfo);
		strcpy (clients[clientID].userinfo, userinfo);
	}
	else 
	{
		char cam_name[MAX_INFO_VALUE];

		strcpy (clients[clientID].userinfo, userinfo);
		// fov for cameras
		ent->client->ps.fov = (float) atof (Info_ValueForKey (userinfo, "fov"));
		if (ent->client->ps.fov < 1)
			ent->client->ps.fov = 90;
		else if (ent->client->ps.fov > 160)
			ent->client->ps.fov = 160;
		// name for cameras
		if (gc_rename_client->value)
			Com_sprintf (cam_name, MAX_INFO_VALUE, "[CAMERA]%s",
			Info_ValueForKey (clients[clientID].userinfo, "name"));
		else
			Com_sprintf (cam_name, MAX_INFO_VALUE, "%s",
			Info_ValueForKey (clients[clientID].userinfo, "name"));

		Info_SetValueForKey (userinfo, "name", cam_name);
	}
}


void FindSpawnPoint (char *entstring)
{
	char *classname;
	char *bracket;
	char *origin;
	float x, y, z;

	spawn_origin[0] = 0;
	spawn_origin[1] = 0;
	spawn_origin[2] = 0;

	classname = strstr (entstring, "info_player_start");
	if (!classname)
		classname = strstr (entstring, "info_player_deathmatch");
	if (!classname)
		return;
	bracket = classname;
	while (bracket >= entstring && *bracket != '{') 
		bracket--;
	if (bracket < entstring) 
		return;
	origin = strstr (bracket, "origin");
	if (!origin)
		return;
	while (*origin && !(isdigit(*origin) || (*origin == '-'))) 
		origin++;
	if (!(*origin))
		return;
	if (sscanf (origin, "%f %f %f", &x, &y, &z) != 3)
		return;
	spawn_origin[0] = x;
	spawn_origin[1] = y;
	spawn_origin[2] = z;
}


void FindModels (char *entstring)
{
	char *model;
	int modelnum;

	model = entstring;
	while ((model = strstr (model, "\"model\"")) != NULL)
	{
		if (sscanf (model, "\"model\" \"*%d\"", &modelnum) == 1)
			sprintf (ConfigStrings[CS_MODELS + modelnum + 1], "*%d", modelnum);
		model += 7;
	}
}


void SpawnEntities (char *mapname, char *entstring, char *spawnpoint)
{
	int i, clientID;

	// new map: all clients need to enter the game first, so we
	// remove all camera targets, and set begin flag to QFALSE
	for (clientID = 0; clientID < maxclients->value; clientID++) 
	{
		if (clients[clientID].inuse) 
		{
			clients[clientID].begin = QFALSE;
			clients[clientID].camera =  NULL;
			if (!clients[clientID].spectator) 
			{
				camera_free_remove_target (clientID);
				camera_chase_remove_target (clientID);
				camera_action_remove_target (clientID);
			}
		}
	}
	strcpy (current_map, mapname);
	camera_fixed_free ();
	camera_fixed_load (mapname);
	FindSpawnPoint (entstring);
	// new map: clear models, sounds and images
	for (i = CS_MODELS + 1; i < CS_LIGHTS; i++)
		ConfigStrings[i][0] = '\0';
	FindModels (entstring);
	intermission = QFALSE;
	match_offsetframes = 0;
	match_updateframes = 0;
	match_started = QFALSE;
	match_10sec = QFALSE;
	if (cam_count)
		ticker_init ();
	ge.SpawnEntities (mapname, entstring, spawnpoint);
}

