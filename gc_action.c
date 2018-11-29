// GameCam v1.02 - camera proxy game module for Quake II
// Copyright (c) 1998-99, by Avi Rozen (Zung!)
// e-mail: zungbang@telefragged.com

// gc_action.c		action camera mode

// Legal Stuff:

/*
The Client Camera is a product of Paul Jordan, and is available from
the Quake2 Camera homepage at http://home.austin.rr.com/thejordan/q2cam/, 
Or as part of the Eraser Bot at http://impact.frag.com.

This program is a modification of the Quake2 Client Camera, and is
therefore in NO WAY supported by Paul Jordan.

This program MUST NOT be sold in ANY form. If you have paid for 
this product, you should contact Paul Jordan immediately, via
the Quake2 Camera Client homepage.
*/

#include "gamecam.h"

priority_list_t *priority_list = NULL;

// q2cam begin

edict_t *pDeadPlayer = NULL;

void CameraStaticThink(edict_t *ent, usercmd_t *ucmd);


int NumPlayersVisible(edict_t *pViewer)
{
	int iCount=0;
	int i;
	edict_t *pTarget;

	for (i = 0; i < maxclients->value; i++) 
	{
		pTarget = Edict(i + 1);
		if (((clients[i].inuse &&
			clients[i].begin &&
			!clients[i].spectator) ||
			(pTarget->inuse &&
			pTarget->s.modelindex != 0)) &&
			pTarget->client &&
			pTarget->client->ps.pmove.pm_type != PM_SPECTATOR &&
			pTarget->client->ps.pmove.pm_type != PM_FREEZE)
		{
			if (IsVisible(pViewer, pTarget, MAX_VISIBLE_RANGE))
			{
				iCount++;
			}
		}
	}
	return iCount;
}




edict_t *PriorityTarget(edict_t *target, qboolean *override)
{
	int i;
	char *skin;
	priority_list_t *current, *group;
	int priority = 0, modelindex;
	edict_t *favorite = NULL, *potential;
	qboolean match;

	group = priority_list;
	// loop thru priority groups
	while (group)
	{
		// loop thru clients
		for (i = 0; i < maxclients->value; i++)
		{
			if (!clients[i].valid_target)
				continue;
			current = group;
			potential = Edict(i + 1);
			match = QFALSE;
			// match potential target against rules
			while (!match && current && current->priority == priority)
			{
				switch (current->type)
				{
				case CAMERA_TARGET_MODEL:
					skin = strstr (ConfigStrings[CS_PLAYERSKINS + i], "\\") + 1;
					match = (strcmp (skin, current->path) == 0);
					break;

				case CAMERA_TARGET_SHELL:
					match = ((potential->s.effects & current->effects) != 0 &&
						potential->s.renderfx == current->renderfx);
					break;

				case CAMERA_TARGET_GUN:
					modelindex = potential->client->ps.gunindex;
					goto MATCH_TARGET;

				case CAMERA_TARGET_MODEL2:
					modelindex = potential->s.modelindex2;
					goto MATCH_TARGET;

				case CAMERA_TARGET_MODEL3:
					modelindex = potential->s.modelindex3;
					goto MATCH_TARGET;

				case CAMERA_TARGET_MODEL4:
					modelindex = potential->s.modelindex4;
MATCH_TARGET:
					match = (modelindex != 0 && modelindex != 255 && (strcmp (ConfigStrings[CS_MODELS + modelindex], current->path) == 0));
					break;
				}
				current = current->next;
			}
			// update favorite if we have a match
			// (we favor players with higher frag count)
			if (match)
			{
				if (favorite)
				{
					int favoriteID;

					favoriteID = numEdict(favorite) - 1;
					if (clients[favoriteID].num_visible < clients[i].num_visible)
						favorite = potential;
					else if (clients[favoriteID].num_visible == clients[i].num_visible &&
						favorite->client->ps.stats[STAT_FRAGS] < 
						potential->client->ps.stats[STAT_FRAGS])
						favorite = potential;
				} else
					favorite = potential;
			} 
		}
		// found priority target?
		if (favorite)
			break;
		// advance to next priority group
		while (group && group->priority == priority)
			group = group->next;
		priority++;
	}
	if (favorite)
		*override = QTRUE;
	else
		*override = QFALSE;
	return (favorite)? favorite:target;
}


edict_t *PlayerToFollow(int cameraID)
{
	edict_t *pViewer;
	edict_t *pBest=NULL;
	int i, iPlayers, iBestCount=0;

	// manual target selection
	// (if it fails once then target is set to "auto")
	if (clients[cameraID].select)
	{
		pViewer = Edict(clients[cameraID].select);
		if (((clients[clients[cameraID].select - 1].inuse &&
			clients[clients[cameraID].select - 1].begin &&
			!clients[clients[cameraID].select - 1].spectator) ||
			(pViewer->inuse &&
			pViewer->s.modelindex != 0)) &&
			pViewer->client &&
			pViewer->client->ps.pmove.pm_type != PM_SPECTATOR &&
			pViewer->client->ps.pmove.pm_type != PM_FREEZE)
		{
			clients[cameraID].override = QTRUE;
			return pViewer;
		}
		else
		{
			gci.cprintf (Edict (cameraID + 1), PRINT_HIGH, "bad camera target - \"%d\"\ncamera target is AUTO\n", clients[cameraID].select - 1);
			clients[cameraID].select = 0;
		}
	}

	clients[cameraID].override = QFALSE;

	for (i = 0; i < maxclients->value; i++)
	{
		iPlayers = 0;

		// Don't switch to dead people
		pViewer = Edict(i + 1);
		clients[i].valid_target = QFALSE;
		if (((clients[i].inuse &&
			clients[i].begin &&
			!clients[i].spectator) ||
			(pViewer->inuse &&
			pViewer->s.modelindex != 0)) &&
			pViewer->client->ps.pmove.pm_type == PM_NORMAL)
		{
			iPlayers = NumPlayersVisible(pViewer);
			clients[i].num_visible = iPlayers;
			clients[i].valid_target = QTRUE;
			if (iPlayers > iBestCount)
			{
				iBestCount = iPlayers;
				pBest = pViewer;
			}
			else if ((iPlayers != 0) && (iPlayers == iBestCount) )
			{
				if (pBest->client->ps.stats[STAT_FRAGS] <
					pViewer->client->ps.stats[STAT_FRAGS])
				{
					pBest = pViewer;
				}
			}
		}
	}
	if (pBest == NULL || clients[cameraID].no_priority) 
		return pBest;
	return PriorityTarget(pBest, &clients[cameraID].override);
}


void PointCamAtOrigin(edict_t *ent, vec3_t vLocation)
{
	int i, clientID;
	vec3_t vDiff, vAngles;

	clientID = numEdict(ent) - 1;

	VectorSubtract(vLocation, ent->s.origin, vDiff);
	vectoangles(vDiff, vAngles);
	VectorCopy (vAngles, ent->s.angles);
	VectorCopy (vAngles, ent->client->ps.viewangles);
	VectorCopy (vAngles, clients[clientID].v_angle);
	for (i=0 ; i<3 ; i++) 
		ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(clients[clientID].v_angle[i] - clients[clientID].cmd_angles[i]);
}


void PointCamAtTarget(edict_t *ent)
{
	int i, clientID;
	vec3_t vDiff, vAngles;
	float fDifference;

	clientID = numEdict(ent) - 1;

	if (clients[clientID].camera && 
		clients[clientID].bWatchingTheWall &&
		!IsVisible (ent, clients[clientID].pTarget, 0))
		return;

	VectorSubtract(clients[clientID].pTarget->s.origin, ent->s.origin, vDiff);
	vectoangles(vDiff, vAngles);
	ent->s.angles[0] = vAngles[0];
	ent->s.angles[2] = 0;
	fDifference = vAngles[1] - ent->s.angles[1];

	while (fabs(fDifference) > 180)
	{
		if (fDifference > 0)
		{
			fDifference -= 360;
		}
		else
		{
			fDifference += 360;
		}
	}

	if (fabs(fDifference) > clients[clientID].fAngleLag)
	{
		// GC: upto twice the angular velocity when |fDifference| > 20 deg
		if (fDifference > 0)
		{
			ent->s.angles[1] += (fDifference <  20)? clients[clientID].fAngleLag : ((1 + ( fDifference - 20) / 160) * clients[clientID].fAngleLag);
		}
		else
		{
			ent->s.angles[1] -= (fDifference > -20)? clients[clientID].fAngleLag : ((1 + (-fDifference - 20) / 160) * clients[clientID].fAngleLag);
		}
	}
	else
	{
		ent->s.angles[1] = vAngles[1];
	}

	VectorCopy (ent->s.angles, ent->client->ps.viewangles);
	VectorCopy (ent->s.angles, clients[clientID].v_angle);
	for (i=0 ; i<3 ; i++) 
		ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(clients[clientID].v_angle[i] - clients[clientID].cmd_angles[i]);
}


qboolean inSolid (edict_t *ent)
{
	int		contents;
	vec3_t	vieworg;

	VectorAdd (ent->s.origin, ent->client->ps.viewoffset, vieworg);
	contents = gci.pointcontents (vieworg);
	return ((contents & CONTENTS_SOLID) != 0);
}


void FindCamPos (int clientID, float angle, vec3_t vOffsetPosition, vec3_t vCamPos)
{
	vec3_t forward;

	AngleVectors(tv (clients[clientID].pTarget->client->ps.viewangles[PITCH],
		clients[clientID].pTarget->client->ps.viewangles[YAW] + angle,
		clients[clientID].pTarget->client->ps.viewangles[ROLL]), forward, NULL,NULL);
	forward[2] = 0;

	VectorNormalize(forward);

	vCamPos[0] = clients[clientID].pTarget->s.origin[0] +
		(vOffsetPosition[0] * forward[0]);

	vCamPos[1] = clients[clientID].pTarget->s.origin[1] +
		(vOffsetPosition[1] * forward[1]);

	vCamPos[2] = clients[clientID].pTarget->s.origin[2] + 
		vOffsetPosition[2];
}


void RepositionAtTarget (edict_t *ent, vec3_t vOffsetPosition)
{
	int i, clientID;
	vec3_t vDiff;
	vec3_t vCamPos;
	trace_t trace;
	camera_t *camera;
	qboolean snapto = QFALSE; // snapto towards target when jumping to new position

	clientID = numEdict(ent) - 1;

	// select a fixed camera if it exists and can see target
	clients[clientID].bWatchingTheWall = QFALSE;
	if (clients[clientID].fixed && cameras)
	{
		camera = camera_fixed_find (clients[clientID].pTarget);
		// need to switch camera?
		if (camera != clients[clientID].camera) 
		{
			if (clients[clientID].fixed_switch_time < leveltime)
			{
				ticker_update_camera (ent, camera);
				clients[clientID].camera = camera;
				clients[clientID].fixed_switch_time = leveltime + CAMERA_FIXED_SWITCH_TIME; 
			}
			else
				clients[clientID].bWatchingTheWall = QTRUE;
		}
		// use fixed camera?
		if (clients[clientID].camera)
		{
			if (!clients[clientID].bWatchingTheWall)
				camera_fixed_select (clients[clientID].camera, ent, clients[clientID].pTarget);
			return;
		}
	}

	if (gc_set_fov->value)
		set_fov (ent, 90, QFALSE);

	// clear camera location display
	if (!clients[clientID].score && !intermission)
	{
		ent->client->ps.stats[STAT_CAM_OFFSET] = 0;
		ent->client->ps.stats[STAT_CAM_LOCATION] = 0;
	}

	// try to be behind target, but if too close
	// try to be on his right/left/front
	FindCamPos (clientID, 0, vOffsetPosition, vCamPos);
	trace = gci.trace( clients[clientID].pTarget->s.origin, NULL, NULL, vCamPos,
		clients[clientID].pTarget, CONTENTS_SOLID);

	if (trace.fraction < 1)
	{
		VectorSubtract(trace.endpos, clients[clientID].pTarget->s.origin, vDiff);
		if (VectorLength (vDiff) < 48)
		{
			FindCamPos (clientID, 90, vOffsetPosition, vCamPos);
			trace = gci.trace( clients[clientID].pTarget->s.origin, NULL, NULL, vCamPos,
				clients[clientID].pTarget, CONTENTS_SOLID);
			if (trace.fraction < 1)
			{
				VectorSubtract(trace.endpos, clients[clientID].pTarget->s.origin, vDiff);
				if (VectorLength (vDiff) < 48)
				{
					FindCamPos (clientID, -90, vOffsetPosition, vCamPos);
					trace = gci.trace( clients[clientID].pTarget->s.origin, NULL, NULL, vCamPos,
						clients[clientID].pTarget, CONTENTS_SOLID);
					if (trace.fraction < 1)
					{
						VectorSubtract(trace.endpos, clients[clientID].pTarget->s.origin, vDiff);
						if (VectorLength (vDiff) < 48)
						{
							FindCamPos (clientID, 180, vOffsetPosition, vCamPos);
							trace = gci.trace( clients[clientID].pTarget->s.origin, NULL, NULL, vCamPos,
								clients[clientID].pTarget, CONTENTS_SOLID);
						}
					}
				}
			}
		}
		VectorNormalize(vDiff);
		VectorMA(trace.endpos, -8, vDiff, trace.endpos);
		if (trace.plane.normal[2] > 0.8)
			trace.endpos[2] += 4;
		snapto = QTRUE;
	}

	if (fabs(trace.endpos[0]-ent->s.origin[0]) > clients[clientID].fXYLag)
		if (trace.endpos[0] > ent->s.origin[0])
			ent->s.origin[0] += clients[clientID].fXYLag; 
		else
			ent->s.origin[0] -= clients[clientID].fXYLag; 
	else
		ent->s.origin[0] = trace.endpos[0];

	if (fabs(trace.endpos[1]-ent->s.origin[1]) > clients[clientID].fXYLag)
		if (trace.endpos[1] > ent->s.origin[1])
			ent->s.origin[1] += clients[clientID].fXYLag; 
		else
			ent->s.origin[1] -= clients[clientID].fXYLag; 
	else
		ent->s.origin[1] = trace.endpos[1];

	if (fabs(trace.endpos[2]-ent->s.origin[2]) > clients[clientID].fZLag)
		if (trace.endpos[2] > ent->s.origin[2])
			ent->s.origin[2] += clients[clientID].fZLag; 
		else
			ent->s.origin[2] -= clients[clientID].fZLag; 
	else
		ent->s.origin[2] = trace.endpos[2];

	trace = gci.trace( clients[clientID].pTarget->s.origin, NULL, NULL, ent->s.origin,
		clients[clientID].pTarget, CONTENTS_SOLID);

	if (trace.fraction < 1)
	{
		VectorSubtract(trace.endpos, clients[clientID].pTarget->s.origin, vDiff);
		VectorNormalize(vDiff);
		VectorMA(trace.endpos, -8, vDiff, trace.endpos);

		if (trace.plane.normal[2] > 0.8)
			trace.endpos[2] += 4;

		VectorCopy (trace.endpos, ent->s.origin);

		snapto = QTRUE;
	}

	if (snapto)
	{
		VectorSubtract (clients[clientID].pTarget->s.origin, ent->s.origin, vDiff);
		vectoangles (vDiff, ent->client->ps.viewangles);
		VectorCopy (ent->client->ps.viewangles, ent->s.angles);
		VectorCopy (ent->client->ps.viewangles, clients[clientID].v_angle);
		for (i=0 ; i<3 ; i++) 
			ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(clients[clientID].v_angle[i] - clients[clientID].cmd_angles[i]);
	}

	for (i=0 ; i<3 ; i++) 
		ent->client->ps.pmove.origin[i] = (short) ent->s.origin[i]*8;
}


void RepositionAtOrigin(edict_t *ent, vec3_t vOffsetPosition)
{
	int i, clientID;
	vec3_t vCamPos;
	trace_t trace;

	clientID = numEdict(ent) - 1;

	clients[clientID].camera = NULL;
	clients[clientID].fixed_switch_time = 0;

	vCamPos[0] = vOffsetPosition[0] + 40;
	vCamPos[1] = vOffsetPosition[1] + 40;
	vCamPos[2] = vOffsetPosition[2] + 30;

	trace = gci.trace( vOffsetPosition, NULL, NULL, vCamPos,
		clients[clientID].pTarget, CONTENTS_SOLID);

	if (trace.fraction < 1)
	{
		vec3_t vDiff;

		VectorSubtract(trace.endpos, vOffsetPosition, vDiff);
		VectorNormalize(vDiff);
		VectorMA(trace.endpos, -8, vDiff, trace.endpos);

		if (trace.plane.normal[2] > 0.8)
			trace.endpos[2] += 4;
	}

	if (fabs(trace.endpos[0]-ent->s.origin[0]) > clients[clientID].fXYLag)
	{
		if (trace.endpos[0] > ent->s.origin[0])
		{
			ent->s.origin[0] += clients[clientID].fXYLag; 
		}
		else
		{
			ent->s.origin[0] -= clients[clientID].fXYLag; 
		}
	}
	else
	{
		ent->s.origin[0] = trace.endpos[0];
	}

	if (fabs(trace.endpos[1]-ent->s.origin[1]) > clients[clientID].fXYLag)
	{
		if (trace.endpos[1] > ent->s.origin[1])
		{
			ent->s.origin[1] += clients[clientID].fXYLag; 
		}
		else
		{
			ent->s.origin[1] -= clients[clientID].fXYLag; 
		}
	}
	else
	{
		ent->s.origin[1] = trace.endpos[1];
	}

	if (fabs(trace.endpos[2]-ent->s.origin[2]) > clients[clientID].fZLag)
	{
		if (trace.endpos[2] > ent->s.origin[2])
		{
			ent->s.origin[2] += clients[clientID].fZLag; 
		}
		else
		{
			ent->s.origin[2] -= clients[clientID].fZLag; 
		}
	}
	else
	{
		ent->s.origin[2] = trace.endpos[2];
	}

	trace = gci.trace( vOffsetPosition, NULL, NULL, ent->s.origin,
		clients[clientID].pTarget, CONTENTS_SOLID);

	if (trace.fraction < 1)
	{
		vec3_t vDiff;

		VectorSubtract(trace.endpos, vOffsetPosition, vDiff);
		VectorNormalize(vDiff);
		VectorMA(trace.endpos, -8, vDiff, trace.endpos);

		if (trace.plane.normal[2] > 0.8)
			trace.endpos[2] += 4;

		//VectorCopy(trace.endpos, ent->s.origin);
	}

	if (trace.fraction != 1)
	{
		VectorCopy(trace.endpos, ent->s.origin);
	}

	for (i=0 ; i<3 ; i++) 
		ent->client->ps.pmove.origin[i] = (short) ent->s.origin[i]*8;
}


void CameraFollowThink(edict_t *ent, usercmd_t *ucmd)
{
	int clientID;
	vec3_t vCameraOffset;

	clientID = numEdict(ent) - 1;

	if (clients[clientID].pTarget || (clients[clientID].pTarget = PlayerToFollow(clientID)) != NULL)
	{
		// just keep looking for action!
		vCameraOffset[0] = -60;
		vCameraOffset[1] = -60;
		vCameraOffset[2] = 40;
		RepositionAtTarget(ent, vCameraOffset);
		PointCamAtTarget(ent);
	}
}


void SwitchToNewTarget (int clientID, edict_t *pNewTarget)
{
	if (clients[clientID].pTarget == NULL)
	{
		clients[clientID].pTarget = pNewTarget;
		clients[clientID].last_switch_time = leveltime + CAMERA_MIN_SWITCH_TIME;
		clients[clientID].fixed_switch_time = -1.0F; // force fixed camera switch
	}
	else if (clients[clientID].pTarget != pNewTarget)
	{
		if (clients[clientID].last_switch_time < leveltime)
		{
			clients[clientID].pTarget = pNewTarget;
			clients[clientID].last_switch_time = leveltime + CAMERA_MIN_SWITCH_TIME;
			clients[clientID].fixed_switch_time = -1.0F; // force fixed camera switch
		}
	}
	if (clients[clientID].pTarget == NULL)
		clients[clientID].last_switch_time = 0;
}


void CameraNormalThink(edict_t *ent, usercmd_t *ucmd)
{
	int clientID;
	vec3_t vCameraOffset;
	int iNumVis;
	edict_t *pNewTarget;

	clientID = numEdict(ent) - 1;

	iNumVis = NumPlayersVisible (ent);
	pNewTarget =  PlayerToFollow (clientID);

	// only watch the dead if it's the one we followed
	if (!clients[clientID].bWatchingTheDead && clients[clientID].pTarget && clients[numEdict(clients[clientID].pTarget) - 1].last_pmtype == PM_DEAD)
	{
		clients[clientID].bWatchingTheDead = QTRUE;
		//clients[clientID].pTarget = pDeadPlayer;
		clients[clientID].last_move_time = leveltime + CAMERA_DEAD_SWITCH_TIME;
		PointCamAtTarget(ent);
	}
	else if (clients[clientID].bWatchingTheDead)
	{
		if (clients[clientID].last_move_time < leveltime || inSolid (ent))
		{
			clients[clientID].bWatchingTheDead = QFALSE;
		}
		else 
		{
			if (clients[numEdict(clients[clientID].pTarget) - 1].last_pmtype == PM_DEAD)
			{
				VectorCopy(clients[clientID].pTarget->s.origin, clients[clientID].vDeadOrigin);
			}
			PointCamAtOrigin(ent, clients[clientID].vDeadOrigin);
			RepositionAtOrigin(ent, clients[clientID].vDeadOrigin);
		}
	}
	else if ( iNumVis < 2 )
	{
		vCameraOffset[0] = -60;
		vCameraOffset[1] = -60;
		vCameraOffset[2] = 40;

		if (clients[clientID].last_move_time >= leveltime) 
		{
			edict_t *pClosestTarget;

			if ((pNewTarget != NULL) && (clients[clientID].override || (NumPlayersVisible (pNewTarget) > 1)))
			{
				SwitchToNewTarget (clientID, pNewTarget);
				RepositionAtTarget(ent, vCameraOffset);
				PointCamAtTarget(ent);
			}
			else if ((pClosestTarget = ClosestVisible(ent, MAX_VISIBLE_RANGE, QFALSE)) != NULL)
			{
				SwitchToNewTarget (clientID, pClosestTarget);
				RepositionAtTarget(ent, vCameraOffset);
				PointCamAtTarget(ent);
			}
			else if (pNewTarget != NULL)
			{
				// look for someone new!
				SwitchToNewTarget (clientID, pNewTarget);
				RepositionAtTarget(ent, vCameraOffset);
				PointCamAtTarget(ent);
				clients[clientID].last_move_time = 0;
			}
		}
		else if (pNewTarget != NULL)
		{
			// just keep looking for action!
			vCameraOffset[0] = -60;
			vCameraOffset[1] = -60;
			vCameraOffset[2] = 40;
			SwitchToNewTarget (clientID, pNewTarget);
			RepositionAtTarget(ent, vCameraOffset);
			PointCamAtTarget(ent);
		}
	}
	// if we are done during a battle.
	else if (clients[clientID].last_move_time < leveltime ||
		(clients[clientID].pTarget && 
		!gci.inPVS(ent->s.origin, clients[clientID].pTarget->s.origin)) ||
		(clients[clientID].pTarget && inSolid (ent)))
	{
		if (pNewTarget != NULL)
		{
			vCameraOffset[0] = -60;
			vCameraOffset[1] = -60;
			vCameraOffset[2] = 80;
			clients[clientID].pTarget = NULL;
			SwitchToNewTarget (clientID, pNewTarget);
			RepositionAtTarget(ent, vCameraOffset);
			PointCamAtTarget(ent);
			clients[clientID].last_move_time = leveltime + CAMERA_SWITCH_TIME;
		}
	}
	else if (clients[clientID].pTarget != NULL)
	{
		if (IsVisible (ent, clients[clientID].pTarget, 0))
		{
			if (!clients[clientID].camera) // reposition camera if too close or too far
			{
				float distance;
				vec3_t vDiff;

				VectorSubtract (ent->s.origin, clients[clientID].pTarget->s.origin, vDiff);
				distance = VectorLength (vDiff);
				if (distance < CAMERA_MIN_RANGE || distance > CAMERA_MAX_RANGE)
					RepositionAtTarget(ent, tv (-60, -60, 80));
			}
			PointCamAtTarget(ent);
		}
		else
			clients[clientID].last_move_time = 0;
	}

	pDeadPlayer = NULL;
}


void CameraStaticThink(edict_t *ent, usercmd_t *ucmd)
{
	int i, clientID;
	trace_t trace;
	vec3_t vEndFloor, vEndCeiling;

	clientID = numEdict(ent) - 1;

	vEndFloor[0] = ent->s.origin[0];
	vEndFloor[1] = ent->s.origin[1];
	vEndFloor[2] = ent->s.origin[2] - 40000;
	trace = gci.trace(ent->s.origin, NULL, NULL, vEndFloor, ent, CONTENTS_SOLID);

	VectorCopy (trace.endpos, vEndFloor );

	vEndCeiling[0] = vEndFloor[0];
	vEndCeiling[1] = vEndFloor[1];
	vEndCeiling[2] = vEndFloor[2] + 175;
	trace = gci.trace(vEndFloor, NULL, NULL, vEndCeiling, ent, CONTENTS_SOLID);

	VectorCopy (trace.endpos, ent->s.origin);

	while (inSolid(ent)) ent->s.origin[2] -= 1;

	for (i=0 ; i<3 ; i++) 
		ent->client->ps.pmove.origin[i] = (short) ent->s.origin[i]*8;

	if (clients[clientID].last_move_time < leveltime )
	{
		clients[clientID].last_move_time = leveltime + 2;
		ent->s.angles[0] = 45;
		ent->s.angles[1] = 0;
		ent->s.angles[2] = 0;
		VectorCopy (ent->s.angles, ent->client->ps.viewangles);
		VectorCopy (ent->s.angles, clients[clientID].v_angle);
		for (i=0 ; i<3 ; i++) 
			ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(clients[clientID].v_angle[i] - clients[clientID].cmd_angles[i]);
	}
}


// q2cam end


void camera_action_setup (int clientID)
{
	int i;
	edict_t *ent;

	if (!clients[clientID].begin)
		return;

	// clear all effects and layouts
	ent = Edict(clientID+1);
	ent->client->ps.gunindex = 0;
	VectorClear (ent->client->ps.kick_angles);
	//memset(ent->client->ps.stats, 0, MAX_STATS * sizeof(short));
	ent->s.effects = 0;
	ent->s.renderfx = 0;
	ent->s.sound = 0;
	ent->s.event = 0;
	// reset fov
	if (gc_set_fov->value)
		set_fov (ent, 90, QFALSE);

	//clients[clientID].iMode = CAM_NORMAL_MODE;
	clients[clientID].bWatchingTheDead = QFALSE;
	clients[clientID].bWatchingTheWall = QFALSE;
	clients[clientID].fXYLag = DAMP_VALUE_XY; 
	clients[clientID].fZLag = DAMP_VALUE_Z;
	clients[clientID].fAngleLag = DAMP_ANGLE_Y; 
	clients[clientID].last_move_time = 0;
	clients[clientID].last_switch_time = 0;
	clients[clientID].fixed_switch_time = 0;
	clients[clientID].pTarget = NULL;
	clients[clientID].camera = NULL;

	clients[clientID].oldbuttons = 0;

	//VectorClear (ent->s.angles);
	//VectorCopy (ent->s.angles, ent->client->ps.viewangles);
	//VectorCopy (ent->s.angles, clients[clientID].v_angle);
	VectorClear (ent->client->ps.viewoffset);

	ticker_clear (ent);

	// avoid solid areas
	if (inSolid (ent)) 
	{
		VectorCopy(spawn_origin, ent->s.origin);
		for (i=0 ; i<3 ; i++) 
			ent->client->ps.pmove.origin[i] = (short) spawn_origin[i]*8;
	}

	if (priority_list == NULL) 
		clients[clientID].no_priority = QTRUE;
}


void camera_action_wrapup (int clientID)
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

	clients[clientID].oldbuttons = 0;

	ent->client->ps.stats[STAT_LAYOUTS] = 0;
}


void camera_action_begin (int clientID)
{
	edict_t *ent;

	ent = Edict(clientID + 1);

	ticker_clear (ent);

	clients[clientID].inven = QFALSE;
	clients[clientID].score = QFALSE;
	clients[clientID].help = QFALSE;
	clients[clientID].menu = QFALSE;
	clients[clientID].layouts = QFALSE;
	clients[clientID].statusbar_removed = QFALSE;
	clients[clientID].bWatchingTheDead = QFALSE;
	clients[clientID].bWatchingTheWall = QFALSE;
	clients[clientID].pTarget = NULL;
	clients[clientID].camera = NULL;
	clients[clientID].last_switch_time = 0;
	clients[clientID].fixed_switch_time = 0;

	clients[clientID].oldbuttons = 0;

	// force re-think
	clients[clientID].last_move_time = leveltime;

	// reset fov
	if (gc_set_fov->value)
		set_fov (ent, 90, QFALSE);
}


void camera_action_add_target (int other)
{
}


void camera_action_remove_target (int other)
{
	int i;
	edict_t *pTarget;

	pTarget = Edict(other + 1);

	// force rethink on all cameras
	for (i = 0; i < maxclients->value; i++)
		if (clients[i].inuse &&
			clients[i].spectator &&
			clients[i].mode == CAMERA_ACTION) 
		{
			clients[i].last_move_time = leveltime;
			if (clients[i].pTarget == pTarget)
				clients[i].pTarget = NULL;
		}

}


void camera_action_frame (int clientID)
{
	edict_t *ent;

	ent = Edict(clientID + 1);

	// move towards target if inside solid
	if (clients[clientID].pTarget && !clients[clientID].bWatchingTheDead && inSolid (ent))
		RepositionAtTarget (ent, tv (-60, -60, 40));
	// update viewing effects (water, lava, slime)
	SV_CalcBlend (ent);
	// update client id view
	if (!clients[clientID].score && !intermission)
	{
		ent->client->ps.stats[STAT_ID_VIEW] = 0;
		if (clients[clientID].id)
			SetIDView(ent, QFALSE); // QFALSE means that we id bots too
	}
}


void camera_action_think (int clientID, usercmd_t *cmd)
{
	edict_t *ent;

	ent = Edict(clientID + 1);

	ent->client->ps.pmove.pm_type = PM_FREEZE;
	ent->client->ps.pmove.gravity = 0;

	if (numPlayers() == 0)
	{
		CameraStaticThink(ent, cmd);
	}
	else
	{
		// keep tracking the same player until a new player is selected (with the attack key)
		if (clients[clientID].iMode == CAM_FOLLOW_MODE &&
			clients[clientID].pTarget && 
			(cmd->buttons & BUTTON_ATTACK) && 
			!(clients[clientID].oldbuttons & BUTTON_ATTACK))
		{
			int target, next;
			qboolean found = QFALSE;
			edict_t *pTarget = NULL;

			target = numEdict (clients[clientID].pTarget) - 1;
			for (next = target + 1; next < (int) maxclients->value && !found; next++)
			{
				pTarget = Edict(next + 1);
				found = ((clients[next].inuse && clients[next].begin && !clients[next].spectator) ||
					(pTarget->inuse && pTarget->s.modelindex != 0)) &&
					pTarget->client->ps.pmove.pm_type == PM_NORMAL;
			}
			if (!found)
				for (next = 0; next < target && !found; next++)
				{
					pTarget = Edict(next + 1);
					found = ((clients[next].inuse && clients[next].begin && !clients[next].spectator) ||
						(pTarget->inuse && pTarget->s.modelindex != 0)) &&
						pTarget->client->ps.pmove.pm_type != PM_NORMAL;
				}

				if (found)
					clients[clientID].pTarget = pTarget;
		}

		clients[clientID].oldbuttons = cmd->buttons;


		switch (clients[clientID].iMode)
		{
		case CAM_FOLLOW_MODE:
			CameraFollowThink(ent, cmd);
			break;
		case CAM_NORMAL_MODE:
		default:
			CameraNormalThink(ent, cmd);
			break;

		}
	}
}


void FreePriorityList(void)
{
	priority_list_t *current;
	priority_list_t *del;

	current = priority_list;
	while (current)
	{
		del = current;
		current = current->next;
		gci.TagFree (del);
	}
	priority_list = NULL;
}


qboolean ParsePriorityList (void)
{
	FILE *rfp;
	unsigned int i, j, linenum = 0;
	int	priority = 0;
	char line[MAX_STRING_CHARS], path[MAX_OSPATH];
	qboolean prev_blank = QTRUE;
	priority_list_t current;
	priority_list_t *last = NULL;

	if (strlen (game->string))
		strcat (strcat (strcat (strcpy (path, basedir->string), "/"), game->string),"/gamecam.ini");
	else
		strcat (strcpy (path, basedir->string), "/baseq2/gamecam.ini");

	rfp = fopen (path, "rt");
	if (rfp == NULL)
		return QFALSE;
	gci.dprintf ("reading \"%s\" ... ", path);
	while (fgets (line, MAX_STRING_CHARS, rfp) != NULL)
	{
		linenum++;
		// skip white space
		for (i = 0; i < strlen(line) && line[i] && line[i] <= ' '; i++);
		// blank line?
		if (i == strlen(line) || !line[i]) 
		{
			if (!prev_blank)
				priority++;
			if (priority != 0)
				prev_blank = QTRUE;
			continue;
		}
		current.priority = priority;
		current.effects = 0;
		current.renderfx = 0;
		current.next = NULL;
		// remark?
		if (!strncmp (&line[i], "//", 2))
			continue;
		prev_blank = QFALSE;
		// valid type?
		if (!Q_strncasecmp (&line[i], "model2", 6)) 
		{
			i += 6;
			current.type = CAMERA_TARGET_MODEL2;
		} 
		else if (!Q_strncasecmp (&line[i], "model3", 6)) 
		{
			i += 6;
			current.type = CAMERA_TARGET_MODEL3;
		} 
		else if (!Q_strncasecmp (&line[i], "model4", 6)) 
		{
			i += 6;
			current.type = CAMERA_TARGET_MODEL4;
		}
		else if (!Q_strncasecmp (&line[i], "model", 5)) 
		{
			i += 5;
			current.type = CAMERA_TARGET_MODEL;
		}
		else if (!Q_strncasecmp (&line[i], "gun", 3)) 
		{
			i += 3;
			current.type = CAMERA_TARGET_GUN;
		}
		else if (!Q_strncasecmp (&line[i], "shell", 5)) 
		{
			i += 5;
			current.type = CAMERA_TARGET_SHELL;
		}
		else 
			goto ERR_BADTARGET;
		if (line[i] > ' ')
			goto ERR_BADTARGET;
		// skip white space
		for (; i < strlen(line) && line[i] && line[i] <= ' '; i++);
		if (i == strlen(line) || !line[i]) 
			goto ERR_BADTARGET;
		// valid value?
		for (j = 0; i+j < strlen(line) && line[i+j] > ' ' && !(line[i+j]=='/' && line[i+j-1]=='/'); j++); 
		if (line[i+j] == '/')
			j--;
		if (j >= MAX_QPATH)
			goto ERR_BADTARGET;
		strncpy (current.path, &line[i], j);
		current.path[j] = '\0';
		switch (current.type) 
		{
		case CAMERA_TARGET_GUN:
		case CAMERA_TARGET_MODEL2:
		case CAMERA_TARGET_MODEL3:
		case CAMERA_TARGET_MODEL4:
			if (strlen (current.path) < 5)
				goto ERR_BADTARGET;
			break;
		case CAMERA_TARGET_MODEL:
			if (strchr (current.path, '/') == NULL)
				goto ERR_BADTARGET;
			break;
		case CAMERA_TARGET_SHELL:
			if (sscanf (current.path, "%d:%d", &current.effects, &current.renderfx) != 2)
				goto ERR_BADTARGET;
			break;
		}
		// skip white space
		for (i += j; i < strlen(line) && line[i] && line[i] <= ' '; i++);
		// remark?
		if (i < strlen(line) && strncmp (&line[i], "//", 2))
			goto ERR_BADTARGET;
		// allocate memory for current entry
		if (priority_list == NULL)
			last = priority_list = gci.TagMalloc (sizeof(priority_list_t), TAG_GAME);
		else 
		{
			last->next = gci.TagMalloc (sizeof(priority_list_t), TAG_GAME);
			last = last->next;
		}
		// save current entry
		memcpy (last, &current, sizeof(priority_list_t));
	}

	fclose (rfp);
	gci.dprintf ("ok\n");
	return QTRUE;

ERR_BADTARGET:
	fclose (rfp);
	FreePriorityList();
	gci.dprintf ("failed\n");
	gci.dprintf ("ParsePriorityList: bad target in gamecam.ini (line %d)\n", linenum);
	return QFALSE;
}
