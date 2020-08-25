// GameCam v1.02 - camera proxy game module for Quake II
// Copyright (c) 1998-99, by Avi Rozen (Zung!)
// e-mail: zungbang@telefragged.com

// gc_utils.c		GameCam utilities

#include "gamecam.h"

char *dayNameShort[] =
{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

char *dayName[] =
{"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};

char *monthNameShort[] =
{"Jan","Feb","Mar","Apr","May","Jun", "Jul","Aug","Sep","Oct","Nov","Dec"};

char *monthName[] =
{"January","February","March","April","May","June", "July","August","September","October","November","December"};


edict_t	*pm_passent;

// pmove doesn't need to know about passent and contentmask
trace_t	PM_trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	return gci.trace (start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
}


int PrevClient(int clientID)
{
	int prev;
	edict_t *target;
	qboolean found = false;
	qboolean chase_observers = ((((int) gc_flags->value) & GCF_CHASE_OBSERVERS) != 0);

	for (prev = clientID - 1; prev >= 0 && !found; prev--)
	{
		target = Edict (prev + 1);
		found = (clients[prev].inuse && 
			clients[prev].begin && 
			!clients[prev].spectator &&
			(chase_observers ||
			(target->client && 
			target->client->ps.pmove.pm_type != PM_SPECTATOR &&
			target->client->ps.pmove.pm_type != PM_FREEZE)));
	}
	if (!found)
	{
		for (prev = (int) maxclients->value - 1; prev > clientID && !found; prev--)
		{
			target = Edict (prev + 1);
			found = (clients[prev].inuse &&
				clients[prev].begin && 
				!clients[prev].spectator &&
				(chase_observers ||
				(target->client && 
				target->client->ps.pmove.pm_type != PM_SPECTATOR &&
				target->client->ps.pmove.pm_type != PM_FREEZE)));
		}
	}
	return (found) ? prev + 1 : clientID;
}


int NextClient(int clientID)
{
	int next;
	edict_t *target;
	qboolean found = false;
	qboolean chase_observers = ((((int) gc_flags->value) & GCF_CHASE_OBSERVERS) != 0);

	for (next = clientID + 1; next < (int) maxclients->value && !found; next++)
	{
		target = Edict (next + 1);
		found = (clients[next].inuse &&
			clients[next].begin && 
			!clients[next].spectator &&
			(chase_observers ||
			(target->client && 
			target->client->ps.pmove.pm_type != PM_SPECTATOR &&
			target->client->ps.pmove.pm_type != PM_FREEZE)));
	}
	if (!found)
	{
		for (next = 0; next < clientID && !found; next++)
		{
			target = Edict (next + 1);
			found = (clients[next].inuse && 
				clients[next].begin && 
				!clients[next].spectator &&
				(chase_observers ||
				(target->client && 
				target->client->ps.pmove.pm_type != PM_SPECTATOR &&
				target->client->ps.pmove.pm_type != PM_FREEZE)));
		}
	}
	return (found) ? next - 1 : clientID;
}


int ClosestClient(int clientID)
{
	int i, other;
	short stat_id_view;
	float range, min_range = -1.0F;
	edict_t *ent, *target;
	vec3_t diff;
	qboolean chase_observers = ((((int) gc_flags->value) & GCF_CHASE_OBSERVERS) != 0);

	ent = Edict(clientID + 1);
	// use SetIDView to find which client is in view
	stat_id_view = ent->client->ps.stats[STAT_ID_VIEW]; // save
	SetIDView (ent, true); // true means that only *real* clients are found (can't chase bots)
	//	SetIDView (ent, false);
	other = ent->client->ps.stats[STAT_ID_VIEW];
	ent->client->ps.stats[STAT_ID_VIEW] = stat_id_view; // restore
	// if there's a client in view return its number
	if (other) 
		return (other - CS_PLAYERSKINS);

	// if no clients are in view then find the closest one
	for (i = 0; i < maxclients->value; i++) 
	{
		target = Edict (i + 1);
		if (i != clientID &&
			clients[i].inuse && 
			clients[i].begin && 
			!clients[i].spectator &&
			(chase_observers ||
			(target->client && 
			target->client->ps.pmove.pm_type != PM_SPECTATOR &&
			target->client->ps.pmove.pm_type != PM_FREEZE)))
		{
			VectorSubtract(ent->s.origin, target->s.origin, diff);
			range = VectorLength(diff);
			if (min_range < 0 || range < min_range) 
			{
				min_range = range;
				other = i;
			}
		}
	}

	// no clients? -> return self
	if (min_range < 0)
		other = clientID;

	return other;
}


qboolean IsVisible(edict_t *pPlayer1, edict_t *pPlayer2, float maxrange)
{
	vec3_t vLength;
	float distance;
	trace_t trace;

	// check for looking through non-transparent water
	if (!gci.inPVS(pPlayer1->s.origin, pPlayer2->s.origin))
		return false;

	trace = gci.trace (pPlayer1->s.origin, vec3_origin, vec3_origin, pPlayer2->s.origin, pPlayer1, MASK_SOLID);

	VectorSubtract(pPlayer1->s.origin, pPlayer2->s.origin, vLength);
	distance = VectorLength(vLength);

	return ((maxrange == 0 || distance < maxrange) && trace.fraction == 1.0);
}


edict_t *ClosestVisible(edict_t *ent, float maxrange, qboolean pvs)
{
	int i;
	edict_t *pTarget;
	edict_t *pBest = NULL;
	vec3_t vDistance;
	float fCurrent, fClosest = -1.0F;

	for (i = 0; i < maxclients->value; i++)
	{
		pTarget = Edict(i + 1);
		if (pTarget != ent &&
			((clients[i].inuse &&
			clients[i].begin &&
			!clients[i].spectator) ||
			(pTarget->inuse &&
			pTarget->s.modelindex !=0)) &&
			pTarget->client &&
			pTarget->client->ps.pmove.pm_type != PM_SPECTATOR &&
			pTarget->client->ps.pmove.pm_type != PM_FREEZE &&
			clients[i].last_pmtype != PM_DEAD &&
			clients[i].last_pmtype != PM_GIB &&
			((pvs)? gci.inPVS (ent->s.origin, pTarget->s.origin):IsVisible (ent, pTarget, maxrange)))
		{
			VectorSubtract(pTarget->s.origin, ent->s.origin, vDistance);
			fCurrent = VectorLength(vDistance);
			if (fClosest < 0 || fCurrent < fClosest)
			{
				pBest = pTarget;
				fClosest = fCurrent;
			}
		}
	}
	return pBest;
}


// count all players, including bots, but excluding spectators
int numPlayers(void)
{
	int i, count = 0;
	edict_t *current;

	for (i = 0; i < maxclients->value; i++) 
	{
		current = Edict(i+1);
		if (((clients[i].inuse &&
			clients[i].begin &&
			!clients[i].spectator) ||
			(current->inuse &&
			current->s.modelindex != 0)) &&
			current->client &&
			current->client->ps.pmove.pm_type != PM_SPECTATOR &&
			current->client->ps.pmove.pm_type != PM_FREEZE)
			count ++;
	}
	return count;
}


int getBestClient (void)
{
	int i;
	int clientID = -1;
	int frags = 0;
	edict_t *current;

	for (i = 0; i < maxclients->value; i++) 
	{
		current = Edict(i+1);
		if ((clients[i].inuse &&
			clients[i].begin &&
			!clients[i].spectator) &&
			(clientID < 0 || current->client->ps.stats[STAT_FRAGS] > frags))
		{
			clientID = i;
			frags = current->client->ps.stats[STAT_FRAGS];
		}
	}
	return clientID;
}


qboolean detect_intermission (void)
{
	qboolean intermission = true;
	qboolean found = false;
	int i;
	edict_t *current;

	for (i = 0; i < maxclients->value; i++)
	{
		current = Edict(i+1);
		if (!found)
			found = clients[i].inuse && clients[i].begin && !clients[i].spectator;
		if (!found)
			continue;
		if (clients[i].inuse && 
			clients[i].begin && 
			!clients[i].spectator)
			if (!(current->client->ps.stats[STAT_LAYOUTS] & 1) ||
				current->client->ps.pmove.pm_type != PM_FREEZE)
			{
				intermission = false;
				break;
			}
	}	
	return (found && intermission);
}

char getHexDigit (char hex_char)
{
	if (hex_char >= '0' && hex_char <= '9')
		return hex_char - '0';
	if (hex_char >= 'a' && hex_char <= 'f')
		return hex_char - 'a';
	if (hex_char >= 'A' && hex_char <= 'F')
		return hex_char - 'A';
	return '?'; // not a hex digit?
}

int getHex (char *hex_string, char *hex_number)
{
	*hex_number = 0;
	if ((hex_string[0] >= '0' && hex_string[0] <= '9') ||
		(hex_string[0] >= 'A' && hex_string[0] <= 'F') ||
		(hex_string[0] >= 'a' && hex_string[0] <= 'f'))
	{
		*hex_number = getHexDigit (hex_string[0]);
		if ((hex_string[1] >= '0' && hex_string[1] <= '9') ||
			(hex_string[1] >= 'A' && hex_string[1] <= 'F') ||
			(hex_string[1] >= 'a' && hex_string[1] <= 'f'))
		{
			*hex_number *= 16;
			*hex_number += getHexDigit (hex_string[1]);
			return 2;
		}
		else
			return 1;
	}
	else
		return 0;
}


int getOct (char *oct_string, char *oct_number)
{
	*oct_number = 0;
	if (oct_string[0] >= '0' && oct_string[0] <= '7')
	{
		*oct_number = oct_string[0] - '0';
		if (oct_string[1] >= '0' && oct_string[1] <= '7')
		{
			*oct_number *= 8;
			*oct_number += oct_string[1] - '0';
			if (oct_string[2] >= '0' && oct_string[2] <= '7')
			{
				*oct_number *= 8;
				*oct_number += oct_string[2] - '0';
				return 3;
			}
			else
				return 2;
		}
		else
			return 1;
	}
	else
		return 0;
}


char *highlightText (char *text)
{
	while (*text)
		*text++ |= 0x80;
	return text;
}


int sortScores (const void *a, const void *b)
{
	return ((Edict(((int *)b)[0] + 1))->client->ps.stats[STAT_FRAGS] - 
		(Edict(((int *)a)[0] + 1))->client->ps.stats[STAT_FRAGS]);
}


char *scoreBoard (char *text)
{
	int i, j;
	int hiscores[MAX_CLIENTS];
	edict_t *current;

	for (i = 0, j = 0; i < maxclients->value; i++)
	{
		current = Edict(i + 1);

		if (((clients[i].inuse &&
			clients[i].begin &&
			!clients[i].spectator) ||
			(current->inuse &&
			current->s.modelindex != 0)) &&
			current->client &&
			current->client->ps.pmove.pm_type != PM_SPECTATOR &&
			current->client->ps.pmove.pm_type != PM_FREEZE)
			hiscores[j++] = i;
	}
	if (j)
	{
		strcpy (text, "\213");
		qsort (hiscores, j, sizeof(int), sortScores);
		for (i = 0; i < ((gc_maxscores->value)? MIN(j, gc_maxscores->value):j); i++)
		{
			char currentScore[MAX_INFO_STRING];
			char currentName[MAX_INFO_VALUE];

			strcpy (currentName, ConfigStrings[CS_PLAYERSKINS + hiscores[i]]);
			char* tp = strstr (currentName, "\\"); // chomp off skin
			if (tp) *tp = '\0';
			sprintf (currentScore, " %s - %d \213", currentName, (Edict(hiscores[i] + 1))->client->ps.stats[STAT_FRAGS]);
			if (strlen (text) + strlen (currentScore) < MAX_STRING_CHARS)
				strcat (text, currentScore);
			else
				break;
		}
	}
	else
		highlightText (strcpy (text, "\235\236server\236is\236empty\236\237"));
	return text;
}


char *motd (char *motdstr)
{
	int i = 0;
	qboolean highlight = false;
	static char motdprint[MAX_STRING_CHARS];

	motdprint[0] = '\0';
	while (i < MAX_STRING_CHARS && *motdstr)
	{
		motdprint[i] = *motdstr | ((highlight)? 0x80:0x00);
		if (*motdstr == '\\')
		{
			if (*(motdstr + 1) == 'n')
			{
				motdprint[i] = '\n';
				motdstr++;
			}
			else if (*(motdstr + 1) == '!')
			{
				highlight = !highlight;
				i--;
				motdstr++;
			}
			else if (*(motdstr + 1) == '\"')
			{
				motdprint[i] = '\"' | ((highlight)? 0x80:0x00);
				motdstr++;
			}
			else if (*(motdstr + 1) == '\\')
				motdstr++;
			else if (*(motdstr + 1) == 'x')
			{
				motdstr += getHex (motdstr + 2, &motdprint[i]);
				motdstr++;
			}
			else if (*(motdstr + 1) >= '0' && *(motdstr + 1) <= '7')
				motdstr += getOct (motdstr + 1, &motdprint[i]);
			else if (*(motdstr + 1) == '{')
			{
				int j = 0;
				cvar_t *temp_cvar;
				char esc_code[MAX_STRING_CHARS];
				char temp_motd[MAX_STRING_CHARS];

				struct tm *today;
				time_t sysclock;

				time (&sysclock);
				today = localtime (&sysclock);

				temp_motd[0] = '\0';
				esc_code[0] = '\0';
				motdstr++;
				while (*(motdstr++) && *motdstr !='}')
					esc_code[j++] = *motdstr;
				if (*(motdstr) == '\0') // string too short
					motdstr--;
				else
				{
					esc_code[j] = '\0';
					if (esc_code[0] == '$' && esc_code[j - 1] == '$')
					{
						esc_code[j - 1] = '\0';
						if (esc_code[1])
						{
							temp_cvar = gci.cvar (&esc_code[1], "", 0);
							strcpy (temp_motd, temp_cvar->string);
						}
					}
					else if (strcmp (esc_code, "sb") == 0)
						scoreBoard (temp_motd);
					else if (strcmp (esc_code, "tt") == 0)
						sprintf (temp_motd, "%d:%.2d %s", today->tm_hour % 12 + ((today->tm_hour == 12)? 12:0), today->tm_min, (today->tm_hour >= 12)? "pm":"am");
					else if (strcmp (esc_code, "hh") == 0)
						sprintf (temp_motd, "%d", today->tm_hour % 12 + ((today->tm_hour == 12)? 12:0));
					else if (strcmp (esc_code, "HH") == 0)
						sprintf (temp_motd, "%d", today->tm_hour);
					else if (strcmp (esc_code, "hm") == 0)
						sprintf (temp_motd, "%.2d", today->tm_min);
					else if (strcmp (esc_code, "ss") == 0)
						sprintf (temp_motd, "%.2d", today->tm_sec);
					else if (strcmp (esc_code, "am") == 0)
						sprintf (temp_motd, "%s", (today->tm_hour >= 12)? "pm":"am");
					else if (strcmp (esc_code, "AM") == 0)
						sprintf (temp_motd, "%s", (today->tm_hour >= 12)? "PM":"AM");
					else if (strcmp (esc_code, "dd") == 0)
						strcpy (temp_motd, dayNameShort[today->tm_wday]);
					else if (strcmp (esc_code, "DD") == 0)
						strcpy (temp_motd, dayName[today->tm_wday]);
					else if (strcmp (esc_code, "dn") == 0)
						sprintf (temp_motd, "%d", today->tm_mday);
					else if (strcmp (esc_code, "mm") == 0)
						strcpy (temp_motd, monthNameShort[today->tm_mon]);
					else if (strcmp (esc_code, "MM") == 0)
						strcpy (temp_motd, monthName[today->tm_mon]);
					else if (strcmp (esc_code, "mn") == 0)
						sprintf (temp_motd, "%d", today->tm_mon + 1);
					else if (strcmp (esc_code, "yy") == 0)
						sprintf (temp_motd, "%d", today->tm_year % 100);
					else if (strcmp (esc_code, "YY") == 0)
						sprintf (temp_motd, "%d", 1900 + today->tm_year);
					if (highlight)
						highlightText (temp_motd);
					temp_motd[MAX_STRING_CHARS - i - 1] = '\0'; // clip
					strcpy (&motdprint[i], temp_motd);
					i = strlen (motdprint) - 1;
				}
			}
		}
		motdstr++;
		i++;
	}
	if (i >= MAX_STRING_CHARS)
		i = MAX_STRING_CHARS - 1;
	motdprint[i] = '\0';
	return motdprint;
}


// trim a and compare it to b
qboolean trimcmp (char *a, char *b)
{
	char *pos_head, *pos_tail;

	if ((pos_head = pos_tail = strstr (a, b)) == NULL)
		return false;
	while (pos_head != a)
		if (!isspace (*(--pos_head)))
			return false;
	pos_tail += strlen (b);
	while (*pos_tail)
		if (!isspace (*(pos_tail++)))
			return false;
	return true;
}


// test if line is blank or remark (starts with '//')
qboolean blank_or_remark (char *line)
{
	while (*line && isspace (*line)) 
		line++;
	return ((*line == '\0') || (*line == '/' && *(line+1) == '/'));
}


// set client fov (make sure fov in userinfo is the same)
// if force is true then fov is set to "90" or "90.0"
// so that client is forced to change info - letting us
// to modify the name reported to GameSpy by the server
// (the name stuff is done in ClientUserinfoChanged)
void set_fov (edict_t *ent, float fov, qboolean force)
{
	char fov_cmd[MAX_INFO_VALUE];

	if (force)
	{
		int clientID = numEdict (ent) - 1;

		ent->client->ps.fov = 90;
		if (strcmp (Info_ValueForKey (clients[clientID].userinfo, "fov"), "90"))
			strcpy (fov_cmd, "fov 90\n");
		else
			strcpy (fov_cmd, "fov 90.0\n");
	}
	else
	{
		if (ent->client->ps.fov == fov)
			return;
		ent->client->ps.fov = fov;
		sprintf (fov_cmd, "fov %d\n", (int) fov);
	}
	gci.WriteByte (svc_stufftext);
	gci.WriteString (fov_cmd);
	gci.unicast (ent, true);
}


qboolean sameTeam (edict_t *player1, edict_t *player2)
{
	char *skin1, *skin2, model1[MAX_INFO_VALUE], model2[MAX_INFO_VALUE];
	int client1, client2;

	// no teams
	if ((((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)) == 0) && 
		gc_teams->string[0] == '\0' && !ctf_game)
		return false;
	// detect teams by model/skin
	client1 = numEdict (player1) - 1;
	client2 = numEdict (player2) - 1;
	skin1 = strstr (ConfigStrings[CS_PLAYERSKINS + client1], "\\") + 1;
	skin2 = strstr (ConfigStrings[CS_PLAYERSKINS + client2], "\\") + 1;

	if ((((int) dmflags->value) & DF_SKINTEAMS) ||
		((((int) gc_flags->value) & GCF_TEAMS) && strcmp (gc_teams->string, "skin") == 0) ||
		(!(((int) gc_flags->value) & GCF_TEAMS) && ctf_game))
	{
		skin1 = strchr (skin1, '/') + 1;
		skin2 = strchr (skin2, '/') + 1;
		return (strcmp (skin1, skin2) == 0);
	}
	if ((((int) dmflags->value) & DF_MODELTEAMS) || 
		((((int) gc_flags->value) & GCF_TEAMS) && strcmp (gc_teams->string, "model") == 0))
	{
		strcpy (model1, skin1);
		*(strchr (model1, '/')) = '\0';
		strcpy (model2, skin2);
		*(strchr (model2, '/')) = '\0';
		return (strcmp (model1, model2) == 0);
	}
	// detect teams by other model
	if (((int) gc_flags->value) & GCF_TEAMS)
	{
		if (strcmp (gc_teams->string, "gun") == 0)
			return (player1->client->ps.gunindex == player2->client->ps.gunindex);
		if (strcmp (gc_teams->string, "model2") == 0)
			return (player1->s.modelindex2 == player2->s.modelindex2);
		if (strcmp (gc_teams->string, "model3") == 0)
			return (player1->s.modelindex3 == player2->s.modelindex3);
		if (strcmp (gc_teams->string, "model4") == 0)
			return (player1->s.modelindex4 == player2->s.modelindex4);
	}
	// false by default
	return false;
}


// limit angle to [-180, 180]
float anglemod (float angle)
{
	while (fabs (angle) > 180)
		if (angle > 0)
			angle -= 360;
		else
			angle += 360;
	return angle;
}


// subtract angle b from a to give minimum difference
// assume a and b are in [-180, 180]
float anglediff (float a, float b)
{
	float c, c1, c2, c3;

	c1 = a - b;
	c2 = a - (b + 360);
	c3 = a - (b - 360);
	c = c1;
	if (fabs (c2) < fabs (c))
		c = c2;
	if (fabs (c3) < fabs (c))
		c = c3;
	return c;
}


// case independent string compare
// if s1 is contained within s2 then return 0, they are "equal".
// else return the lexicographical difference between them.
int	Q_strcasecmp(const char *s1, const char *s2)
{
	const unsigned char
		*uc1 = (const unsigned char *)s1,
		*uc2 = (const unsigned char *)s2;

	while (tolower(*uc1) == tolower(*uc2++))
		if (*uc1++ == '\0')
			return (0);
	return (tolower(*uc1) - tolower(*--uc2));
}

// case independent string compare of length n
// compare strings up to length n or until the end of s1
// if s1 is contained within s2 then return 0
// else return the lexicographical difference between them.
int Q_strncasecmp(const char *s1, const char *s2, size_t n)
{
	const unsigned char
		*uc1 = (const unsigned char *)s1,
		*uc2 = (const unsigned char *)s2;

	if (n != 0) {
		do {
			if (tolower(*uc1) != tolower(*uc2++))
				return (tolower(*uc1) - tolower(*--uc2));
			if (*uc1++ == '\0')
				break;
		} while (--n != 0);
	}
	return (0);
}
