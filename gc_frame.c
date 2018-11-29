// GameCam v1.02 - camera proxy game module for Quake II
// Copyright (c) 1998-99, by Avi Rozen (Zung!)
// e-mail: zungbang@telefragged.com

// gb_camera.c		update world, cameras and clients

#include "gamecam.h"

long framenum = 0;
float leveltime = 0.0F;
long match_startframe = 0;
long match_offsetframes = 0;
long match_updateframes = 0;
qboolean match_10sec = QFALSE;
qboolean match_started = QFALSE;
qboolean intermission = QFALSE;
int best_clientID;

void RunFrame (void)
{
	int clientID;
	edict_t *ent;
	float save_timelimit = 0.0F;
	char s_save_timelimit[MAX_INFO_VALUE];
	char s_timelimit[MAX_INFO_VALUE];

	framenum++;
	leveltime = framenum * FRAMETIME;
	first_print_message[0] = '\0';

	if (gc_autocam->value)
	{
		if (match_started)
		{
			if (timelimit->value && gc_update->value)
			{
				match_updateframes++;
				if (match_updateframes / 600.0F >= gc_update->value)
				{
					float minutes_left = timelimit->value - (framenum - match_startframe) / 600.0F;
					match_updateframes = 0;
					if (minutes_left > 0)
						gci.bprintf (PRINT_CHAT, "[GameCam]: match ends in %d minutes\n", (int) minutes_left);
				}
			}
			if (timelimit->value && !match_10sec && ((long) (timelimit->value * 600.0F)) - (framenum - match_startframe) <= 100)
			{
				match_10sec = QTRUE;
				gci.sound (gce->edicts, CHAN_AUTO, gci.soundindex ("world/10_0.wav"), 1, ATTN_NONE, 0);
			}
		}
		else
			match_offsetframes++;
		if (timelimit->value)
		{
			save_timelimit = timelimit->value;
			strcpy (s_save_timelimit, timelimit->string);
			if (match_started)
			{
				sprintf (s_timelimit, "%f", timelimit->value + match_offsetframes / 600.0F);
				gci.cvar_set ("timelimit", s_timelimit);
			}
			else
				gci.cvar_set ("timelimit", "0");
		}
	}

	ge.RunFrame();

	if (gc_autocam->value && save_timelimit)
		gci.cvar_set ("timelimit", s_save_timelimit);

	if (!intermission)
	{
		intermission = detect_intermission ();
		if (intermission)
		{
			best_clientID = getBestClient ();
			for (clientID = 0; clientID < maxclients->value; clientID++)
				clients[clientID].intermission = QFALSE;
		}
	}

	// update ticker tape
	ticker_flags = (cam_count)? ticker_update () : 0;

	for (clientID = 0; clientID < maxclients->value; clientID++) 
	{
		ent = Edict(clientID + 1);
		if (clients[clientID].ticker_delay)
		{
			clients[clientID].ticker_delay--;
			if (clients[clientID].ticker_delay < 0)
				clients[clientID].ticker_delay = 0;
			if (clients[clientID].ticker_delay == 0)
				ticker_setup (ent);
		}
		if (gc_autocam->value && clients[clientID].motddelay)
		{
			clients[clientID].motddelay--;
			if (!clients[clientID].motddelay)
				gci.centerprintf (ent, motd (gc_motd->string));
		}

		if (clients[clientID].inuse && 
			clients[clientID].begin && 
			clients[clientID].spectator)
		{
			// is spectator in limbo?
			if (clients[clientID].limbo_time == 0) 
			{ 
				// not in limbo
				switch (clients[clientID].mode) 
				{
				case CAMERA_FREE:
					camera_free_frame (clientID);
					break;
				case CAMERA_CHASE:
					camera_chase_frame (clientID);
					break;
				case CAMERA_ACTION:
					camera_action_frame (clientID);
				}
				if (!intermission)
				{
					if (clients[clientID].score && !(framenum & 31))
						UpdateScore (clientID);
					if (clients[clientID].inven) 
					{
						ent->client->ps.stats[STAT_LAYOUTS] = 2;
						ent->client->ps.stats[STAT_SELECTED_ITEM] = clients[clientID].item;
					}
					else 
					{
						if (!clients[clientID].layouts)
							ent->client->ps.stats[STAT_LAYOUTS] &= ~2;
					}
				}
				else 
				{
					if (!clients[clientID].intermission)
					{
						clients[clientID].intermission = QTRUE;
						ticker_remove_statusbar (ent);
						if (clients[clientID].last_score != framenum) // prevent overflow
						{ 
							gci.WriteByte (svc_layout);
							gci.WriteString (clients[best_clientID].layout);
							gci.unicast(ent, QTRUE);
							clients[clientID].last_score = framenum;
						}
						ent->client->ps.stats[STAT_LAYOUTS] &= ~2;
						ent->client->ps.stats[STAT_LAYOUTS] |= 1;
						ent->client->ps.pmove.pm_type = PM_FREEZE;
					}
				}
				if (clients[clientID].mode != CAMERA_CHASE &&
					!clients[clientID].score &&
					!intermission)
					ticker_frame (ent);
			} 
			else 
			{   
				// spectator in limbo
				clients[clientID].limbo_time--;
				switch (clients[clientID].limbo_time)
				{
				case 0:
					ReturnToGame (ent);
					break;
				case 1:
					CameraOff (ent);
					break;
				default:
					break;
				}
			}
		}
		// find out who has just died
		if ((clients[clientID].inuse && 
			clients[clientID].begin &&	
			!clients[clientID].spectator) || 
			(ent->inuse &&
			ent->s.modelindex != 0))
		{
			if (ent->client->ps.pmove.pm_type != clients[clientID].last_pmtype) 
			{
				clients[clientID].last_pmtype = ent->client->ps.pmove.pm_type;
				if (clients[clientID].last_pmtype == PM_DEAD)
					pDeadPlayer = ent;
			}
		}
		// update menu
		if (clients[clientID].menu)
			ent->client->ps.stats[STAT_LAYOUTS] |= 1;
		else if (clients[clientID].menu_hnd)
			PMenu_Close (ent);
	}
}


int current_client = -1;

void ClientThink (edict_t *ent, usercmd_t *cmd)
{
	int clientID;

	clientID = numEdict(ent) - 1;

	// clients in limbo can't move
	clients[clientID].cmd_angles[0] = SHORT2ANGLE(cmd->angles[0]);
	clients[clientID].cmd_angles[1] = SHORT2ANGLE(cmd->angles[1]);
	clients[clientID].cmd_angles[2] = SHORT2ANGLE(cmd->angles[2]);

	if (clients[clientID].limbo_time == 0)
	{
		if (clients[clientID].spectator)
			switch (clients[clientID].mode) 
		{
			case CAMERA_FREE:
				camera_free_think (clientID, cmd);
				break;
			case CAMERA_CHASE:
				camera_chase_think (clientID, cmd);
				break;
			case CAMERA_ACTION:
				camera_action_think (clientID, cmd);
		}
		else 
		{
			current_client = clientID;
			ge.ClientThink (ent, cmd);
			current_client = -1;
		}
	}
	/*	else 
	{
	int i;

	ent->client->ps.pmove.pm_type = PM_FREEZE;
	ent->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
	for (i=0 ; i<3 ; i++)
	ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(ent->client->ps.viewangles[i] - clients[clientID].cmd_angles[i]);
	} */

}


void Pmove (pmove_t *pmove)
{
	gci.Pmove (pmove);

	if (current_client < 0)
		return;

	// save results
	clients[current_client].old_pmove = pmove->s;
	clients[current_client].viewheight = pmove->viewheight;
	clients[current_client].waterlevel = pmove->waterlevel;
	clients[current_client].watertype = pmove->watertype;
	clients[current_client].groundentity = pmove->groundentity;
	clients[current_client].velocity[0] = pmove->s.velocity[0]*0.125F;
	clients[current_client].velocity[1] = pmove->s.velocity[1]*0.125F;
	clients[current_client].velocity[2] = pmove->s.velocity[2]*0.125F;
	VectorCopy (pmove->viewangles, clients[current_client].v_angle);
}

