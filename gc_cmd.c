// GameCam v1.02 - camera proxy game module for Quake II
// Copyright (c) 1998-99, by Avi Rozen (Zung!)
// e-mail: zungbang@telefragged.com

// gc_cmd.c		gamecam console commands

#include "gamecam.h"

char command_args[MAX_STRING_CHARS];
char command_argv[MAX_STRING_TOKENS][MAX_TOKEN_CHARS];
int  command_argc;
qboolean camera_command_flag;

edict_t* wait_camera = NULL;
edict_t* wait_inven = NULL;
edict_t* wait_score = NULL;
edict_t* wait_help = NULL;
edict_t* wait_cprintf = NULL;

char captured_print_message[MAX_STRING_CHARS];


void UpdateScore(int clientID)
{
	int targetID, count = 0;
	edict_t* ent;
	edict_t* target;

	ent = Edict(clientID + 1);

	if (clients[clientID].mode == CAMERA_CHASE)
	{
		target = Edict(clients[clientID].target + 1);
		if (!(target->client->ps.stats[STAT_LAYOUTS]))
		{
			wait_camera = ent;
			wait_score = target;
			GameCommand(target, "score");
			GameCommand(target, "putaway");
			ent->client->ps.stats[STAT_LAYOUTS] |= 1;
		}
	}
	if (clients[clientID].mode != CAMERA_CHASE || wait_camera == NULL)
	{
		for (targetID = 0; targetID < maxclients->value; targetID++)
		{
			target = Edict(targetID + 1);
			if ((clients[targetID].inuse &&
				clients[targetID].begin &&
				!clients[targetID].spectator) ||
				(!((int)gc_flags->value & GCF_AVOID_BOTS) &&
					target->inuse &&
					target->s.modelindex != 0))
			{
				count++;
				if (!(target->client->ps.stats[STAT_LAYOUTS]))
				{
					wait_camera = ent;
					wait_score = target;
					GameCommand(target, "score");
					memcpy(ent->client->ps.stats, target->client->ps.stats, MAX_STATS * sizeof(short));
					GameCommand(target, "putaway");
					ent->client->ps.stats[STAT_LAYOUTS] |= 1;
					break;
				}
			}
		}
		if (wait_camera == NULL)
		{
			if (count == 0 && clients[clientID].mode != CAMERA_CHASE)
				ent->client->ps.stats[STAT_LAYOUTS] &= ~1;
			gci.centerprintf(ent, "waiting for scoreboard\nto be available");
		}
	}
	wait_camera = NULL;
	wait_score = NULL;
}



void SpectatorBegin(edict_t* ent, char* password, qboolean validate)
{
	int clientID;
	char s_cam_count[32];

	clientID = numEdict(ent) - 1;

	if (isSpectator(clientID)) // do nothing if client is already a spectator
		return;

	if (intermission)
		return;

	if (gc_autocam->value == 0)
	{
		// camera password
		if (validate)
		{
			if (((int)gc_flags->value) & GCF_SPECTATOR)
			{
				if (*spectator_password->string &&
					strcmp(spectator_password->string, "none") &&
					strcmp(spectator_password->string, password))
				{
					gci.cprintf(ent, PRINT_HIGH, "spectator password incorrect\n");
					gci.WriteByte(svc_stufftext);
					gci.WriteString("spectator 0\n");
					gci.unicast(ent, true);
					return;
				}
			}
			else
			{
				if (strcmp(gc_password->string, password) != 0)
				{
					gci.cprintf(ent, PRINT_HIGH, "password missing or incorrect\n");
					return;
				}
			}
		}

		// too many cameras
		if (((int)gc_flags->value) & GCF_SPECTATOR)
		{
			if (cam_count >= maxspectators->value)
			{
				gci.cprintf(ent, PRINT_HIGH, "no more cameras allowed\n");
				gci.WriteByte(svc_stufftext);
				gci.WriteString("spectator 0\n");
				gci.unicast(ent, true);
				return;
			}
		}
		else
		{
			if (gc_maxcameras->value != 0 && gc_maxcameras->value <= cam_count)
			{
				gci.cprintf(ent, PRINT_HIGH, "no more cameras allowed\n");
				return;
			}
		}

		if (((int)gc_flags->value) & GCF_SPECTATOR)
		{
			char* spectator = Info_ValueForKey(clients[clientID].userinfo, "spectator");

			if (*spectator == '\0' || strcmp(spectator, "0") == 0)
			{
				gci.WriteByte(svc_stufftext);
				gci.WriteString("spectator 1\n");
				gci.unicast(ent, true);
			}
		}
	}

	// make sure other spectators are not following the new spectator
	camera_free_remove_target(clientID);
	camera_chase_remove_target(clientID);
	camera_action_remove_target(clientID);
	// the only time when we don't validate the password
	// is when the client is already a spectator
	// and he can't return to the game
	// so we don't disconnect him if we don't need to validate
	if (validate)
		ge.ClientDisconnect(ent);
	// misc setup
	clients[clientID].spectator = true;
	ent->client->ps.pmove.pm_type = PM_SPECTATOR;
	// reset fov
	if (gc_set_fov->value)
		set_fov(ent, 90, false);
	// setup camera mode
	switch (clients[clientID].mode)
	{
	case CAMERA_FREE:
		camera_free_setup(clientID);
		break;
	case CAMERA_CHASE:
		camera_chase_setup(clientID);
		break;
	case CAMERA_ACTION:
		camera_action_setup(clientID);
	}
	cam_count++;
	sprintf(s_cam_count, "%d", cam_count);
	gci.cvar_forceset("gc_count", s_cam_count);
	if (cam_count == 1) // first spectator, so setup ticker tape
		ticker_init();
	clients[clientID].ticker_frame = 0;
	gci.bprintf(PRINT_HIGH, "%s entered the game as a spectator\n",
		Info_ValueForKey(clients[clientID].userinfo, "name"));
	gci.centerprintf(ent, motd("type '\\!camera\\!' to see the menu"));
}


void SpectatorEnd(edict_t* ent, char* password)
{
	int clientID;

	clientID = numEdict(ent) - 1;

	if (!isSpectator(clientID)) // do nothing if client is not a spectator already
		return;

	if (intermission)
		return;

	if (gc_autocam->value)
	{
		// match password
		if (strcmp(gc_password->string, password))
		{
			gci.cprintf(ent, PRINT_HIGH, "password missing or incorrect\n");
			return;
		}

		// too many players
		if (gc_maxplayers->value && numPlayers() >= gc_maxplayers->value)
		{
			gci.cprintf(ent, PRINT_HIGH, "no more players allowed\n");
			return;
		}
		match_started = true;
		match_startframe = framenum;
	}

	if (((int)gc_flags->value) & GCF_SPECTATOR)
	{
		char* spectator = Info_ValueForKey(clients[clientID].userinfo, "spectator");

		if (*spectator == '\0' || strcmp(spectator, "0"))
		{
			gci.WriteByte(svc_stufftext);
			gci.WriteString("spectator 0\n");
			gci.unicast(ent, true);
		}
	}

	// put spectator in limbo (to prevent overflow)
	// 1st frame - no operation (to allow scoreboard update)
	// 2nd frame - exit camera (and reset hud)
	// 3rd frame - return to game
	clients[clientID].limbo_time = 3;
}


void CameraOff(edict_t* ent)
{
	int clientID;

	clientID = numEdict(ent) - 1;

	switch (clients[clientID].mode)
	{
	case CAMERA_FREE:
		clients[clientID].reset_layouts = true;
		camera_free_wrapup(clientID);
		break;
	case CAMERA_CHASE:
		camera_chase_wrapup(clientID);
		break;
	case CAMERA_ACTION:
		clients[clientID].reset_layouts = true;
		camera_action_wrapup(clientID);
	}
}


void ReturnToGame(edict_t* ent)
{
	int clientID;
	char s_cam_count[32];
	qboolean success;

	clientID = numEdict(ent) - 1;

	clients[clientID].spectator = false;
	cam_count--;
	sprintf(s_cam_count, "%d", cam_count);
	gci.cvar_forceset("gc_count", s_cam_count);
	// reset fov
	if (gc_set_fov->value)
		set_fov(ent, 90, false);
	// re-connect client to game
	Info_SetValueForKey(clients[clientID].userinfo, "ip", clients[clientID].ip);
	success = ge.ClientConnect(ent, clients[clientID].userinfo);
	if (success)
	{
		int i;

		ge.ClientBegin(ent);
		for (i = 0; i < 3; i++)
			ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(ent->client->ps.viewangles[i] - clients[clientID].cmd_angles[i]);
		// make him kill himself
		// (to discourage spectator mode toggle)
		if ((int)gc_flags->value & GCF_SUICIDE)
		{
			gci.WriteByte(svc_stufftext);
			gci.WriteString("kill\n");
			gci.unicast(ent, true);
		}
	}
	else
	{
		gci.cprintf(ent, PRINT_HIGH, "can't return to game - \"%s\"\n", Info_ValueForKey(clients[clientID].userinfo, "rejmsg"));
		SpectatorBegin(ent, "", false);
	}
}


void demoOFF(edict_t* ent)
{
	clients[numEdict(ent) - 1].demo = false;
	gci.WriteByte(svc_stufftext);
	gci.WriteString("stop\n");
	gci.unicast(ent, true);
}


void demoON(edict_t* ent)
{
	char demo_cmd[MAX_STRING_CHARS];
	struct tm* today;
	time_t sysclock;

	time(&sysclock);
	today = localtime(&sysclock);
	clients[numEdict(ent) - 1].demo = true;
	sprintf(demo_cmd, "stop; record %s-%.2d%.2d%.2d-%.2d%.2d\n",
		current_map,
		today->tm_mday, today->tm_mon + 1, today->tm_year % 100,
		today->tm_hour, today->tm_min);
	gci.WriteByte(svc_stufftext);
	gci.WriteString(demo_cmd);
	gci.unicast(ent, true);
}


void ToggleLayouts(edict_t* ent)
{
	int clientID;

	clientID = numEdict(ent) - 1;

	clients[clientID].help = false;
	clients[clientID].score = false;
	clients[clientID].inven = false;
	clients[clientID].menu = false;
	clients[clientID].layouts = !clients[clientID].layouts;

	if (clients[clientID].layouts)
	{
		memset(clients[clientID].items, 0, MAX_ITEMS * sizeof(short));
		clients[clientID].layout[0] = '\0';
	}
	else
		clients[clientID].update_chase = true;

	if ((int)gc_flags->value & GCF_VERBOSE)
		gci.cprintf(ent, PRINT_HIGH, "client layouts display is %s\n",
			(clients[clientID].layouts) ? "ON" : "OFF");
}


void ToggleInven(edict_t* ent)
{
	int clientID;
	edict_t* target;

	clientID = numEdict(ent) - 1;

	clients[clientID].score = false;
	clients[clientID].help = false;
	clients[clientID].menu = false;
	clients[clientID].layouts = false;

	clients[clientID].inven = !clients[clientID].inven;

	if (clients[clientID].inven)
	{
		target = Edict(clients[clientID].target + 1);
		wait_camera = ent;
		wait_inven = target;
		if (!(target->client->ps.stats[STAT_LAYOUTS] & 1))
		{
			GameCommand(target, "inven");
			GameCommand(target, "inven");
			clients[clientID].item = target->client->ps.stats[STAT_SELECTED_ITEM];
		}
		else
		{
			gci.centerprintf(ent, "client HUD is in use\ntry again later");
			clients[clientID].inven = false;
		}
		wait_camera = NULL;
		wait_inven = NULL;
	}
	if (!clients[clientID].inven)
		clients[clientID].update_chase = true;
}


void Cmd_Camera_f(edict_t* ent)
{
	char* mode;
	int	oldmode;
	int clientID;

	if (gci.argc() > 2 && gci.argc() != 3)
	{
		gci.cprintf(ent, PRINT_HIGH, motd("type '\\!camera ?\\!' for help"));
		return;
	}

	clientID = numEdict(ent) - 1;
	oldmode = clients[clientID].mode;

	// use 'camera' to show/hide menu when in spectator mode
	if (gci.argc() == 1)
	{
		if (clients[clientID].menu)
			hideMenu(clientID);
		else
			showMenu(clientID);
		return;
	}

	// disable most commands while showing menu
	if (clients[clientID].menu)
	{
		if (Q_strcasecmp(gci.argv(1), "menu") &&
			Q_strcasecmp(gci.argv(1), "off") &&
			Q_strcasecmp(gci.argv(1), "?"))
		{
			gci.cprintf(ent, PRINT_HIGH, "please exit camera menu first\n");
			return;
		}
	}


	// execute camera command
	if (gci.argc() == 2)
	{
		mode = gci.argv(1);
		// process camera options
		if (Q_strcasecmp(mode, "?") == 0)
		{
			if (gc_autocam->value)
				gci.cprintf(ent, PRINT_HIGH, motd(
					"\\!GameCam v" GAMECAMVERNUM " " GAMECAMVERSTATUS "\\!\n(C) 1998-99, Avi \"\\!Zung!\\!\" Rozen\ne-mail: zungbang@telefragged.com\n"
					"http://www.telefragged.com/\\!zungbang\\!\n\n"
					"USAGE:\n"
					"\\!camera\\!      - show camera menu\n"
					"\\!camera ?\\!    - show this message\n"
					"\\!camera on\\!   - start camera\n"
					"\\!camera off [<password>]\\! - end camera\n"
					"\\!camera demo\\! - auto demo recording\n"
					"\\!camera chase|action\\! - select mode\n"));
			else
				gci.cprintf(ent, PRINT_HIGH, motd(
					"\\!GameCam v" GAMECAMVERNUM " " GAMECAMVERSTATUS "\\!\n(C) 1998-99, Avi \"\\!Zung!\\!\" Rozen\ne-mail: zungbang@telefragged.com\n"
					"http://www.telefragged.com/\\!zungbang\\!\n\n"
					"USAGE:\n"
					"\\!camera\\!      - show camera menu\n"
					"\\!camera ?\\!    - show this message\n"
					"\\!camera on [<password>]\\! - start camera\n"
					"\\!camera off\\!  - end camera\n"
					"\\!camera demo\\! - auto demo recording\n"
					"\\!camera chase|action\\! - select mode\n"));
			return;
		}
		else if (Q_strcasecmp(mode, "on") == 0)
		{
			SpectatorBegin(ent, "", true);
			return;
		}
		else if (Q_strcasecmp(mode, "off") == 0)
		{
			SpectatorEnd(ent, "");
			return;
		}
		else if (Q_strcasecmp(mode, "max_xy") == 0)
		{
			gci.cprintf(ent, PRINT_HIGH, "Max X/Y delta is %f\n", clients[clientID].fXYLag);
			return;
		}
		else if (Q_strcasecmp(mode, "max_z") == 0)
		{
			gci.cprintf(ent, PRINT_HIGH, "Max Z delta is %f\n", clients[clientID].fZLag);
			return;
		}
		else if (Q_strcasecmp(mode, "max_angle") == 0)
		{
			gci.cprintf(ent, PRINT_HIGH, "Max Yaw Angle delta is %f\n", clients[clientID].fAngleLag);
			return;
		}
		else if (Q_strcasecmp(mode, "id") == 0)
		{
			if (clients[clientID].id)
			{
				gci.cprintf(ent, PRINT_HIGH, "camera player id turned OFF\n");
				clients[clientID].id = false;
			}
			else
			{
				gci.cprintf(ent, PRINT_HIGH, "camera player id turned ON\n");
				clients[clientID].id = true;
			}
			return;
		}
		else if (Q_strcasecmp(mode, "ticker") == 0)
		{
			if (clients[clientID].ticker)
			{
				gci.cprintf(ent, PRINT_HIGH, "ticker tape turned OFF\n");
				clients[clientID].ticker = false;
			}
			else
			{
				gci.cprintf(ent, PRINT_HIGH, "ticker tape turned ON\n");
				clients[clientID].ticker = true;
				clients[clientID].ticker_frame = 0; // force update
			}
			return;
		}
		else if (Q_strcasecmp(mode, "menu") == 0)
		{
			if (clients[clientID].menu)
				hideMenu(clientID);
			else
				showMenu(clientID);
			return;
		}
		else if (Q_strcasecmp(mode, "free") == 0)
		{
			if ((int)gc_flags->value & GCF_ALLOW_FREE)
				clients[clientID].mode = CAMERA_FREE;
			else
				gci.cprintf(ent, PRINT_HIGH, "FREE mode not allowed\n");
		}
		else if (Q_strcasecmp(mode, "chase") == 0)
		{
			if ((int)gc_flags->value & GCF_ALLOW_CHASE)
				clients[clientID].mode = CAMERA_CHASE;
			else
				gci.cprintf(ent, PRINT_HIGH, "CHASE mode not allowed\n");
		}
		else if (Q_strcasecmp(mode, "action") == 0)
		{
			clients[clientID].mode = CAMERA_ACTION;
		}
		else if (Q_strcasecmp(mode, "select") == 0)
		{
			if (clients[clientID].select == 0)
				gci.cprintf(ent, PRINT_HIGH, "camera target is AUTO\n");
			else
				gci.cprintf(ent, PRINT_HIGH, "camera target is #%d\n", clients[clientID].select - 1);
			return;
		}
		else if (Q_strcasecmp(mode, "layout") == 0)
		{
			if (clients[clientID].spectator && clients[clientID].mode == CAMERA_CHASE)
				ToggleLayouts(ent);
			else
				gci.cprintf(ent, PRINT_HIGH, "must be chase camera to view layouts\n");
			return;
		}
		else if (Q_strcasecmp(mode, "inven") == 0)
		{
			if (clients[clientID].spectator && clients[clientID].mode == CAMERA_CHASE)
				ToggleInven(ent);
			else
				gci.cprintf(ent, PRINT_HIGH, "must be chase camera to view inventory\n");
			return;
		}
		else if (Q_strcasecmp(mode, "fixed") == 0)
		{
			if (clients[clientID].fixed)
			{
				gci.cprintf(ent, PRINT_HIGH, "fixed cameras turned OFF\n");
				clients[clientID].fixed = false;
				if (gc_set_fov->value)
					set_fov(ent, 90, false);
			}
			else
			{
				gci.cprintf(ent, PRINT_HIGH, "fixed cameras turned ON\n");
				clients[clientID].fixed = true;
				clients[clientID].camera = NULL;
				clients[clientID].fixed_switch_time = -1.0F;
			}
			return;
		}
		else if (Q_strcasecmp(mode, "list") == 0)
		{
			if (dedicated->value != 0 || clientID != 0 ||
				!(clients[clientID].spectator && clients[clientID].mode == CAMERA_FREE))
				gci.cprintf(ent, PRINT_HIGH, "can't list cameras\n");
			else
				camera_fixed_list(ent);
			return;
		}
		else if (Q_strcasecmp(mode, "add") == 0)
		{
			if (dedicated->value != 0 || clientID != 0 ||
				!(clients[clientID].spectator && clients[clientID].mode == CAMERA_FREE))
				gci.cprintf(ent, PRINT_HIGH, "can't add cameras\n");
			else
			{
				clients[clientID].camera = camera_fixed_add(ent, "unnamed camera");
				ticker_update_camera(ent, clients[clientID].camera);
			}
			return;
		}
		else if (Q_strcasecmp(mode, "update") == 0)
		{
			if (dedicated->value != 0 || clientID != 0 ||
				!(clients[clientID].spectator && clients[clientID].mode == CAMERA_FREE))
				gci.cprintf(ent, PRINT_HIGH, "can't update cameras\n");
			else
			{
				if (clients[clientID].camera)
					camera_fixed_update(clients[clientID].camera, ent, NULL);
				else
					clients[clientID].camera = camera_fixed_add(ent, "unnamed camera");
				ticker_update_camera(ent, clients[clientID].camera);
			}
			return;
		}
		else if (Q_strcasecmp(mode, "remove") == 0)
		{
			if (dedicated->value != 0 || clientID != 0 ||
				!(clients[clientID].spectator && clients[clientID].mode == CAMERA_FREE))
				gci.cprintf(ent, PRINT_HIGH, "can't remove cameras\n");
			else
				if (clients[clientID].camera)
				{
					camera_fixed_remove(clients[clientID].camera);
					clients[clientID].camera = NULL;
					ticker_update_camera(ent, NULL);
				}
				else
					gci.cprintf(ent, PRINT_HIGH, "no camera selected\n");
			return;
		}
		else if (Q_strcasecmp(mode, "save") == 0)
		{
			if (dedicated->value != 0 || clientID != 0 ||
				!(clients[clientID].spectator && clients[clientID].mode == CAMERA_FREE))
				gci.cprintf(ent, PRINT_HIGH, "can't save cameras\n");
			else
				if (cameras)
					camera_fixed_save(current_map);
				else
					gci.cprintf(ent, PRINT_HIGH, "cameras not defined\n");
			return;
		}
		else if (Q_strcasecmp(mode, "next") == 0)
		{
			if (cameras)
			{
				if (clients[clientID].spectator && clients[clientID].mode == CAMERA_FREE)
				{
					clients[clientID].camera = (clients[clientID].camera) ?
						clients[clientID].camera->next : cameras;
					camera_fixed_select(clients[clientID].camera, ent, NULL);
					ticker_update_camera(ent, clients[clientID].camera);
				}
			}
			else
				gci.cprintf(ent, PRINT_HIGH, "cameras not defined\n");
			return;
		}
		else if (Q_strcasecmp(mode, "prev") == 0)
		{
			if (cameras)
			{
				if (clients[clientID].spectator && clients[clientID].mode == CAMERA_FREE)
				{
					clients[clientID].camera = (clients[clientID].camera) ?
						clients[clientID].camera->prev : cameras;
					camera_fixed_select(clients[clientID].camera, ent, NULL);
					ticker_update_camera(ent, clients[clientID].camera);
				}
			}
			else
				gci.cprintf(ent, PRINT_HIGH, "cameras not defined\n");
			return;
		}
		else if (Q_strcasecmp(mode, "none") == 0)
		{
			if (clients[clientID].spectator && clients[clientID].mode == CAMERA_FREE)
			{
				clients[clientID].camera = NULL;
				ticker_update_camera(ent, NULL);
				if (gc_set_fov->value)
					set_fov(ent, 90, false);
			}
			return;
		}
		else if (Q_strcasecmp(mode, "demo") == 0)
		{
			if (clients[clientID].demo)
			{
				gci.cprintf(ent, PRINT_HIGH, "auto demo recording is OFF\n");
				demoOFF(ent);
			}
			else
			{
				gci.cprintf(ent, PRINT_HIGH, "auto demo recording is ON\n");
				demoON(ent);
			}
			return;
		}
		else
		{
			gci.cprintf(ent, PRINT_HIGH, "no such camera option - \"%s\"\n", mode);
			return;
		}
	}

	if (gci.argc() == 3)
	{
		float fTemp;
		if (Q_strcasecmp(gci.argv(1), "on") == 0)
		{
			SpectatorBegin(ent, gci.argv(2), true);
			return;
		}
		else if (Q_strcasecmp(gci.argv(1), "off") == 0)
		{
			SpectatorEnd(ent, gci.argv(2));
			return;
		}
		else if (Q_strcasecmp(gci.argv(1), "max_xy") == 0)
		{
			if ((fTemp = (float)atof(gci.argv(2))) >= 1)
			{
				clients[clientID].fXYLag = fTemp;
				gci.cprintf(ent, PRINT_HIGH, "Max X/Y delta set to %f\n", clients[clientID].fXYLag);
				return;
			}
		}
		else if (Q_strcasecmp(gci.argv(1), "max_z") == 0)
		{
			if ((fTemp = (float)atof(gci.argv(2))) >= 1)
			{
				clients[clientID].fZLag = fTemp;
				gci.cprintf(ent, PRINT_HIGH, "Max Z delta set to %f\n", clients[clientID].fZLag);
				return;
			}
		}
		else if (Q_strcasecmp(gci.argv(1), "max_angle") == 0)
		{
			if ((fTemp = (float)atof(gci.argv(2))) >= 1)
			{
				clients[clientID].fAngleLag = fTemp;
				gci.cprintf(ent, PRINT_HIGH, "Max Yaw Angle delta set to %f\n", clients[clientID].fAngleLag);
				return;
			}
		}
		else if (Q_strcasecmp(gci.argv(1), "chase") == 0)
		{
			if (Q_strcasecmp(gci.argv(2), "auto") == 0)
			{
				clients[clientID].chase_auto = !clients[clientID].chase_auto;
				clients[clientID].update_chase = true;
				if (clients[clientID].chase_auto)
					gci.cprintf(ent, PRINT_HIGH, "AUTO CHASE camera is ON\n");
				else
					gci.cprintf(ent, PRINT_HIGH, "AUTO CHASE camera is OFF\n");
			}
			else
				gci.cprintf(ent, PRINT_HIGH, "no such CHASE camera sub-mode - \"%s\"\n", gci.argv(2));
			return;
		}
		else if (Q_strcasecmp(gci.argv(1), "action") == 0)
		{
			if (Q_strcasecmp(gci.argv(2), "normal") == 0)
			{
				clients[clientID].iMode = CAM_NORMAL_MODE;
				clients[clientID].last_move_time = 0; // rethink
				gci.cprintf(ent, PRINT_HIGH, "ACTION camera mode is NORMAL\n");
				return;
			}
			else if (Q_strcasecmp(gci.argv(2), "follow") == 0)
			{
				clients[clientID].iMode = CAM_FOLLOW_MODE;
				clients[clientID].last_move_time = 0; // rethink
				gci.cprintf(ent, PRINT_HIGH, "ACTION camera mode is FOLLOW\n");
				return;
			}
			else
			{
				gci.cprintf(ent, PRINT_HIGH, "no such ACTION camera sub-mode - \"%s\"\n", gci.argv(2));
				return;
			}
		}
		else if (Q_strcasecmp(gci.argv(1), "select") == 0)
		{
			int target;

			if (Q_strcasecmp(gci.argv(2), "auto") == 0)
			{
				clients[clientID].select = 0;
				gci.cprintf(ent, PRINT_HIGH, "camera target is AUTO\n");
				return;
			}
			else if (sscanf(gci.argv(2), " %d ", &target) == 1 && target != clientID && target >= 0 && target < maxclients->value)
			{
				clients[clientID].select = target + 1;
				clients[clientID].last_move_time = 0;
				clients[clientID].last_switch_time = 0;
				gci.cprintf(ent, PRINT_HIGH, "camera target is #%d\n", target);
				if (clients[clientID].spectator && clients[clientID].mode == CAMERA_CHASE)
				{
					if (clients[target].inuse &&
						clients[target].begin &&
						!clients[target].spectator)
					{
						clients[clientID].target = target;
						clients[clientID].help = false;
						clients[clientID].score = false;
						clients[clientID].inven = false;
						clients[clientID].layouts = false;
						clients[clientID].update_chase = true;
						clients[clientID].chase_distance = 0.0F;
						clients[clientID].chase_height = 0.0F;
						clients[clientID].chase_angle = 0.0F;
						clients[clientID].chase_yaw = 0.0F;
						clients[clientID].chase_pitch = 0.0F;
					}
				}
				return;
			}
			else
			{
				gci.cprintf(ent, PRINT_HIGH, "bad camera target - \"%s\"\n", gci.argv(2));
				return;
			}
		}
		else if (Q_strcasecmp(gci.argv(1), "add") == 0)
		{
			if (dedicated->value != 0 || clientID != 0 ||
				!(clients[clientID].spectator && clients[clientID].mode == CAMERA_FREE))
				gci.cprintf(ent, PRINT_HIGH, "can't add cameras\n");
			else
			{
				clients[clientID].camera = camera_fixed_add(ent, gci.argv(2));
				ticker_update_camera(ent, clients[clientID].camera);
			}
			return;
		}
		else if (Q_strcasecmp(gci.argv(1), "update") == 0)
		{
			if (dedicated->value != 0 || clientID != 0 ||
				!(clients[clientID].spectator && clients[clientID].mode == CAMERA_FREE))
				gci.cprintf(ent, PRINT_HIGH, "can't update cameras\n");
			else
			{
				if (clients[clientID].camera)
					camera_fixed_update(clients[clientID].camera, ent, gci.argv(2));
				else
					clients[clientID].camera = camera_fixed_add(ent, gci.argv(2));
				ticker_update_camera(ent, clients[clientID].camera);
			}
			return;
		}
		else
		{
			gci.cprintf(ent, PRINT_HIGH, "no such camera option - \"%s %s\"\n", gci.argv(1), gci.argv(2));
			return;
		}
	}

	if (clients[clientID].mode != oldmode && isSpectator(clientID))
	{
		switch (oldmode)
		{
		case CAMERA_FREE:
			clients[clientID].reset_layouts = (clients[clientID].mode == CAMERA_CHASE);
			camera_free_wrapup(clientID);
			break;
		case CAMERA_CHASE:
			camera_chase_wrapup(clientID);
			break;
		case CAMERA_ACTION:
			clients[clientID].reset_layouts = (clients[clientID].mode == CAMERA_CHASE);
			camera_action_wrapup(clientID);
		}
		switch (clients[clientID].mode)
		{
		case CAMERA_FREE:
			camera_free_setup(clientID);
			break;
		case CAMERA_CHASE:
			camera_chase_setup(clientID);
			break;
		case CAMERA_ACTION:
			camera_action_setup(clientID);
		}
	}

	switch (clients[clientID].mode)
	{
	case CAMERA_FREE:
		gci.cprintf(ent, PRINT_HIGH, "camera mode is FREE\n");
		break;
	case CAMERA_CHASE:
		gci.cprintf(ent, PRINT_HIGH, "camera mode is CHASE\n");
		break;
	case CAMERA_ACTION:
		gci.cprintf(ent, PRINT_HIGH, "camera mode is ACTION\n");
		if (clients[clientID].iMode == CAM_NORMAL_MODE)
			gci.cprintf(ent, PRINT_HIGH, "ACTION camera mode is NORMAL\n");
		else
			gci.cprintf(ent, PRINT_HIGH, "ACTION camera mode is FOLLOW\n");
	}
	return;
}


void Cmd_Players_f(edict_t* ent)
{
	int		i;
	char	psmall[64];
	char	plarge[1024] = { 0 };
	char* tail;
	edict_t* current;

	wait_camera = NULL;
	wait_cprintf = NULL;
	if (clients[numEdict(ent) - 1].spectator)
	{
		wait_camera = ent;
		for (i = 0; i < maxclients->value; i++)
		{
			current = Edict(i + 1);
			if ((clients[i].inuse &&
				clients[i].begin &&
				!clients[i].spectator) ||
				(!((int)gc_flags->value & GCF_AVOID_BOTS) &&
					current->inuse &&
					current->client != NULL))
			{
				wait_cprintf = current;
				break;
			}
		}
	}
	plarge[0] = '\0';
	captured_print_message[0] = '\0';
	tail = NULL;
	if (wait_cprintf != NULL)
		GameCommand(wait_cprintf, "players");
	else
	{
		if (wait_camera == NULL)
		{
			wait_camera = ent;
			wait_cprintf = ent;
			GameCommand(ent, "players");
		}
	}
	wait_camera = NULL;
	wait_cprintf = NULL;

	if (captured_print_message[0])
	{
		tail = captured_print_message + strlen(captured_print_message) - 8;
		if (strcmp(tail, "players\n"))
			tail = NULL;
		else
		{
			while (tail > captured_print_message && *tail != '\n') tail--;
			*tail = '\0';
			tail++;
		}
	}

	for (i = 0; i < maxclients->value; i++)
	{
		if (clients[i].inuse && clients[i].spectator)
		{
			if (gc_rename_client->value)
				Com_sprintf(psmall, sizeof(psmall), "--- [CAMERA]%s\n",
					Info_ValueForKey(clients[i].userinfo, "name"));
			else
				Com_sprintf(psmall, sizeof(psmall), "%s\n",
					Info_ValueForKey(clients[i].userinfo, "name"));
			if (strlen(psmall) + strlen(plarge) > sizeof(plarge) - 100)
			{
				// can't print all of them in one packet
				strcat(plarge, "...\n");
				break;
			}
			strcat(plarge, psmall);
		}
	}
	gci.cprintf(ent, PRINT_HIGH, "%s%s\n%s%i spectators\n", captured_print_message, plarge, (tail) ? tail : "0 players\n", cam_count);
}


void Cmd_Kill_f(edict_t* ent)
{
	SpectatorEnd(ent, "");
}


void Cmd_Help_f(edict_t* ent)
{
	int clientID;
	edict_t* target;

	clientID = numEdict(ent) - 1;

	clients[clientID].inven = false;
	clients[clientID].score = false;
	clients[clientID].menu = false;
	clients[clientID].layouts = false;
	clients[clientID].help = !clients[clientID].help;

	if (clients[clientID].help)
	{
		target = Edict(clients[clientID].target + 1);
		wait_camera = ent;
		wait_help = target;
		if (!(target->client->ps.stats[STAT_LAYOUTS]))
		{
			GameCommand(target, "help");
			GameCommand(target, "help");
		}
		else
		{
			gci.centerprintf(ent, "client HUD is in use\ntry again later");
			clients[clientID].help = false;
		}
		wait_camera = NULL;
		wait_help = NULL;
	}
	if (!clients[clientID].help)
		clients[clientID].update_chase = true;
}


void Cmd_Score_f(edict_t* ent)
{
	int clientID;

	clientID = numEdict(ent) - 1;

	clients[clientID].inven = false;
	clients[clientID].help = false;
	clients[clientID].menu = false;
	clients[clientID].layouts = false;

	clients[clientID].score = !clients[clientID].score;

	if (clients[clientID].score)
	{
		UpdateScore(clientID);
		if (clients[clientID].mode != CAMERA_CHASE)
			ticker_remove_statusbar(ent);
	}
	else
	{
		if (clients[clientID].mode == CAMERA_CHASE)
		{
			clients[clientID].update_chase = true;
		}
		else
		{
			ent->client->ps.stats[STAT_LAYOUTS] &= ~1;
			ticker_restore_statusbar(ent);
		}
	}
}


void Cmd_Putaway_f(edict_t* ent)
{
	int clientID;

	clientID = numEdict(ent) - 1;
	// if camera is viewing any layout then force the main menu
	// (this hack is needed becase the "Chasing" message is 
	//  a layout itself, so Quake2 does not open the main menu
	//  when pressing ESC)
	if (!clients[clientID].inven &&
		!clients[clientID].score &&
		!clients[clientID].help &&
		!clients[clientID].menu)
	{
		gci.WriteByte(svc_stufftext);
		gci.WriteString("menu_main");
		gci.unicast(ent, true);
	}
	else
	{
		clients[clientID].inven = false;
		clients[clientID].score = false;
		clients[clientID].help = false;
		clients[clientID].menu = false;
		clients[clientID].layouts = false;
		if (clients[clientID].spectator && clients[clientID].mode == CAMERA_CHASE)
			clients[clientID].update_chase = true;
		else
		{
			ent->client->ps.stats[STAT_LAYOUTS] &= ~1;
			ticker_restore_statusbar(ent);
		}
	}
}


void Cmd_NextClient_f(edict_t* ent)
{
	int clientID, other;

	clientID = numEdict(ent) - 1;

	if (!((int)gc_flags->value & GCF_ALLOW_CHASE) &&
		clients[clientID].mode != CAMERA_CHASE)
	{
		gci.cprintf(ent, PRINT_HIGH, "CHASE mode not allowed\n");
		return;
	}

	switch (clients[clientID].mode)
	{
	case CAMERA_FREE:
		clients[clientID].reset_layouts = true;
		camera_free_wrapup(clientID);
		clients[clientID].mode = CAMERA_CHASE;
		camera_chase_setup(clientID);
		break;
	case CAMERA_CHASE:
		other = NextClient(clients[clientID].target);
		clients[clientID].target = other;
		clients[clientID].help = false;
		clients[clientID].score = false;
		clients[clientID].inven = false;
		clients[clientID].layouts = false;
		clients[clientID].update_chase = true;
		clients[clientID].chase_distance = 0.0F;
		clients[clientID].chase_height = 0.0F;
		clients[clientID].chase_angle = 0.0F;
		clients[clientID].chase_yaw = 0.0F;
		clients[clientID].chase_pitch = 0.0F;
		return;
		break;
	case CAMERA_ACTION:
		other = NextClient(clientID);
		if (other != clientID)
		{
			clients[clientID].reset_layouts = true;
			camera_action_wrapup(clientID);
			clients[clientID].mode = CAMERA_CHASE;
			camera_chase_setup(clientID);
		}
		break;
	}
	if ((int)gc_flags->value & GCF_VERBOSE)
		gci.cprintf(ent, PRINT_HIGH, "camera mode is %s\n",
			(clients[clientID].mode == CAMERA_CHASE) ?
			"CHASE" : "ACTION");
}


void Cmd_PrevClient_f(edict_t* ent)
{
	int clientID, other;

	clientID = numEdict(ent) - 1;

	if (!((int)gc_flags->value & GCF_ALLOW_CHASE) &&
		clients[clientID].mode != CAMERA_CHASE)
	{
		gci.cprintf(ent, PRINT_HIGH, "CHASE mode not allowed\n");
		return;
	}

	switch (clients[clientID].mode)
	{
	case CAMERA_FREE:
		clients[clientID].reset_layouts = true;
		camera_free_wrapup(clientID);
		clients[clientID].mode = CAMERA_CHASE;
		camera_chase_setup(clientID);
		break;
	case CAMERA_CHASE:
		other = PrevClient(clients[clientID].target);
		clients[clientID].target = other;
		clients[clientID].help = false;
		clients[clientID].score = false;
		clients[clientID].inven = false;
		clients[clientID].layouts = false;
		clients[clientID].update_chase = true;
		clients[clientID].chase_distance = 0.0F;
		clients[clientID].chase_height = 0.0F;
		clients[clientID].chase_angle = 0.0F;
		clients[clientID].chase_yaw = 0.0F;
		clients[clientID].chase_pitch = 0.0F;
		return;
		break;
	case CAMERA_ACTION:
		other = PrevClient(clientID);
		if (other != clientID)
		{
			clients[clientID].reset_layouts = true;
			camera_action_wrapup(clientID);
			clients[clientID].mode = CAMERA_CHASE;
			camera_chase_setup(clientID);
		}
		break;
	}
	if ((int)gc_flags->value & GCF_VERBOSE)
		gci.cprintf(ent, PRINT_HIGH, "camera mode is %s\n",
			(clients[clientID].mode == CAMERA_CHASE) ?
			"CHASE" : "ACTION");
}


void Cmd_NextItem_f(edict_t* ent)
{
	int next;
	qboolean found = false;
	int clientID;

	clientID = numEdict(ent) - 1;

	for (next = clients[clientID].item + 1; next < MAX_ITEMS && !found; next++)
		found = (clients[clientID].items[next] != 0);
	if (!found)
		for (next = 0; next < clients[clientID].item && !found; next++)
			found = (clients[clientID].items[next] != 0);
	if (found)
		clients[clientID].item = next - 1;
}



void Cmd_PrevItem_f(edict_t* ent)
{
	int prev;
	qboolean found = false;
	int clientID;

	clientID = numEdict(ent) - 1;

	for (prev = clients[clientID].item - 1; prev >= 0 && !found; prev--)
		found = (clients[clientID].items[prev] != 0);
	if (!found)
		for (prev = MAX_ITEMS - 1; prev > clients[clientID].item && !found; prev--)
			found = (clients[clientID].items[prev] != 0);
	if (found)
		clients[clientID].item = prev + 1;
}


void Cmd_NextOption_f(edict_t* ent)
{
	PMenu_Next(ent);
}


void Cmd_PrevOption_f(edict_t* ent)
{
	PMenu_Prev(ent);
}


void Cmd_SelectOption_f(edict_t* ent)
{
	PMenu_Select(ent);
}


void Cmd_ParentMenu_f(edict_t* ent)
{
	PMenu_Parent(ent);
}


void say(edict_t* ent, qboolean everyone)
{
	int		j;
	int		clientID;
	char* p;
	char	text[2048];

	clientID = numEdict(ent) - 1;
	if (gc_rename_client->value)
		Com_sprintf(text, sizeof(text), "[CAMERA]%s: ",
			Info_ValueForKey(clients[clientID].userinfo, "name"));
	else
		Com_sprintf(text, sizeof(text), "%s: ",
			Info_ValueForKey(clients[clientID].userinfo, "name"));

	p = gci.args();
	if (*p == '"')
	{
		p++;
		p[strlen(p) - 1] = 0;
	}
	strcat(text, p);

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	strcat(text, "\n");

	if (flood_msgs->value)
	{
		int i;

		if (leveltime < clients[clientID].flood_locktill)
		{
			gci.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
				(int)(clients[clientID].flood_locktill - leveltime));
			return;
		}
		i = clients[clientID].flood_whenhead - ((int)flood_msgs->value) + 1;
		if (i < 0)
			i = ((int)sizeof(clients[clientID].flood_when) / (int)sizeof(clients[clientID].flood_when[0])) + i;
		if (clients[clientID].flood_when[i] && leveltime - clients[clientID].flood_when[i] < flood_persecond->value)
		{
			clients[clientID].flood_locktill = leveltime + flood_waitdelay->value;
			gci.cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n",
				(int)flood_waitdelay->value);
			return;
		}
		clients[clientID].flood_whenhead = (clients[clientID].flood_whenhead + 1) %
			((int)sizeof(clients[clientID].flood_when) / (int)sizeof(clients[clientID].flood_when[0]));
		clients[clientID].flood_when[clients[clientID].flood_whenhead] = leveltime;
	}

	if (dedicated->value)
		gci.cprintf(NULL, PRINT_CHAT, "%s", text);

	j = 0;
	while (j < maxclients->value)
	{
		if (clients[j].inuse && (everyone || clients[j].spectator))
			gci.cprintf(Edict(j + 1), PRINT_CHAT, "%s", text);
		j++;
	}
}


void Cmd_Say_f(edict_t* ent)
{
	if (gci.argc() < 2)
		return;

	if (!((int)gc_flags->value & GCF_ALLOW_SAY))
	{
		gci.cprintf(ent, PRINT_HIGH, motd("camera chat not allowed\nuse '\\!say_team\\!'\n"));
		return;
	}

	say(ent, true);
}

void Cmd_SayTeam_f(edict_t* ent)
{
	if (gci.argc() < 2)
		return;
	say(ent, false);
}



void Cmd_ActionCam_f(edict_t* ent)
{
	int clientID, oldmode;

	clientID = numEdict(ent) - 1;
	oldmode = clients[clientID].mode;
	clients[clientID].mode = CAMERA_ACTION;

	switch (oldmode)
	{
	case CAMERA_FREE:
		clients[clientID].reset_layouts = false;
		camera_free_wrapup(clientID);
		break;
	case CAMERA_CHASE:
		camera_chase_wrapup(clientID);
		break;
	case CAMERA_ACTION:
		/*
		if (priority_list != NULL)
		{
		clients[clientID].no_priority = !clients[clientID].no_priority;
		clients[clientID].last_move_time = 0; // force re-think
		if ((int) gc_flags->value & GCF_VERBOSE)
		if (clients[clientID].no_priority)
		gci.cprintf (ent, PRINT_HIGH, "target selection method is DEFAULT\n");
		else
		gci.cprintf (ent, PRINT_HIGH, "target selection method is PRIORITY\n");
		}
		*/
		return;
		break;
	}

	camera_action_setup(clientID);
	if ((int)gc_flags->value & GCF_VERBOSE)
		gci.cprintf(ent, PRINT_HIGH, "camera mode is ACTION\n");
}

void Cmd_FreeCam_f(edict_t* ent)
{
	int clientID, oldmode;

	clientID = numEdict(ent) - 1;

	if (!((int)gc_flags->value & GCF_ALLOW_FREE) &&
		clients[clientID].mode != CAMERA_FREE)
	{
		gci.cprintf(ent, PRINT_HIGH, "FREE mode not allowed\n");
		return;
	}

	oldmode = clients[clientID].mode;
	clients[clientID].mode = CAMERA_FREE;

	switch (oldmode)
	{
	case CAMERA_FREE:
		return;
		break;
	case CAMERA_CHASE:
		camera_chase_wrapup(clientID);
		break;
	case CAMERA_ACTION:
		clients[clientID].reset_layouts = false;
		camera_action_wrapup(clientID);
		break;
	}

	camera_free_setup(clientID);
	if ((int)gc_flags->value & GCF_VERBOSE)
		gci.cprintf(ent, PRINT_HIGH, "camera mode is FREE\n");
}


void Cmd_CameraMenu_f(edict_t* ent)
{
	int clientID;

	clientID = numEdict(ent) - 1;

	if (clients[clientID].menu)
		hideMenu(clientID);
	else
		showMenu(clientID);
}


void ClientCommand(edict_t* ent)
{
	int clientID;
	char* cmd;

	camera_command_flag = false;

	if (!ent->client)
		return;		// not fully in game yet

	cmd = gci.argv(0);

	clientID = numEdict(ent) - 1;

	if (Q_strcasecmp(cmd, "camera") == 0)
	{
		Cmd_Camera_f(ent);
		return;
	}
	else if (Q_strcasecmp(cmd, "players") == 0)
	{
		Cmd_Players_f(ent);
		return;
	}
	else if (Q_strcasecmp(cmd, "invuse") == 0 && isMenuOn(clientID))
	{
		Cmd_SelectOption_f(ent);
		return;
	}
	else if (Q_strcasecmp(cmd, "invuse") == 0 && isSpectator(clientID))
	{
		Cmd_ActionCam_f(ent);
		return;
	}
	else if (Q_strcasecmp(cmd, "invdrop") == 0 && isMenuOn(clientID))
	{
		Cmd_ParentMenu_f(ent);
		return;
	}
	else if (Q_strcasecmp(cmd, "invdrop") == 0 && isSpectator(clientID))
	{
		Cmd_FreeCam_f(ent);
		return;
	}
	else if (Q_strcasecmp(cmd, "inven") == 0 && isSpectator(clientID))
	{
		Cmd_CameraMenu_f(ent);
		return;
	}
	else if (Q_strcasecmp(cmd, "invnext") == 0 && isMenuOn(clientID))
	{
		Cmd_NextOption_f(ent);
		return;
	}
	else if (Q_strcasecmp(cmd, "invnext") == 0 && isSpectator(clientID))
	{
		if (isInvenOn(clientID))
			Cmd_NextItem_f(ent);
		else
			Cmd_NextClient_f(ent);
		return;
	}
	else if (Q_strcasecmp(cmd, "invprev") == 0 && isMenuOn(clientID))
	{
		Cmd_PrevOption_f(ent);
		return;
	}
	else if (Q_strcasecmp(cmd, "invprev") == 0 && isSpectator(clientID))
	{
		if (isInvenOn(clientID))
			Cmd_PrevItem_f(ent);
		else
			Cmd_PrevClient_f(ent);
		return;
	}
	else if (Q_strcasecmp(cmd, "kill") == 0 && isSpectator(clientID))
	{
		Cmd_Kill_f(ent);
		return;
	}
	else if (Q_strcasecmp(cmd, "help") == 0 &&
		isSpectator(clientID) &&
		isFollowing(clientID) &&
		!deathmatch->value) // coop or single player
	{
		Cmd_Help_f(ent);
		return;
	}
	else if ((Q_strcasecmp(cmd, "help") == 0 || Q_strcasecmp(cmd, "score") == 0) &&
		isSpectator(clientID))
	{
		Cmd_Score_f(ent);
		return;
	}
	else if (Q_strcasecmp(cmd, "say") == 0 && isSpectator(clientID))
	{
		Cmd_Say_f(ent);
		return;
	}
	else if (Q_strcasecmp(cmd, "say_team") == 0 && isSpectator(clientID))
	{
		Cmd_SayTeam_f(ent);
		return;
	}
	else if (Q_strcasecmp(cmd, "putaway") == 0 && (isSpectator(clientID) || isMenuOn(clientID)))
	{
		Cmd_Putaway_f(ent);
		return;
	}
	else if (!isSpectator(clientID))
	{
		if (Q_strcasecmp(cmd, "say") == 0 || Q_strcasecmp(cmd, "say_team") == 0)
			first_print_message[0] = '\0';
		ge.ClientCommand(ent);
	}
}

void ServerCommand(void)
{
	camera_command_flag = false;
	ge.ServerCommand();
}


void GameCommand(edict_t* ent, char* command)
{
	char c;
	char* com_token;
	char* command_temp;

	if (command == NULL)
		return;

	camera_command_flag = true;

	command_argc = 0;
	command_temp = command;
	com_token = COM_Parse(&command_temp);
	if (command_temp)
	{
		// skip whitespace
		while ((c = *command_temp) <= ' ')
		{
			if (c == 0)
			{
				command_temp = NULL;
				break;
			}
			command_temp++;
		}
		if (command_temp)
			Q_strncpyz(command_args, command_temp, MAX_STRING_CHARS - 1);
		else
			command_args[0] = '\0';
	}
	else
	{
		camera_command_flag = false;
		return;
	}
	Q_strncpyz(command_argv[0], com_token, MAX_TOKEN_CHARS - 1);
	while (strlen((com_token = COM_Parse(&command_temp))))
	{
		command_argc++;
		Q_strncpyz(command_argv[command_argc], com_token, MAX_TOKEN_CHARS - 1);
	}
	command_argc++;
	ge.ClientCommand(ent);
}

int gc_argc(void)
{
	if (camera_command_flag)
		return command_argc;
	else
		return gci.argc();
}

char* gc_argv(int n)
{
	if (camera_command_flag)
	{
		if (n < 0 || n >= command_argc)
			return "";
		return command_argv[n];
	}
	else
		return gci.argv(n);

}

char* gc_args(void)
{
	if (camera_command_flag)
		return command_args;
	else
		return gci.args();
}

