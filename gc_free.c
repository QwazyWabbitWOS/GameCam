// GameCam v1.02 - camera proxy game module for Quake II
// Copyright (c) 1998-99, by Avi Rozen (Zung!)
// e-mail: zungbang@telefragged.com

// gc_free.c		free camera mode

#include "gamecam.h"

void camera_free_setup (int clientID)
{
	edict_t *ent;

	// clear all effects and layouts
	ent = Edict(clientID+1);
	ent->client->ps.gunindex = 0;
	VectorClear(ent->client->ps.kick_angles);
	//memset(ent->client->ps.stats, 0, MAX_STATS * sizeof(short));
	ent->s.effects = 0;
	ent->s.renderfx = 0;
	ent->s.sound = 0;
	ent->s.event = 0;
	// reset fov
	if (gc_set_fov->value)
		set_fov (ent, 90, QFALSE);

	ticker_clear (ent);
}

void camera_free_wrapup (int clientID)
{
	edict_t *ent;

	ent = Edict(clientID + 1);

	// restore status bar & remove layouts
	if (clients[clientID].reset_layouts) 
		ticker_wrapup (ent);

	clients[clientID].inven = QFALSE;
	clients[clientID].score = QFALSE;
	clients[clientID].help = QFALSE;
	clients[clientID].menu = QFALSE;
	clients[clientID].layouts = QFALSE;
	clients[clientID].statusbar_removed = QFALSE;

	ent->client->ps.stats[STAT_LAYOUTS] = 0;
}


void camera_free_begin (int clientID)
{
	edict_t *ent;

	ent = Edict(clientID + 1);

	// reset fov
	if (gc_set_fov->value)
		set_fov (ent, 90, QFALSE);

	ticker_clear (ent);

	clients[clientID].inven = QFALSE;
	clients[clientID].score = QFALSE;
	clients[clientID].help = QFALSE;
	clients[clientID].menu = QFALSE;
	clients[clientID].layouts = QFALSE;
	clients[clientID].statusbar_removed = QFALSE;
}


void camera_free_add_target (int other)
{
}


void camera_free_remove_target (int other)
{
}


void camera_free_frame (int clientID)
{
	edict_t *ent;

	ent = Edict(clientID + 1);
	// update viewing effects (water, lava, slime)
	SV_CalcBlend (ent);
	// update client id view
	if (!clients[clientID].score && !intermission)
	{
		ent->client->ps.stats[STAT_ID_VIEW] = 0;
		if (clients[clientID].id)
			SetIDView(ent, QFALSE); // QFALSE means that we id bots too);
	}
}


void camera_free_think (int clientID, usercmd_t *cmd)
{
	edict_t *ent;
	gclient_t	*client;
	int i;
	pmove_t	pm;

	if (intermission)
		return;

	ent = Edict(clientID + 1);
	client = ent->client;

	pm_passent = ent;

	client->ps.pmove.pm_type = PM_SPECTATOR;

	// set up for pmove
	memset (&pm, 0, sizeof(pm));

	client->ps.pmove.gravity = (short) sv_gravity->value;
	pm.s = client->ps.pmove;

	for (i=0 ; i<3 ; i++)
	{
		pm.s.origin[i] = (short) ent->s.origin[i]*8;
		pm.s.velocity[i] = (short) clients[clientID].velocity[i]*8;
	}

	if (memcmp(&clients[clientID].old_pmove, &pm.s, sizeof(pm.s)))
	{
		pm.snapinitial = QTRUE;
		//gci.dprintf ("pmove changed!\n");
	}

	pm.cmd = *cmd;

	pm.trace = PM_trace;	// adds default parms
	pm.pointcontents = gci.pointcontents;

	// perform a pmove
	gci.Pmove (&pm);

	// save results of pmove
	client->ps.pmove = pm.s;
	clients[clientID].old_pmove = pm.s;
	clients[clientID].viewheight = pm.viewheight;
	clients[clientID].waterlevel = pm.waterlevel;
	clients[clientID].watertype = pm.watertype;
	clients[clientID].groundentity = pm.groundentity;

	for (i=0 ; i<3 ; i++)
	{
		ent->s.origin[i] = pm.s.origin[i]*0.125F;
		clients[clientID].velocity[i] = pm.s.velocity[i]*0.125F;
	}

	VectorCopy (pm.mins, ent->mins);
	VectorCopy (pm.maxs, ent->maxs);
	VectorCopy (pm.viewangles, ent->s.angles);
	VectorCopy (pm.viewangles, client->ps.viewangles);
	VectorCopy (pm.viewangles, clients[clientID].v_angle);
}

