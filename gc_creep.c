// GameCam v1.02 - camera proxy game module for Quake II
// Copyright (c) 1998-99, by Avi Rozen (Zung!)
// e-mail: zungbang@telefragged.com

// gc_creep.c		creep camera functions

#include "gamecam.h"

#define CREEP_ANGLE_STEP		20.0F
#define CREEP_VIEWANGLE_STEP	12.0F
#define CREEP_NOMINAL_DISTANCE	48.0F
#define CREEP_FOV_FACTOR		1.25F

edict_t* camera_creep_target(int clientID)
{
	int i;
	edict_t* pTarget1st, * pTarget2nd;
	edict_t* pBest1 = NULL;
	edict_t* pBest2 = NULL;
	vec3_t vDistance = { 0 };
	float fCurrent, fClosest1 = -1.0F, fClosest2 = -1.0F;

	pTarget1st = Edict(clients[clientID].target + 1);

	for (i = 0; i < maxclients->value; i++)
	{
		pTarget2nd = Edict(i + 1);
		if (pTarget1st != pTarget2nd &&
			((clients[i].inuse &&
				clients[i].begin &&
				!clients[i].spectator) ||
				(pTarget2nd->inuse &&
					pTarget2nd->s.modelindex != 0)) &&
			pTarget2nd->client &&
			pTarget2nd->client->ps.pmove.pm_type != PM_SPECTATOR &&
			pTarget2nd->client->ps.pmove.pm_type != PM_FREEZE &&
			clients[i].last_pmtype != PM_DEAD &&
			clients[i].last_pmtype != PM_GIB &&
			gci.inPVS(pTarget1st->s.origin, pTarget2nd->s.origin))
		{
			VectorSubtract(pTarget1st->s.origin, pTarget2nd->s.origin, vDistance);
			fCurrent = VectorLength(vDistance);
			if (sameTeam(pTarget1st, pTarget2nd))
			{
				if (fClosest1 < 0 || fCurrent < fClosest1)
				{
					pBest1 = pTarget2nd;
					fClosest1 = fCurrent;
				}
			}
			else
			{
				if (fClosest2 < 0 || fCurrent < fClosest2)
				{
					pBest2 = pTarget2nd;
					fClosest2 = fCurrent;
				}
			}
		}
	}
	return ((pBest2) ? pBest2 : pBest1);
}


void camera_creep_angle(int clientID)
{
	//edict_t *camera;
	edict_t* target;
	edict_t* player;
	vec3_t player2target, vDiff = { 0 };
	float best_angle, chase_diff, target_distance, target_yaw;
	float gamma, best_angle1, best_angle2, chase_diff1, chase_diff2;

	//camera = Edict (clientID + 1);
	target = clients[clientID].creep_target;
	player = Edict(clients[clientID].target + 1);

	// reset on intermission
	if (intermission)
	{
		clients[clientID].chase_angle = 0;
		return;
	}

	// no 2nd target
	if (target == NULL) // return to chase_angle == 0
	{
		if ((fabs(clients[clientID].chase_angle) < CREEP_ANGLE_STEP))
			clients[clientID].chase_angle = 0;
		else
			if (clients[clientID].chase_angle > 0)
				clients[clientID].chase_angle = anglemod(clients[clientID].chase_angle - CREEP_ANGLE_STEP);
			else
				clients[clientID].chase_angle = anglemod(clients[clientID].chase_angle + CREEP_ANGLE_STEP);
		return;
	}

	VectorSubtract(target->s.origin, player->s.origin, vDiff);
	target_distance = VectorLength(vDiff);
	vectoangles(vDiff, player2target);
	target_yaw = anglemod(player2target[YAW] - player->client->ps.viewangles[YAW]);
	if (target_distance > CREEP_NOMINAL_DISTANCE)
		gamma = CREEP_FOV_FACTOR * ((float)acos(CREEP_NOMINAL_DISTANCE / target_distance)) * 180.0F / M_PI;
	else
		gamma = CREEP_FOV_FACTOR * (90.0F - (((float)asin(0.5 * target_distance / CREEP_NOMINAL_DISTANCE)) * 180.0F / M_PI));
	best_angle1 = anglemod(target_yaw - (180 - gamma));
	best_angle2 = anglemod(target_yaw + (180 - gamma));
	chase_diff1 = anglediff(clients[clientID].chase_angle, best_angle1);
	chase_diff2 = anglediff(clients[clientID].chase_angle, best_angle2);

	if (fabs(chase_diff1) < fabs(chase_diff2))
	{
		chase_diff = chase_diff1;
		best_angle = best_angle1;
	}
	else
	{
		chase_diff = chase_diff2;
		best_angle = best_angle2;
	}

	// chase_diff is used to determine the direction to move to
	// in order to decrease/increase yaw separation
	if (fabs(chase_diff) < CREEP_ANGLE_STEP)
		clients[clientID].chase_angle = best_angle;
	else
		if (chase_diff > 0)
			clients[clientID].chase_angle = anglemod(clients[clientID].chase_angle - CREEP_ANGLE_STEP);
		else
			clients[clientID].chase_angle = anglemod(clients[clientID].chase_angle + CREEP_ANGLE_STEP);
	return;
}


void camera_creep_viewangles(int clientID)
{
	edict_t* ent;
	float desired_yaw, desired_pitch, yaw_diff, pitch_diff;

	ent = Edict(clientID + 1);

	// reset on intermission
	if (intermission)
	{
		clients[clientID].chase_yaw = 0;
		clients[clientID].chase_pitch = 0;
		return;
	}
	// determine desired yaw and pitch
	if (clients[clientID].creep_target)
	{
		vec3_t vDiff = { 0 }, target_angles;

		VectorSubtract(clients[clientID].creep_target->s.origin, ent->s.origin, vDiff);
		vectoangles(vDiff, target_angles);
		desired_yaw = anglemod(target_angles[YAW] - (clients[clientID].chase_angle + ent->client->ps.viewangles[YAW])) / 2;
		if (desired_yaw > 45)
			desired_yaw = 45;
		if (desired_yaw < -45)
			desired_yaw = -45;
		desired_pitch = anglemod(target_angles[PITCH] - ent->client->ps.viewangles[PITCH]) / 2;
		if (desired_pitch > 45)
			desired_pitch = 45;
		if (desired_pitch < -45)
			desired_pitch = -45;
	}
	else
	{
		desired_yaw = 0;
		desired_pitch = 0;
	}

	// move yaw
	yaw_diff = anglemod(clients[clientID].chase_yaw - desired_yaw);
	if (fabs(yaw_diff) < CREEP_VIEWANGLE_STEP)
		clients[clientID].chase_yaw = desired_yaw;
	else
		if (yaw_diff > 0)
			clients[clientID].chase_yaw = anglemod(clients[clientID].chase_yaw - CREEP_VIEWANGLE_STEP);
		else
			clients[clientID].chase_yaw = anglemod(clients[clientID].chase_yaw + CREEP_VIEWANGLE_STEP);

	// move pitch
	pitch_diff = anglemod(clients[clientID].chase_pitch - desired_pitch);
	if (fabs(pitch_diff) < CREEP_VIEWANGLE_STEP)
		clients[clientID].chase_pitch = desired_pitch;
	else
		if (pitch_diff > 0)
			clients[clientID].chase_pitch = anglemod(clients[clientID].chase_pitch - CREEP_VIEWANGLE_STEP);
		else
			clients[clientID].chase_pitch = anglemod(clients[clientID].chase_pitch + CREEP_VIEWANGLE_STEP);
}

