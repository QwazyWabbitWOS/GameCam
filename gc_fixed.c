// GameCam v1.02 - camera proxy game module for Quake II
// Copyright (c) 1998-99, by Avi Rozen (Zung!)
// e-mail: zungbang@telefragged.com

// gc_fixed.c		fixed cameras routines

#include "gamecam.h"

char current_map[MAX_INFO_VALUE];
camera_t *cameras = NULL;


void camera_fixed_free (void)
{
	camera_t *current_camera;

	current_camera = cameras;
	if (cameras == NULL)
		return;
	cameras->prev->next = NULL;
	while (current_camera)
	{
		cameras = cameras->next;
		gci.TagFree (current_camera);
		current_camera = cameras;
	}
}


qboolean camera_fixed_read_camera (FILE *camfile, char *name, vec3_t origin, vec3_t angles, float *fov)
{
	qboolean done;
	char line[MAX_STRING_CHARS];
	char classname[MAX_INFO_VALUE] = { 0 };

	name[0] = '\0';
	VectorClear (origin);
	VectorClear (angles);
	*fov = 0.0F;

	done = false;
	while (!done && fgets (line, MAX_STRING_CHARS, camfile))
	{
		if (blank_or_remark (line))
			continue;
		if (!trimcmp (line, "{"))
		{
			gci.dprintf ("camera_fixed_read_camera: found \"%s\" when expecting \"{\"\n", line);
			return false;
		}
		done = true;
	}
	if (!done)
		return false;
	done = false;
	while (!done && fgets (line, MAX_STRING_CHARS, camfile))
	{
		done = trimcmp (line, "}");
		if (!done)
		{
			float x, y, z;

			if (blank_or_remark (line))
				continue;
			if (sscanf (line, " \"classname\" \"%[^\"]\" ", classname) == 1 && strcmp (classname, "info_camera") == 0)
				continue;
			if (sscanf (line, " \"origin\" \"%f %f %f\" ", &x, &y, &z) == 3)
			{
				VectorSet (origin, x, y, z);
				continue;
			}
			if (sscanf (line, " \"angles\" \"%f %f %f\" ", &x, &y, &z) == 3)
			{
				VectorSet (angles, x, y, z);
				continue;
			}
			if (sscanf (line, " \"fov\" \"%f\" ", &x) == 1)
			{
				*fov = x;
				continue;
			}
			if (sscanf (line, " \"name\" \"%[^\"]\" ", name) == 1)
				continue;
			gci.dprintf ("camera_fixed_read_camera: syntax error - \"%s\"", line);
			return false;
		}
	}
	if (!done)
	{
		gci.dprintf ("camera_fixed_read_camera: unexpected EOF\n");
		return false;
	}
	return true;
}


void camera_fixed_load (char *mapname)
{
	char cameraPath[MAX_OSPATH];
	FILE *camfile = NULL;
	camera_t* camera;
	camera_t* current_camera = cameras;
	int cam_count = 0;
	vec3_t origin, angles;
	float fov;
	char name[MAX_INFO_VALUE];

	if (strstr (mapname, ".")) // this is not a map
		return;


	if (game->string[0] == '\0')
	{
		sprintf (cameraPath, "%s/baseq2/cameras/%s.cam", basedir->string, mapname);
		camfile = fopen (cameraPath, "rt");
	}
	else
	{
		sprintf (cameraPath, "%s/%s/cameras/%s.cam", basedir->string, game->string, mapname);
		camfile = fopen (cameraPath, "rt");
		if (camfile == NULL)
		{
			sprintf (cameraPath, "%s/baseq2/cameras/%s.cam", basedir->string, mapname);
			camfile = fopen (cameraPath, "rt");
		}
	}

	if (camfile == NULL)
		return;

	while (camera_fixed_read_camera (camfile, name, origin, angles, &fov))
	{
		cam_count++;
		camera = gci.TagMalloc (sizeof (camera_t), TAG_GAME);
		if (camera)
		{
			strcpy(camera->name, name);
			VectorCopy(origin, camera->origin);
			VectorCopy(angles, camera->angles);
			camera->fov = fov;
			if (cameras)
			{
				camera->next = cameras;
				camera->prev = current_camera;
				current_camera->next = camera;
				current_camera = camera;
			}
			else
			{
				cameras = camera;
				cameras->next = cameras;
				cameras->prev = cameras;
				current_camera = cameras;
			}
		}
	}
	if (cameras)
		cameras->prev = current_camera;
	fclose (camfile);
	gci.dprintf ("GameCam: loaded %d cameras\n", cam_count);
}


// camera save
void camera_fixed_save (char *mapname)
{
	char cameraPath[MAX_OSPATH];
	FILE *camfile;
	camera_t *current_camera;

	if (cameras == NULL || strstr (mapname, ".")) // this is not a map
	{
		gci.dprintf ("GameCam: cameras not defined\n");
		return;
	}

#ifdef _WIN32
	// create the folder 
	sprintf (cameraPath, 
		"%s/%s/cameras", 
		basedir->string, 
		(strlen (game->string))? game->string:"baseq2");
	CreateDirectory (cameraPath, NULL);
#endif
	sprintf (cameraPath, 
		"%s/%s/cameras/%s.cam", 
		basedir->string, 
		(strlen (game->string))? game->string:"baseq2",
		mapname);

	camfile = fopen (cameraPath, "wt");
	if (camfile == NULL)
	{
		gci.dprintf ("GameCam: camera_fixed_save: can't write to file - \"%s\"\n", cameraPath);
		return;
	}
	current_camera = cameras;
	do
	{
		fprintf (camfile, "{\n"
			"\"classname\" \"info_camera\"\n"
			"\"name\" \"%s\"\n"
			"\"origin\" \"%d %d %d\"\n"
			"\"angles\" \"%d %d %d\"\n"
			"\"fov\" \"%d\"\n"
			"}\n",
			current_camera->name, 
			(int) current_camera->origin[0], 
			(int) current_camera->origin[1], 
			(int) current_camera->origin[2], 
			(int) current_camera->angles[PITCH],
			(int) current_camera->angles[YAW],
			(int) current_camera->angles[ROLL],
			(int) current_camera->fov);
		current_camera = current_camera->next;
	}
	while (current_camera != cameras);
	fclose (camfile);
	gci.dprintf ("GameCam: cameras saved to \"%s\"\n", cameraPath);
}


// camera add [name]
camera_t *camera_fixed_add (edict_t *ent, char *name)
{
	int clientID;
	camera_t *camera, *current_camera;

	clientID = numEdict (ent) - 1;

	camera = gci.TagMalloc (sizeof (camera_t), TAG_GAME);
	strcpy (camera->name, name);
	if (camera->name[0] == '\0')
		strcpy (camera->name, " ");
	VectorCopy (ent->s.origin, camera->origin);
	VectorCopy (ent->s.angles, camera->angles);
	camera->fov = ent->client->ps.fov;
	current_camera = (clients[clientID].camera)? clients[clientID].camera:cameras;
	if (current_camera)
	{
		camera->prev = current_camera;
		camera->next = current_camera->next;
		current_camera->next = camera;
		camera->next->prev = camera;
	}
	else
	{
		cameras = camera;
		cameras->next = cameras;
		cameras->prev = cameras;
	}
	gci.dprintf ("GameCam: camera \"%s\" added to list\n", camera->name);
	return camera;
}


// camera update
void camera_fixed_update (camera_t *camera, edict_t *ent, char *name)
{
	if (name)
	{
		strcpy (camera->name, name);
		if (camera->name[0] == '\0')
			strcpy (camera->name, " ");
	}
	VectorCopy (ent->s.origin, camera->origin);
	VectorCopy (ent->s.angles, camera->angles);
	camera->fov = ent->client->ps.fov;
	gci.dprintf ("GameCam: camera \"%s\" updated\n", camera->name);
}


// camera remove 
void camera_fixed_remove (camera_t *camera)
{
	if (camera == camera->next) // last camera
		cameras = NULL;
	else if (camera == cameras) // first camera on list
		cameras = camera->next;
	// remove from list
	camera->next->prev = camera->prev;
	camera->prev->next = camera->next;
	gci.dprintf ("GameCam: camera \"%s\" removed\n", camera->name);
	// free memory
	gci.TagFree (camera);
}


// camera list 
void camera_fixed_list (edict_t *ent)
{
	if (cameras)
	{
		int cam_count = 0;
		camera_t *current_camera = cameras;
		char camera_record[MAX_STRING_CHARS];

		do
		{
			cam_count++;
			sprintf (camera_record, 
				"\\!name\\!   %s\n\\!origin\\! %s\n\\!angles\\! %s\n\\!fov\\!    %d\n\n",
				current_camera->name,
				vtos (current_camera->origin),
				vtos (current_camera->angles),
				(int) current_camera->fov);
			gci.dprintf (motd (camera_record));
			current_camera = current_camera->next;
		}
		while (current_camera != cameras);
		gci.dprintf ("%d cameras defined\n", cam_count);
	}
	else
		gci.cprintf (ent, PRINT_HIGH, "cameras not defined\n");
}


// find a visible camera closest to entity
// returns NULL if no such camera exists (or no cameras)
camera_t *camera_fixed_find (edict_t *ent)
{
	float range, minrange = 4096.0F;
	vec3_t vDiff;
	trace_t trace;
	camera_t *current_camera, *best_camera;

	if (cameras == NULL) return NULL;
	best_camera = NULL;
	range = 0;
	current_camera = cameras;
	do
	{
		if (gci.inPVS (current_camera->origin, ent->s.origin))
		{
			VectorSubtract (ent->s.origin, current_camera->origin, vDiff);
			if (best_camera == NULL || ((range = VectorLength (vDiff)) < minrange))
			{
				trace = gci.trace (ent->s.origin, NULL, NULL, current_camera->origin, 
					ent, CONTENTS_SOLID);
				if (trace.fraction == 1) // camera can see target
				{
					minrange = range;
					best_camera = current_camera;
				}
			}
		}
		current_camera = current_camera->next;
	} while (current_camera != cameras);
	return best_camera;
}


void camera_fixed_select (camera_t *camera, edict_t *ent, edict_t *target)
{
	int i, clientID;
	vec3_t vDiff;

	clientID = numEdict (ent) - 1;

	VectorCopy (camera->origin, ent->s.origin);

	if (gc_set_fov->value)
		set_fov (ent, 90, false);

	if (target)
	{
		VectorSubtract (target->s.origin, camera->origin, vDiff);
		vectoangles (vDiff, ent->client->ps.viewangles);
	}
	else
		VectorCopy (camera->angles, ent->client->ps.viewangles);
	VectorCopy (ent->client->ps.viewangles, ent->s.angles);
	VectorCopy (ent->client->ps.viewangles, clients[clientID].v_angle);
	for (i=0 ; i<3 ; i++) 
	{
		ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(clients[clientID].v_angle[i] - clients[clientID].cmd_angles[i]);
		ent->client->ps.pmove.origin[i] = (short) ent->s.origin[i]*8;
	}
}

