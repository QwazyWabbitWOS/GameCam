// GameCam v1.02 - camera proxy game module for Quake II
// Copyright (c) 1998-99, by Avi Rozen (Zung!)
// e-mail: zungbang@telefragged.com

// gc_menu.c		GameCam menu system

#include "gamecam.h"

void Menu_CameraMode_f (edict_t *ent, struct pmenu_s *entry, int flag);
void Menu_CameraOptions_f (edict_t *ent, struct pmenu_s *entry, int flag);
void Menu_JoinGame_f (edict_t *ent, struct pmenu_s *entry, int flag);
void Menu_Help_f (edict_t *ent, struct pmenu_s *entry, int flag);
void Menu_PlayerID_f (edict_t *ent, struct pmenu_s *entry, int flag);
void Menu_Ticker_f (edict_t *ent, struct pmenu_s *entry, int flag);
void Menu_Fixed_f (edict_t *ent, struct pmenu_s *entry, int flag);
void Menu_Creep_f (edict_t *ent, struct pmenu_s *entry, int flag);
void Menu_Demo_f (edict_t *ent, struct pmenu_s *entry, int flag);
//void Menu_Arena_f (edict_t *ent, struct pmenu_s *entry, int flag);

char separator[] = "\235\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\236\237";

pmenu_t menu_main[] = {
	{ "\\!GameCam\\! v" GAMECAMVERNUM,		PMENU_ALIGN_CENTER, NULL, NULL },
	{ "\255\275\220The Evil Eye\221\275\255",	PMENU_ALIGN_CENTER, NULL, NULL },
	{ "\220\200\221 1998-99, by \\!Zung\\!!",PMENU_ALIGN_CENTER, NULL, NULL },
	{ separator,			PMENU_ALIGN_CENTER, NULL, NULL },
	{ NULL,					PMENU_ALIGN_CENTER, NULL, NULL },
	{ "Camera Mode",		PMENU_ALIGN_LEFT, NULL, Menu_CameraMode_f },
	{ NULL,					PMENU_ALIGN_LEFT, NULL, NULL },
	{ "Camera Options",		PMENU_ALIGN_LEFT, NULL, Menu_CameraOptions_f },
	{ NULL,					PMENU_ALIGN_LEFT, NULL, NULL },
	{ "Camera Help",		PMENU_ALIGN_LEFT, NULL, Menu_Help_f },
	{ NULL,					PMENU_ALIGN_LEFT, NULL, NULL },
	{ "Join Game",		PMENU_ALIGN_LEFT, NULL, Menu_JoinGame_f },
	{ NULL,					PMENU_ALIGN_LEFT, NULL, NULL },
	{ separator,			PMENU_ALIGN_CENTER, NULL, NULL },
	{ "Use \\![\\! and \\!]\\! to move",PMENU_ALIGN_LEFT, NULL, NULL },
	{ "\\!ENTER\\! to select",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ "\\!BS\\! to go to parent menu",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ "\\!TAB\\! or \\!ESC\\! to exit menu",	PMENU_ALIGN_LEFT, NULL, NULL },
};


pmenu_t menu_help[] = {
	{ "\\!GameCam\\! v" GAMECAMVERNUM,			PMENU_ALIGN_CENTER, NULL, NULL },
	{ "\255\275\220The Evil Eye\221\275\255",	PMENU_ALIGN_CENTER, NULL, NULL },
	{ "\220\200\221 1998-99, by \\!Zung\\!!",PMENU_ALIGN_CENTER, NULL, NULL },
	{ separator,	PMENU_ALIGN_CENTER, NULL, NULL },
	{ NULL,			PMENU_ALIGN_CENTER, NULL, NULL },
	{ "\\!Keyboard:\\!",		PMENU_ALIGN_LEFT, NULL, NULL },
	{ NULL,			PMENU_ALIGN_CENTER, NULL, NULL },
	{ "Use \\!TAB\\! to show the menu,", PMENU_ALIGN_LEFT, NULL, NULL },
	{ "\\![\\! and \\!]\\! to enter CHASE",PMENU_ALIGN_LEFT, NULL, NULL },
	{ "mode and select prev/next",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ "player, \\!ENTER\\! to select",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ "ACTION mode, and \\!CTRL\\! to",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ "reset manual position of",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ "CHASE camera.",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ NULL, PMENU_ALIGN_LEFT, NULL, NULL },
	{ "\\!Console Commands:\\!",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ NULL,			PMENU_ALIGN_CENTER, NULL, NULL },
	{ "Type '\\!camera ?\\!' for help.", PMENU_ALIGN_LEFT, NULL, NULL },
};

pmenu_t menu_options[] = {
	{ "\\!GameCam\\! v" GAMECAMVERNUM,			PMENU_ALIGN_CENTER, NULL, NULL },
	{ "\255\275\220The Evil Eye\221\275\255",	PMENU_ALIGN_CENTER, NULL, NULL },
	{ "\220\200\221 1998-99, by \\!Zung\\!!",PMENU_ALIGN_CENTER, NULL, NULL },
	{ separator,	PMENU_ALIGN_CENTER, NULL, NULL },
	{ NULL,			PMENU_ALIGN_CENTER, NULL, NULL },
	{ "\\!Action Camera:\\!",	PMENU_ALIGN_LEFT, NULL, NULL },
	{ NULL, PMENU_ALIGN_CENTER, NULL, NULL },
	{ "Player ID",	PMENU_ALIGN_LEFT, NULL, Menu_PlayerID_f },
	{ "Ticker Tape",	PMENU_ALIGN_LEFT, NULL, Menu_Ticker_f },
	{ "Fixed Cameras",	PMENU_ALIGN_LEFT, NULL, Menu_Fixed_f },
	{ NULL, PMENU_ALIGN_CENTER, NULL, NULL },
	{ "\\!Chase Camera:\\!",PMENU_ALIGN_LEFT, NULL, NULL },
	{ NULL, PMENU_ALIGN_CENTER, NULL, NULL },
	{ "Auto Position", PMENU_ALIGN_LEFT, NULL, Menu_Creep_f },
	{ NULL, PMENU_ALIGN_CENTER, NULL, NULL },
	{ "\\!Other:\\!",PMENU_ALIGN_LEFT, NULL, NULL },
	{ NULL, PMENU_ALIGN_CENTER, NULL, NULL },
	//{ "Arena Number",		PMENU_ALIGN_LEFT, NULL, Menu_Arena_f },
	{ "Auto Demo Recording",PMENU_ALIGN_LEFT, NULL, Menu_Demo_f },
};


void Menu_CameraMode_f (edict_t *ent, struct pmenu_s *entry, int flag)
{
	int clientID, oldmode;

	clientID = numEdict (ent) - 1;

	switch (flag)
	{
	case PMENU_FLAG_CREATE:
		entry->option = (char *) gci.TagMalloc (7 * sizeof(char), TAG_GAME);
		switch (clients[clientID].mode)
		{
		case CAMERA_FREE:
			strcpy (entry->option, "FREE");
			break;
		case CAMERA_CHASE:
			strcpy (entry->option, "CHASE");
			break;
		case CAMERA_ACTION:
			strcpy (entry->option, "ACTION");
			break;
		}
		break;

	case PMENU_FLAG_DESTROY:
		if (!intermission)
		{
			oldmode = clients[clientID].mode;
			switch (*entry->option)
			{
			case 'F': // FREE
				clients[clientID].mode = CAMERA_FREE;
				break;
			case 'A': // ACTION
				clients[clientID].mode = CAMERA_ACTION;
				break;
			case 'C': // CHASE
				clients[clientID].mode = CAMERA_CHASE;
				break;
			}
			if (clients[clientID].spectator && clients[clientID].mode != oldmode) 
			{
				switch (oldmode) 
				{
				case CAMERA_FREE:
					clients[clientID].reset_layouts = (clients[clientID].mode == CAMERA_CHASE);
					camera_free_wrapup (clientID);
					break;
				case CAMERA_CHASE:
					camera_chase_wrapup (clientID);
					break;
				case CAMERA_ACTION:
					clients[clientID].reset_layouts = (clients[clientID].mode == CAMERA_CHASE);
					camera_action_wrapup (clientID);
				}
				switch (clients[clientID].mode) 
				{
				case CAMERA_FREE:
					camera_free_setup (clientID);
					break;
				case CAMERA_CHASE:
					camera_chase_setup (clientID);
					break;
				case CAMERA_ACTION:
					camera_action_setup (clientID);
				}
			}
		}
		gci.TagFree (entry->option);
		break;

	case PMENU_FLAG_SELECT:
		switch (*entry->option)
		{
		case 'F': // FREE
			strcpy (entry->option, "ACTION");
			break;
		case 'A': // ACTION
			if (((int)gc_flags->value) & GCF_ALLOW_CHASE)
				strcpy (entry->option, "CHASE");
			else if (((int)gc_flags->value) & GCF_ALLOW_FREE)
				strcpy (entry->option, "FREE");
			break;
		case 'C': // CHASE
			if (((int)gc_flags->value) & GCF_ALLOW_FREE)
				strcpy (entry->option, "FREE");
			else 
				strcpy (entry->option, "ACTION");
			break;
		}
		break;
	}
}


void Menu_CameraOptions_f (edict_t *ent, struct pmenu_s *entry, int flag)
{
	int clientID;

	clientID = numEdict (ent) - 1;

	switch (flag)
	{
	case PMENU_FLAG_CREATE:
		break;
	case PMENU_FLAG_DESTROY:
		break;
	case PMENU_FLAG_SELECT:
		PMenu_Open (ent , menu_options, -1, sizeof(menu_options) / sizeof(pmenu_t), clients[clientID].menu_hnd);
		break;
	}
}


void Menu_JoinGame_f (edict_t *ent, struct pmenu_s *entry, int flag)
{
	switch (flag)
	{
	case PMENU_FLAG_CREATE:
		break;
	case PMENU_FLAG_DESTROY:
		break;
	case PMENU_FLAG_SELECT:
		SpectatorEnd (ent, "");
		break;
	}
}


void Menu_Help_f (edict_t *ent, struct pmenu_s *entry, int flag)
{
	int clientID;

	clientID = numEdict (ent) - 1;

	switch (flag)
	{
	case PMENU_FLAG_CREATE:
		break;
	case PMENU_FLAG_DESTROY:
		break;
	case PMENU_FLAG_SELECT:
		PMenu_Open (ent , menu_help, -1, sizeof(menu_help) / sizeof(pmenu_t), clients[clientID].menu_hnd);
		break;
	}
}


void Menu_PlayerID_f (edict_t *ent, struct pmenu_s *entry, int flag)
{
	int clientID;

	clientID = numEdict (ent) - 1;

	switch (flag)
	{
	case PMENU_FLAG_CREATE:
		entry->option = (char *) gci.TagMalloc (4 * sizeof(char), TAG_GAME);
		if (clients[clientID].id)
			strcpy (entry->option, "ON");
		else
			strcpy (entry->option, "OFF");
		break;

	case PMENU_FLAG_DESTROY:
		gci.TagFree (entry->option);
		break;

	case PMENU_FLAG_SELECT:
		if (clients[clientID].id)
			strcpy (entry->option, "OFF");
		else
			strcpy (entry->option, "ON");
		clients[clientID].id = !clients[clientID].id;
		break;
	}
}


void Menu_Ticker_f (edict_t *ent, struct pmenu_s *entry, int flag)
{
	int clientID;

	clientID = numEdict (ent) - 1;

	switch (flag)
	{
	case PMENU_FLAG_CREATE:
		entry->option = (char *) gci.TagMalloc (4 * sizeof(char), TAG_GAME);
		if (clients[clientID].ticker)
			strcpy (entry->option, "ON");
		else
			strcpy (entry->option, "OFF");
		break;

	case PMENU_FLAG_DESTROY:
		gci.TagFree (entry->option);
		break;

	case PMENU_FLAG_SELECT:
		if (clients[clientID].ticker)
			strcpy (entry->option, "OFF");
		else
			strcpy (entry->option, "ON");
		clients[clientID].ticker = !clients[clientID].ticker;
		clients[clientID].ticker_frame = 0;
		break;
	}
}


void Menu_Fixed_f (edict_t *ent, struct pmenu_s *entry, int flag)
{
	int clientID;

	clientID = numEdict (ent) - 1;

	switch (flag)
	{
	case PMENU_FLAG_CREATE:
		entry->option = (char *) gci.TagMalloc (4 * sizeof(char), TAG_GAME);
		if (!cameras)
			strcpy (entry->option, "NA");
		else
		{
			if (clients[clientID].fixed)
				strcpy (entry->option, "ON");
			else
				strcpy (entry->option, "OFF");
		}
		break;

	case PMENU_FLAG_DESTROY:
		gci.TagFree (entry->option);
		break;

	case PMENU_FLAG_SELECT:
		if (cameras)
		{
			if (clients[clientID].fixed)
				strcpy (entry->option, "OFF");
			else
				strcpy (entry->option, "ON");
			clients[clientID].fixed = !clients[clientID].fixed;
			if (clients[clientID].fixed)
			{
				clients[clientID].camera = NULL;
				clients[clientID].fixed_switch_time = -1.0F;
				if (gc_set_fov->value)
					set_fov (ent, 90, QFALSE);
			}
		}
		break;
	}
}


void Menu_Creep_f (edict_t *ent, struct pmenu_s *entry, int flag)
{
	int clientID;

	clientID = numEdict (ent) - 1;

	switch (flag)
	{
	case PMENU_FLAG_CREATE:
		entry->option = (char *) gci.TagMalloc (4 * sizeof(char), TAG_GAME);
		if (clients[clientID].chase_auto)
			strcpy (entry->option, "ON");
		else
			strcpy (entry->option, "OFF");
		break;

	case PMENU_FLAG_DESTROY:
		gci.TagFree (entry->option);
		break;

	case PMENU_FLAG_SELECT:
		if (clients[clientID].chase_auto)
			strcpy (entry->option, "OFF");
		else
			strcpy (entry->option, "ON");
		clients[clientID].chase_auto = !clients[clientID].chase_auto;
		clients[clientID].update_chase = QTRUE;
		break;
	}
}


void Menu_Demo_f (edict_t *ent, struct pmenu_s *entry, int flag)
{
	int clientID;

	clientID = numEdict (ent) - 1;

	switch (flag)
	{
	case PMENU_FLAG_CREATE:
		entry->option = (char *) gci.TagMalloc (4 * sizeof(char), TAG_GAME);
		if (clients[clientID].demo)
			strcpy (entry->option, "ON");
		else
			strcpy (entry->option, "OFF");
		break;

	case PMENU_FLAG_DESTROY:
		gci.TagFree (entry->option);
		break;

	case PMENU_FLAG_SELECT:
		if (clients[clientID].demo)
			strcpy (entry->option, "OFF");
		else
			strcpy (entry->option, "ON");
		clients[clientID].demo = !clients[clientID].demo;
		if (clients[clientID].demo)
			demoON (ent);
		else
			demoOFF (ent);
		break;
	}
}

/*
void Menu_Arena_f (edict_t *ent, struct pmenu_s *entry, int flag)
{
	int clientID;

	clientID = numEdict (ent) - 1;

	switch (flag)
	{
	case PMENU_FLAG_CREATE:
		entry->option = (char *) gci.TagMalloc (4 * sizeof(char), TAG_GAME);
		strcpy (entry->option, "NA");
		break;

	case PMENU_FLAG_DESTROY:
		gci.TagFree (entry->option);
		break;

	case PMENU_FLAG_SELECT:
		break;
	}
}
*/


void showMenu (int clientID)
{
	edict_t *ent;

	ent = Edict (clientID + 1);

	if (intermission)
		return;

	if (!clients[clientID].spectator)
		gci.cprintf (ent, PRINT_HIGH, "camera menu is for spectators only\n");
	else
		PMenu_Open (ent , menu_main, -1, sizeof(menu_main) / sizeof(pmenu_t), NULL);
}


void hideMenu (int clientID)
{
	edict_t *ent;

	ent = Edict (clientID + 1);

	if (intermission)
		return;
	
	PMenu_Close (ent);
	ent->client->ps.stats[STAT_LAYOUTS] &= ~1;
	if (clients[clientID].mode != CAMERA_CHASE)
		ticker_restore_statusbar (ent);
}


void PMenu_Open (edict_t *ent, pmenu_t *entries, int cur, int num, pmenuhnd_t *parent)
{
	pmenuhnd_t *hnd;
	pmenu_t *p;
	int i;
	int clientID;

	if (!ent->client)
		return;

	clientID = numEdict (ent) - 1;

	if (!parent && clients[clientID].menu_hnd) 
	{
		gci.dprintf("warning, client #%d already has a menu\n", clientID);
		PMenu_Close(ent);
	}

	hnd = gci.TagMalloc (sizeof (pmenuhnd_t), TAG_GAME);
	hnd->entries = gci.TagMalloc (num * sizeof (pmenu_t), TAG_GAME);
	memcpy (hnd->entries, entries, num * sizeof (pmenu_t));
	// create menu
	for (i = 0, p = hnd->entries; i < num; i++, p++)
		if (p->SelectFunc)
			p->SelectFunc (ent, p, PMENU_FLAG_CREATE);
	hnd->num = num;
	hnd->parent = parent;

	if (cur < 0 || !hnd->entries[cur].SelectFunc) 
	{
		for (i = 0, p = hnd->entries; i < num; i++, p++)
			if (p->SelectFunc)
				break;
	} 
	else
		i = cur;

	if (i >= num)
		hnd->cur = -1;
	else
		hnd->cur = i;

	clients[clientID].menu_hnd = hnd;
	clients[clientID].inven = QFALSE;
	clients[clientID].score = QFALSE;
	clients[clientID].help = QFALSE;
	clients[clientID].layouts = QFALSE;
	clients[clientID].menu = QTRUE;

	PMenu_Update (ent);
}


void PMenu_FreeMenu (edict_t *ent, pmenuhnd_t *hnd)
{
	int i;
	pmenu_t *p;

	for (i = 0, p = hnd->entries; i < hnd->num; i++, p++)
		if (p->SelectFunc)
			p->SelectFunc (ent, p, PMENU_FLAG_DESTROY);
	gci.TagFree (hnd->entries);
	gci.TagFree (hnd);
}


void PMenu_Close (edict_t *ent)
{
	int clientID;

	clientID = numEdict (ent) - 1;

	while (clients[clientID].menu_hnd) 
	{
		pmenuhnd_t *parent = clients[clientID].menu_hnd->parent;
		PMenu_FreeMenu (ent, clients[clientID].menu_hnd);
		clients[clientID].menu_hnd = parent;
	}
	clients[clientID].menu = QFALSE;
	if (clients[clientID].spectator && clients[clientID].mode == CAMERA_CHASE)
		clients[clientID].update_chase = QTRUE;
}


void PMenu_Parent (edict_t *ent)
{
	pmenuhnd_t *parent = NULL;
	int clientID;

	clientID = numEdict (ent) - 1;

	if (clients[clientID].menu_hnd) 
	{
		parent = clients[clientID].menu_hnd->parent;
		PMenu_FreeMenu (ent, clients[clientID].menu_hnd);
	}
	clients[clientID].menu_hnd = parent;
	clients[clientID].menu = (clients[clientID].menu_hnd != NULL);
	if (clients[clientID].menu_hnd)
		PMenu_Update (ent);
	else
	{
		if (clients[clientID].spectator && clients[clientID].mode == CAMERA_CHASE)
			clients[clientID].update_chase = QTRUE;
		ent->client->ps.stats[STAT_LAYOUTS] &= ~1;
	}
}


void PMenu_Update(edict_t *ent)
{
	char string[1400];
	int i;
	pmenu_t *p;
	int x;
	pmenuhnd_t *hnd;
	char *t;
	qboolean alt = QFALSE;
	int clientID;

	clientID = numEdict (ent) - 1;

	if (!clients[clientID].menu_hnd) 
	{
		gci.dprintf("warning: client #%d has no menu\n", clientID);
		return;
	}

	hnd = clients[clientID].menu_hnd;

	// overflow protection
	if (clients[clientID].last_score == framenum)
		return;
	else
		clients[clientID].last_score = framenum;

	strcpy(string, "xv 32 yv 8 picn inventory ");

	for (i = 0, p = hnd->entries; i < hnd->num; i++, p++) 
	{
		char line[MAX_INFO_VALUE];

		if (!p->text || !*(p->text))
			continue; // blank line
		t = p->text;
		if (*t == '*') 
		{
			alt = QTRUE;
			t++;
		}
		if (p->option && *(p->option))
		{
			if (hnd->cur == i)
				sprintf (line, "\\!%-19s\\!%6s", t, p->option);
			else
				sprintf (line, "%-19s\\!%6s\\!", t, p->option);
			strcpy (line, motd (line));
		}
		else
		{
			sprintf (line, "%s", motd (t));
			if (hnd->cur == i)
				highlightText (line);
		}
		sprintf(string + strlen(string), "yv %d ", 32 + i * 8);
		if (p->align == PMENU_ALIGN_CENTER)
			x = 196/2 - strlen(line)*4 + 64;
		else if (p->align == PMENU_ALIGN_RIGHT)
			x = 64 + (196 - strlen(line)*8);
		else
			x = 64;

		sprintf(string + strlen(string), "xv %d ",
			x - ((hnd->cur == i) ? 8 : 0));

		if (hnd->cur == i)
			sprintf(string + strlen(string), "string \"\x8d%s\" ", line);
		else if (alt)
			sprintf(string + strlen(string), "string2 \"%s\" ", line);
		else
			sprintf(string + strlen(string), "string \"%s\" ", line);
		alt = QFALSE;
	}

	gci.WriteByte (svc_layout);
	gci.WriteString (string);
	gci.unicast (ent, QTRUE);
	ent->client->ps.stats[STAT_LAYOUTS] |= 1;
	//gci.dprintf ("strlen(menu)=%d\n", strlen (string));
}


void PMenu_Next(edict_t *ent)
{
	pmenuhnd_t *hnd;
	int i;
	pmenu_t *p;
	int clientID;

	clientID = numEdict (ent) - 1;

	if (!clients[clientID].menu_hnd) 
	{
		gci.dprintf("warning: client #%d has no menu\n", clientID);
		return;
	}

	hnd = clients[clientID].menu_hnd;

	if (hnd->cur < 0)
		return; // no selectable entries

	i = hnd->cur;
	p = hnd->entries + hnd->cur;
	do {
		i++, p++;
		if (i == hnd->num)
			i = 0, p = hnd->entries;
		if (p->SelectFunc)
			break;
	} while (i != hnd->cur);

	hnd->cur = i;
	
	PMenu_Update(ent);
}


void PMenu_Prev(edict_t *ent)
{
	pmenuhnd_t *hnd;
	int i;
	pmenu_t *p;
	int clientID;

	clientID = numEdict (ent) - 1;

	if (!clients[clientID].menu_hnd) 
	{
		gci.dprintf("warning: client #%d has no menu\n", clientID);
		return;
	}

	hnd = clients[clientID].menu_hnd;

	if (hnd->cur < 0)
		return; // no selectable entries

	i = hnd->cur;
	p = hnd->entries + hnd->cur;
	do {
		if (i == 0) 
		{
			i = hnd->num - 1;
			p = hnd->entries + i;
		} 
		else
			i--, p--;
		if (p->SelectFunc)
			break;
	} while (i != hnd->cur);

	hnd->cur = i;

	PMenu_Update(ent);
}


void PMenu_Select(edict_t *ent)
{
	pmenuhnd_t *hnd;
	pmenu_t *p;
	int clientID;

	clientID = numEdict (ent) - 1;

	if (!clients[clientID].menu_hnd) 
	{
		gci.dprintf("warning: client #%d has no menu\n", clientID);
		return;
	}

	hnd = clients[clientID].menu_hnd;

	if (hnd->cur < 0)
		return; // no selectable entries

	p = hnd->entries + hnd->cur;

	if (p->SelectFunc)
		p->SelectFunc(ent, p, PMENU_FLAG_SELECT);
	
	if (clients[clientID].menu_hnd)
		PMenu_Update(ent);
}

