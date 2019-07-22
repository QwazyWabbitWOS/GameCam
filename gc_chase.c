// GameCam v1.02 - camera proxy game module for Quake II
// Copyright (c) 1998-99, by Avi Rozen (Zung!)
// e-mail: zungbang@telefragged.com

// gc_chase.c		chase camera mode

#include "gamecam.h"


void camera_chase_setup (int clientID)
{
	edict_t *ent;
	int other;

	ent = Edict(clientID + 1);
	other = ClosestClient (clientID);
	// no one to follow? -> goto free camera mode
	if (other == clientID) 
	{
		gci.cprintf(ent, PRINT_HIGH, "no one to follow\n");
		clients[clientID].mode = CAMERA_ACTION;
		camera_action_setup (clientID);
	} 
	else 
	{
		clients[clientID].target = other;
		clients[clientID].update_chase = true;
		VectorClear(ent->client->ps.viewoffset);
		ent->client->ps.gunindex = 0;
		ent->client->ps.gunframe = 0;
		clients[clientID].chase_distance = 0.0F;
		clients[clientID].chase_height = 0.0F;
		clients[clientID].chase_angle = 0.0F;
		clients[clientID].chase_yaw = 0.0F;
		clients[clientID].chase_pitch = 0.0F;
		// reset fov
		if (gc_set_fov->value)
			set_fov (ent, 90, false);
	}
}


void camera_chase_wrapup (int clientID)
{
	clients[clientID].target = -1;
	clients[clientID].inven = false;
	clients[clientID].score = false;
	clients[clientID].help = false;
	clients[clientID].menu = false;
	clients[clientID].layouts = false;
	clients[clientID].statusbar_removed = false;
	(Edict(clientID + 1))->client->ps.stats[STAT_LAYOUTS] = 0;
}


void camera_chase_begin (int clientID)
{
	edict_t *ent;

	ent = Edict(clientID + 1);

	clients[clientID].inven = false;
	clients[clientID].score = false;
	clients[clientID].help = false;
	clients[clientID].menu = false;
	clients[clientID].layouts = false;
	clients[clientID].statusbar_removed = false;
	ent->client->ps.stats[STAT_LAYOUTS] = 0;
	clients[clientID].chase_distance = 0.0F;
	clients[clientID].chase_height = 0.0F;
	clients[clientID].chase_angle = 0.0F;
	clients[clientID].chase_yaw = 0.0F;
	clients[clientID].chase_pitch = 0.0F;
	// reset fov
	if (gc_set_fov->value)
		set_fov (ent, 90, false);
}


void camera_chase_add_target (int other)
{
}

void camera_chase_remove_target (int other)
{
	int clientID;

	for (clientID = 0; clientID < maxclients->value; clientID++)
		if (clients[clientID].inuse &&
			clients[clientID].spectator &&
			clients[clientID].mode == CAMERA_CHASE &&
			clients[clientID].target == other) 
		{
			camera_chase_wrapup (clientID);
			clients[clientID].mode = CAMERA_ACTION;
			camera_action_setup (clientID);
		}
}


void camera_chase_frame (int clientID)
{
	vec3_t o, ownerv, goal, vDiff;
	edict_t *target;
	edict_t *ent;
	vec3_t forward, right;
	trace_t trace;
	int i;
	vec3_t angles;

	if (clients[clientID].target < 0)
		return;

	ent	= Edict(clientID + 1);
	target = Edict(clients[clientID].target + 1);

	memcpy(ent->client->ps.stats, target->client->ps.stats, MAX_STATS * sizeof(short));

	if (clients[clientID].layouts)
	{
		if (memcmp(clients[clientID].items, clients[clients[clientID].target].items, MAX_ITEMS * sizeof(short))) 
		{
			memcpy(clients[clientID].items, clients[clients[clientID].target].items, MAX_ITEMS * sizeof(short));
			gci.WriteByte (svc_inventory);
			for (i = 0; i < MAX_ITEMS; i++)
				gci.WriteShort (clients[clientID].items[i]);
			gci.unicast(ent, true);
		}
		if (strcmp(clients[clientID].layout, clients[clients[clientID].target].layout)) 
		{
			strcpy(clients[clientID].layout, clients[clients[clientID].target].layout);
			if (clients[clientID].last_score != framenum) 
			{ 
				// prevent overflow
				gci.WriteByte (svc_layout);
				gci.WriteString (clients[clientID].layout);
				gci.unicast(ent, true);
				clients[clientID].last_score = framenum;
			}
		}
	}
	else
		ent->client->ps.stats[STAT_LAYOUTS] |= 1;

	if (clients[clientID].chase_auto)
	{
		edict_t *old_target = clients[clientID].creep_target;

		clients[clientID].creep_target = camera_creep_target (clientID);
		if (old_target != clients[clientID].creep_target)
			clients[clientID].update_chase = true;
		camera_creep_angle (clientID);
		clients[clientID].chase_distance = 0;
		clients[clientID].chase_height = 0;
	}

	VectorCopy(target->s.origin, ownerv);
	ownerv[2] += clients[clients[clientID].target].viewheight;
	VectorCopy(target->client->ps.viewangles, angles);
	angles[YAW] += clients[clientID].chase_angle;
	if (angles[PITCH] > 56)
		angles[PITCH] = 56;
	angles[PITCH] += SHORT2ANGLE (clients[clientID].chase_pitch);
	AngleVectors (angles, forward, right, NULL);
	VectorNormalize (forward);
	VectorMA (ownerv, -(48 + clients[clientID].chase_distance), forward, o);

	if (o[2] < target->s.origin[2] + 20)
		o[2] = target->s.origin[2] + 20;

	// jump animation lifts
	if (!clients[clients[clientID].target].groundentity)
		o[2] += 16;

	o[2] += clients[clientID].chase_height;

	trace = gci.trace (ownerv, vec3_origin, vec3_origin, o, target, MASK_SOLID);
	VectorCopy (trace.endpos, goal);
	if (trace.fraction < 1)
	{
		VectorMA(goal, 2, trace.plane.normal, goal);
	}
	else
	{
		VectorSubtract (o, ownerv, vDiff);
		VectorNormalize (vDiff);
		VectorMA(goal, -2, vDiff, goal);
	}

	// pad for floors and ceilings
	VectorCopy(goal, o);
	o[2] += 6;
	trace = gci.trace(goal, vec3_origin, vec3_origin, o, target, MASK_SOLID);
	if (trace.fraction < 1) 
	{
		VectorCopy(trace.endpos, goal);
		goal[2] -= 6;
	}

	VectorCopy(goal, o);
	o[2] -= 6;
	trace = gci.trace(goal, vec3_origin, vec3_origin, o, target, MASK_SOLID);
	if (trace.fraction < 1) 
	{
		VectorCopy(trace.endpos, goal);
		goal[2] += 6;
	}

	ent->client->ps.pmove.pm_type = PM_FREEZE;

	// set camera angles
	VectorCopy(target->client->ps.viewangles, ent->client->ps.viewangles);
	if (clients[clientID].chase_auto)
	{
		camera_creep_viewangles (clientID);
		ent->client->ps.viewangles[PITCH] = clients[clientID].chase_pitch;
	}
	else 
		ent->client->ps.viewangles[PITCH] += clients[clientID].chase_pitch;

	if (ent->client->ps.viewangles[PITCH] > 85)
		ent->client->ps.viewangles[PITCH] = 85;
	if (ent->client->ps.viewangles[PITCH] < -85)
		ent->client->ps.viewangles[PITCH] = -85;

	ent->client->ps.viewangles[YAW] += clients[clientID].chase_yaw + clients[clientID].chase_angle;

	VectorCopy(ent->client->ps.viewangles, clients[clientID].v_angle);
	VectorCopy(ent->client->ps.viewangles, ent->s.angles);

	// back away from walls
	if (clients[clientID].chase_auto)
	{
		trace_t trace_left, trace_right;

		VectorCopy (goal, o);
		AngleVectors (ent->client->ps.viewangles, NULL, right, NULL);
		VectorNormalize (right);
		VectorMA (o, 3, right, o);
		trace_right = gci.trace (goal, vec3_origin, vec3_origin, o, target, MASK_SOLID);
		VectorMA (o, -6, right, o);
		trace_left  = gci.trace (goal, vec3_origin, vec3_origin, o, target, MASK_SOLID);
		if (trace_right.fraction < 1)
			VectorCopy (trace_left.endpos, goal);
		else if (trace_left.fraction < 1)
			VectorCopy (trace_right.endpos, goal);
	}

	// set camera position
	VectorCopy(goal, ent->s.origin);

	for (i=0 ; i<3 ; i++) 
	{
		ent->client->ps.pmove.origin[i] = (short) goal[i]*8;
		ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(ent->client->ps.viewangles[i] - clients[clientID].cmd_angles[i]);
	}

	clients[clientID].viewheight = 0;
	ent->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;

	if (!clients[clientID].score && !clients[clientID].menu &&
		!clients[clientID].inven && !clients[clientID].help &&
		!clients[clientID].layouts && !intermission &&
		((framenum & 31) == 0 || clients[clientID].update_chase))
	{
		char s[MAX_STRING_CHARS];
		char target1st[MAX_INFO_VALUE];
		char target2nd[MAX_INFO_VALUE];

		target1st[0] = '\0';
		target2nd[0] = '\0';
		strcpy (target1st, ConfigStrings[CS_PLAYERSKINS + clients[clientID].target]);
		char* tp = strstr(target1st, "\\");
		if (tp) *tp = '\0';
		if (clients[clientID].chase_auto && clients[clientID].creep_target)
		{
			strcpy (target2nd, ConfigStrings[CS_PLAYERSKINS + numEdict (clients[clientID].creep_target) - 1]);
			*(strstr (target2nd, "\\")) = '\0';
		} 
		if (target2nd[0])
			sprintf(s, 
			"xv 0 yb -76 %s \"Tracking %s\" xv 0 yb -68 string2 \"Chasing  %s\"",
			(sameTeam (target, clients[clientID].creep_target))? "string2":"string", 
			target2nd, target1st);
		else
			sprintf(s, "xv 0 yb -68 string2 \"Chasing  %s\"", target1st);
		gci.WriteByte (svc_layout);
		gci.WriteString (s);
		gci.unicast(ent, false);
		clients[clientID].update_chase = false;
	}

	SV_CalcBlend (ent);
}


void camera_chase_think (int clientID, usercmd_t *cmd)
{
	int i;
	edict_t *ent;
	pmove_t	pm;

	if (intermission)
		return;

	if (clients[clientID].chase_auto)
		return;

	ent = Edict (clientID + 1);

	if (cmd->buttons & BUTTON_ATTACK && !(clients[clientID].oldbuttons & BUTTON_ATTACK))
	{
		clients[clientID].chase_distance = 0.0F;
		clients[clientID].chase_height = 0.0F;
		clients[clientID].chase_angle = 0.0F;
		clients[clientID].chase_yaw = 0.0F;
		clients[clientID].chase_pitch = 0.0F;
	}
	else
	{
		vec3_t oldangles;

		clients[clientID].chase_distance -= cmd->forwardmove * (((float)cmd->msec) / 2500.0F);
		if (clients[clientID].chase_distance > 128)
			clients[clientID].chase_distance = 128.0F;
		if (clients[clientID].chase_distance < -128)
			clients[clientID].chase_distance = -128.0F;
		clients[clientID].chase_height += cmd->upmove * (((float)cmd->msec) / 2500.0F);
		if (clients[clientID].chase_height > 128)
			clients[clientID].chase_height = 128.0F;
		if (clients[clientID].chase_height < -128)
			clients[clientID].chase_height = -128.0F;
		clients[clientID].chase_angle = anglemod (clients[clientID].chase_angle + cmd->sidemove * (((float)cmd->msec) / 2500.0F));
		VectorCopy (ent->client->ps.viewangles, oldangles);
		ent->client->ps.pmove.pm_type = PM_SPECTATOR;
		memset (&pm, 0, sizeof(pm));
		ent->client->ps.pmove.gravity = (short) sv_gravity->value;
		pm.s = ent->client->ps.pmove;
		for (i=0 ; i<3 ; i++)
		{
			pm.s.origin[i] = (short) ent->s.origin[i]*8;
			pm.s.velocity[i] = 0;
		}
		pm.snapinitial = true;
		pm.cmd = *cmd;
		pm.cmd.upmove = 0;
		pm.cmd.sidemove = 0;
		pm.cmd.forwardmove = 0;
		pm.trace = PM_trace;
		pm.pointcontents = gci.pointcontents;
		gci.Pmove (&pm);
		VectorCopy (pm.viewangles, ent->s.angles);
		VectorCopy (pm.viewangles, ent->client->ps.viewangles);
		VectorCopy (pm.viewangles, clients[clientID].v_angle);
		clients[clientID].chase_yaw = anglemod (clients[clientID].chase_yaw + ent->client->ps.viewangles[YAW] - oldangles[YAW]);
		clients[clientID].chase_pitch = anglemod (clients[clientID].chase_pitch + ent->client->ps.viewangles[PITCH] - oldangles[PITCH]);
		if (clients[clientID].chase_pitch > 85)
			clients[clientID].chase_pitch = 85;
		if (clients[clientID].chase_pitch < -85)
			clients[clientID].chase_pitch = -85;
		ent->client->ps.pmove.pm_type = PM_FREEZE;
	}

	clients[clientID].oldbuttons = cmd->buttons;
}

