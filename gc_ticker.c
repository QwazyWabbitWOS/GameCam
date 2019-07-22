// GameCam v1.02 - camera proxy game module for Quake II
// Copyright (c) 1998-99, by Avi Rozen (Zung!)
// e-mail: zungbang@telefragged.com

// gc_ticker.c		text ticker tape code

#include "gamecam.h"

// statusbar

char camera_statusbar[] =

// player id
"if 31 "
  "xv 0 "
  "yb -58 "
  "string \"Viewing\" "
  "xv 64 "
  "stat_string 31 "
"endif "

// ticker frame
"if 30 "
  "xv 0 "
  "yb -28 "
  "string \"\22\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\23\24\" "
  "yb -20 "
  "string \"\30\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\31\32\" "
  "yb -24 "
  "xv 4 "
"endif "

// ticker message
"if 29 "
  "xv 8 "
"endif "
"if 30 "
  "stat_string 30 "
"endif "

// camera location
"if 28 "
  "yb -10 "
  "xv 32 "
"endif "
"if 27 "
  "xv 36 "
"endif "
"if 28 "
  "stat_string 28 "
"endif "; 


// ticker tape

#define TICKER_SCRIPT_APPEAR		0x0000
#define TICKER_SCRIPT_SLEEP 		0x0001
#define TICKER_SCRIPT_SCROLLLEFT	0x0002
#define TICKER_SCRIPT_SCROLLRIGHT	0x0003
//#define TICKER_SCRIPT_RANDOM 		0x0004
#define TICKER_SCRIPT_BLINK 		0x0005
//#define TICKER_SCRIPT_SCROLLCENTER	0x0006
#define TICKER_SCRIPT_DO 			0x8001
#define TICKER_SCRIPT_REPEAT		0x8002
#define TICKER_SCRIPT_RELOAD		0x8003
//#define TICKER_SCRIPT_CHAIN 		0x8004
#define TICKER_SCRIPT_UNKNOWN 		0xFFFF

#define TICKER_UPDATE_TEXT			0x0001

int ticker_statusbar_index = 0;
int ticker_flags = 0;
int ticker_dos = 0;
char ticker_text[MAX_STRING_CHARS];
int ticker_offset = 0;
ticker_t *ticker = NULL;	// first ticker script function
ticker_t *ticker_current = NULL;	// current ticker script function

char *ticker_default[] =
{
	"Do",
		"Appear center=true text=\\!GameCam\\! v" GAMECAMVERNUM,
		"Sleep delay=10",
		"ScrollLeft center=true text=\\x90\\x80\\x91 1998-99, Avi \\\"\\!Zung!\\!\\\" Rozen",
		"Sleep delay=10",
		"ScrollRight center=true text=http://www.telefragged.com/\\!zungbang\\!",
		"Do",
			"Sleep delay=3",
			"Appear center=true text=http://www.telefragged.com/zungbang",
			"Sleep delay=3",
			"Appear center=true text=http://www.telefragged.com/\\!zungbang\\!",
		"Repeat times=3",
		"Sleep delay=10",
		"ScrollLeft center=true text=\\!TOP SCORES\\! . . .",
		"ScrollLeft endspace=39 text=\\{sb}",
		"Sleep delay=10",
		"Appear center=true text=type '\\!camera\\!' to see the menu",
		"Sleep delay=10",
		"ScrollRight center=true text=\\!GameCam\\! v" GAMECAMVERNUM,
	"Repeat times=-1",
	""
};

char ticker_spaces[TICKER_MAX_CHARS + 1] = "                                       ";

qboolean ticker_script (char *ticker_filename);


void ticker_clear (edict_t *ent)
{
	int clientID;

	clientID = numEdict(ent) - 1;
	clients[clientID].ticker_delay = 3; // wait before setting up the statusbar
										// (to prevent overflow)

	ent->client->ps.stats[STAT_LAYOUTS] = 0;
	ent->client->ps.stats[STAT_ID_VIEW] = 0;
	ent->client->ps.stats[STAT_CAM_LOCATION] = 0; 
	ent->client->ps.stats[STAT_CAM_OFFSET] = 0; 
	if (clients[clientID].ticker)
	{
		ent->client->ps.stats[STAT_TICKER] = CS_TICKER;
		ent->client->ps.stats[STAT_TICKER_OFFSET] = 0; 
		gci.WriteByte (svc_configstring);
		gci.WriteShort (CS_TICKER);
		gci.WriteString ("");
		gci.unicast (ent, true);
	}
	else
	{
		ent->client->ps.stats[STAT_TICKER] = 0;
		ent->client->ps.stats[STAT_TICKER_OFFSET] = 0; 
	}
	gci.WriteByte (svc_configstring);
	gci.WriteShort (CS_STATUSBAR);
	gci.WriteString ("");
	gci.unicast (ent, true);
}


void ticker_setup (edict_t *ent)
{
	int i;
	//int clientID;
	unsigned int j;

	//clientID = numEdict(ent) - 1;

	for (i = 0, j = 0; j < strlen(camera_statusbar); j += MAX_QPATH, i++)
	{
		gci.WriteByte (svc_configstring);
		gci.WriteShort (CS_STATUSBAR + i);
		gci.WriteString (&camera_statusbar[j]);
		gci.unicast (ent, true);
	}
	gci.WriteByte (svc_configstring);
	gci.WriteShort (CS_STATUSBAR + i);
	gci.WriteString ("");
	gci.unicast (ent, true);
}


void ticker_wrapup (edict_t *ent)
{
	int i, clientID;

	clientID = numEdict(ent) - 1;

	clients[clientID].ticker_delay = 0;
	gci.WriteByte (svc_configstring);
	gci.WriteShort (CS_TICKER);
	gci.WriteString (ConfigStrings[CS_TICKER]);
	gci.unicast (ent, true);

	gci.WriteByte (svc_configstring);
	gci.WriteShort (CS_CAM_LOCATION);
	gci.WriteString (ConfigStrings[CS_CAM_LOCATION]);
	gci.unicast (ent, true);

	for (i = CS_STATUSBAR; i < CS_AIRACCEL; i++) 
	{
		gci.WriteByte (svc_configstring);
		gci.WriteShort (i);
		gci.WriteString (ConfigStrings[i]);
		gci.unicast (ent, true);
	}
}


int ticker_update (void)
{
	qboolean done = false;
	int ticker_flags = 0;
	ticker_t *ticker_next, *ticker_old;
	qboolean ticker_file = false;

	while (ticker_current && !done)
	{
		char *ticker_temp;

		done = true; // modified by do/repeat
		ticker_next = ticker_current->next;
		switch (ticker_current->func)
		{
		case TICKER_SCRIPT_APPEAR:
			strcpy (ticker_text, ticker_spaces);
			ticker_temp = motd (ticker_current->store);
			if (ticker_current->centered && ticker_temp[0])
			{
				int text_len = strlen (ticker_temp);

				ticker_offset = (text_len % 2)? 0:4;
				strncpy (&ticker_text[(TICKER_MAX_CHARS - text_len) / 2], ticker_temp, TICKER_MAX_CHARS);
			}
			else 
			{
				ticker_offset = 0;
				strncpy (&ticker_text[ticker_current->startspace], ticker_temp, TICKER_MAX_CHARS - ticker_current->startspace);
			}
			ticker_text[TICKER_MAX_CHARS] = '\0'; // clip
			ticker_flags |= TICKER_UPDATE_TEXT;
			break;
		case TICKER_SCRIPT_SLEEP:
			ticker_current->counter--;
			if (ticker_current->counter <= 0)
				ticker_current->counter = ticker_current->delay;
			else
				ticker_next = ticker_current;
			break;
		case TICKER_SCRIPT_SCROLLLEFT:
			if (ticker_current->place_target == 0) // first frame of transition
			{
				ticker_current->place = 0;
				if (ticker_text[0])
				{
					// original text
					ticker_current->text[0] = '\0';
					strncpy (ticker_current->text, 
						ticker_spaces, 
						ticker_offset / 8 + ((ticker_offset % 8)? 1:0));
					if (ticker_offset % 8)
						ticker_current->place = 1;
					ticker_current->text[ticker_offset / 8 + ((ticker_offset % 8)? 1:0)] = '\0';
					strcat (ticker_current->text, ticker_text);
					strncat (ticker_current->text, 
						ticker_spaces, 
						TICKER_MAX_CHARS - strlen (ticker_current->text));
					ticker_current->text[TICKER_MAX_CHARS] = '\0';
				}
				else
				{
					// ticker is blanked
					strcpy (ticker_current->text, ticker_spaces);
					ticker_offset = 0;
				}
				ticker_temp = motd (ticker_current->store);
				if (ticker_temp[0])
				{
					int spaces = 0;
					int text_len = strlen (ticker_current->text);
					int temp_len = strlen (ticker_temp);

					// new text
					if (ticker_current->centered && temp_len < TICKER_MAX_CHARS)
					{
						spaces = TICKER_MAX_CHARS - temp_len;
						if (spaces % 2)
						{
							spaces++;
							ticker_current->offset = 4;
							ticker_current->place_target = text_len + 1;
						}
						else
						{
							ticker_current->offset = 0;
							ticker_current->place_target = text_len;
						}
					}
					else
					{
						spaces = ticker_current->startspace * 2;
						ticker_current->offset = 0;
						ticker_current->place_target = text_len + ticker_current->startspace + ticker_current->endspace + strlen (ticker_temp) - TICKER_MAX_CHARS;
					}
					strncat (ticker_current->text, ticker_spaces, spaces / 2);
					ticker_current->text[text_len + spaces / 2] = '\0';
					strcat (ticker_current->text, ticker_temp);
					ticker_current->text[strlen(ticker_current->text) + ticker_current->endspace] = '\0';
					strncat (ticker_current->text, ticker_spaces, ticker_current->endspace);
				}
				else
				{
					// ticker is to be blanked
					strcat (ticker_current->text, ticker_spaces);
					ticker_current->place_target = strlen (ticker_current->text) - TICKER_MAX_CHARS;
					ticker_current->offset = 0;
				}
			}
			if (ticker_offset % 8)
				ticker_offset = 0;
			else
			{
				ticker_offset = 4;
				ticker_current->place++;
			}
			strncpy (ticker_text, 
				&(ticker_current->text[ticker_current->place]), 
				TICKER_MAX_CHARS - ((ticker_offset)? 1:0));
			ticker_text[TICKER_MAX_CHARS - ((ticker_offset)? 1:0)] = '\0'; // clip
			ticker_flags |= TICKER_UPDATE_TEXT;
			if (ticker_current->offset != ticker_offset ||
				ticker_current->place < ticker_current->place_target) // in progress?
				ticker_next = ticker_current;
			else
				ticker_current->place_target = 0; // done.
			break;
		case TICKER_SCRIPT_SCROLLRIGHT:
			if (ticker_current->text[0] == '\0') // first frame of transition
			{
				int tail_len;
				int space_len;

				space_len = ticker_offset / 8;
				tail_len = space_len + strlen (ticker_text);
				ticker_offset = ticker_offset % 8;
				ticker_temp = motd (ticker_current->store);
				if (ticker_current->centered && strlen (ticker_temp) <= TICKER_MAX_CHARS)
				{
					int temp_len = strlen (ticker_temp);
					int startspace = (TICKER_MAX_CHARS - temp_len) / 2;

					ticker_current->offset = ((TICKER_MAX_CHARS - temp_len) % 2)? 4:0;
					strcpy (ticker_current->text, &ticker_spaces[TICKER_MAX_CHARS - startspace]);
					strcat (ticker_current->text, ticker_temp);
					strcat (ticker_current->text, &ticker_spaces[TICKER_MAX_CHARS - startspace]);
				}
				else
				{
					ticker_current->offset = 0;
					strcpy (ticker_current->text, &ticker_spaces[TICKER_MAX_CHARS - ticker_current->startspace]);
					strcat (ticker_current->text, ticker_temp);
					strcat (ticker_current->text, &ticker_spaces[TICKER_MAX_CHARS - ticker_current->endspace]);
				}
				strcat (ticker_current->text, &ticker_spaces[TICKER_MAX_CHARS - space_len]);
				strcat (ticker_current->text, ticker_text);
				ticker_current->place = strlen (ticker_current->text) - tail_len;
			}
			if (ticker_offset % 8)
			{
				ticker_offset = 0;
				ticker_current->place--;
			}
			else
				ticker_offset = 4;
			strncpy (ticker_text, &ticker_current->text[ticker_current->place], TICKER_MAX_CHARS - ((ticker_offset % 8)? 1:0));
			ticker_text[TICKER_MAX_CHARS - ((ticker_offset % 8)? 1:0)] = '\0';
			ticker_flags |= TICKER_UPDATE_TEXT;
			if (ticker_current->place > 0 ||
				ticker_current->offset != ticker_offset) // in progress?
				ticker_next = ticker_current;
			else
				ticker_current->text[0] = '\0';	// transition done
			break;
		case TICKER_SCRIPT_BLINK:
			ticker_current->counter--;
			if (ticker_current->counter <= 0)
			{
				ticker_current->counter = ticker_current->delay;
				if (ticker_current->remaining % 2 == 0)
				{
					strcpy (ticker_current->store, ticker_text);
					ticker_text[0] = '\0';
				}
				else
					strcpy (ticker_text, ticker_current->store);
				ticker_current->remaining--;
				if (ticker_current->remaining <= 0)
					ticker_current->remaining = ticker_current->times;
				else
					ticker_next = ticker_current;
				ticker_flags |= TICKER_UPDATE_TEXT;
			}
			else
				ticker_next = ticker_current;
			break;
		case TICKER_SCRIPT_DO:
			done = false; // continue with script
			break;
		case TICKER_SCRIPT_REPEAT:
			ticker_next = ticker_current->loop;
			if (ticker_current->remaining != -1)
			{
				ticker_current->remaining--;
				if (ticker_current->remaining <= 0)
				{
					ticker_current->remaining = ticker_current->times;
					ticker_next = ticker_current->next;
				}
			}
			done = false; // continue with script
			break;
		case TICKER_SCRIPT_RELOAD:
			// save old ticker (in case we can't load cript)
			ticker_old = ticker;
			ticker = NULL; // so ticker is not shutdown
			if (gc_ticker->string && strlen (gc_ticker->string))
				ticker_file = ticker_script (gc_ticker->string);
			if (ticker_file) 
			{
				ticker_t *ticker_new;

				// new ticker loaded - shutdown old ticker
				ticker_new = ticker;
				ticker = ticker_old;
				ticker_shutdown ();
				ticker = ticker_new;
				ticker_next = ticker;
			}
			else
			{
				// reload failed
				ticker_shutdown ();
				gci.dprintf ("resuming old script\n");
				ticker = ticker_old;
			}
			done = false; // continue with script
			break;
		//case TICKER_SCRIPT_CHAIN:
		//case TICKER_SCRIPT_RANDOM:
		//case TICKER_SCRIPT_SCROLLCENTER:
		case TICKER_SCRIPT_UNKNOWN:
			gci.dprintf ("ticker_update: unknown ticker script directive\n");
			break;
		default:
			gci.dprintf ("ticker_update: bad ticker script directive\n");
			break;
		}
		ticker_current = ticker_next;
	}
	if (ticker_current == NULL) // implied infinite do/repeat around the whole script
		ticker_current = ticker;
	return ticker_flags;
}


void ticker_frame (edict_t *ent)
{
	int clientID;

	clientID = numEdict(ent) - 1;

	if (clients[clientID].ticker)
	{
		ent->client->ps.stats[STAT_TICKER] = CS_TICKER;
		if ((ticker_flags & TICKER_UPDATE_TEXT) || clients[clientID].ticker_frame == 0)
		{
			gci.WriteByte (svc_configstring);
			gci.WriteShort (CS_TICKER);
			gci.WriteString (ticker_text);
			gci.unicast (ent, false);
		}
		if (ticker_offset || clients[clientID].ticker_frame == 0)
			ent->client->ps.stats[STAT_TICKER_OFFSET] = 1; 
		else
			ent->client->ps.stats[STAT_TICKER_OFFSET] = 0;
		clients[clientID].ticker_frame = framenum;
	}
	else
	{
		ent->client->ps.stats[STAT_TICKER] = 0;
		ent->client->ps.stats[STAT_TICKER_OFFSET] = 0; 
	}
}


// find key 'sub' in 'line' and return its value
// returns false upon syntax error
qboolean ticker_parse_getparam (char *line, char *sub, char **value)
{
	char *i;
	char *j;
	static char tmp[MAX_STRING_CHARS];

	tmp[0] = '\0';
	i = strstr (line, sub);
	j = (i)? strstr(line, "text"):NULL;
	*value = tmp;

	if (j == NULL || i <= j)  // if the first occurance of "sub" is before
	{					     // the "text=" (ie not in the message)
		if (i == NULL)
			return true;
		else
		{
			for (i += strlen (sub); *i && isspace (*i); i++); // skip spaces
			if (*i == '\0' || *i != '=')	 // signal error if can't find '='
				return false;
			else
			{
				i++;	// one spot after the "="
				if (strcmp (sub, "text") == 0)
				{
					for (; *i && isspace (*i); i++); // skip spaces
					strcpy (tmp, i); 
					// trim trailing spaces
					for (j = tmp + strlen (tmp) - 1; j >= tmp && isspace (*j); j--);
					*(++j) = '\0';
				}
				else
				{
					for (; *i && isspace (*i); i++); // skip spaces
					if (*i == '\0')		// signal error if no value found
						return false;
					strcpy (tmp, i); 
					// trim trailing spaces
					for (j = tmp; *j && !isspace (*j); j++);
					*j = '\0';
				}
				*value = tmp;
				return true;
			}
		}
	}
	else
		return true;

}


qboolean ticker_parse_script (char *line)
{
	char *tmp;
	ticker_t *ticker_loop;

	// ignore blank lines and remarks (lines that start with "//")
	for (; *line && isspace (*line); line++); // skip spaces
	if (strlen (line) == 0 || strncmp (line, "//", 2) == 0)
		return true;

	// create new ticker function
	if (ticker == NULL)
	{
		ticker = ticker_current = gci.TagMalloc (sizeof (ticker_t), TAG_GAME);
		ticker_loop = NULL;
	}
	else
	{
		// propagate position of last do statement
		// so we can nest do/repeat statements
		if (ticker_current->func == TICKER_SCRIPT_REPEAT)
			ticker_loop = ticker_current->loop->loop; // propagate before-last do
		else if (ticker_current->func == TICKER_SCRIPT_DO)
			ticker_loop = ticker_current;	// propagate new do
		else
			ticker_loop = ticker_current->loop; // propagate last do
		ticker_current->next = gci.TagMalloc (sizeof (ticker_t), TAG_GAME);
		ticker_current = ticker_current->next;
	}
	// assign the defaults
	ticker_current->func = TICKER_SCRIPT_UNKNOWN;
	ticker_current->delay = 0;
	ticker_current->counter = 0;
	ticker_current->startspace = 0;
	ticker_current->endspace = 0;
	ticker_current->offset = 0;
	ticker_current->place = 0;
	ticker_current->place_target = 0;
	ticker_current->times = -1;
	ticker_current->remaining = 0;
	ticker_current->centered = false;
	ticker_current->text[0] = '\0';
	strcpy (ticker_current->store, "text not specified");
//	ticker_current->script[0] = '\0';
	ticker_current->loop = ticker_loop;
	ticker_current->next = NULL;


	// read all params (does NOT signal error if param is not relevant!)
	if (ticker_parse_getparam (line, "delay", &tmp))
	{
		ticker_current->delay = ticker_current->counter = atoi (tmp);
	}
	else
		return false;
	if (ticker_parse_getparam (line, "clear", &tmp))
	{
		if (strcmp (tmp, "true") == 0)
		{
			ticker_current->centered = true;
			ticker_current->store[0] = '\0';
		}
		else
		{
			if (ticker_parse_getparam (line, "center", &tmp))
			{
				if (strcmp (tmp, "true") == 0)
					ticker_current->centered = true;
				else
				{
					ticker_current->centered = false;
					if (ticker_parse_getparam (line, "startspace", &tmp))
					{
						ticker_current->startspace = atoi (tmp);
						if (ticker_current->startspace < 0)
							ticker_current->startspace = 0;
						if (ticker_current->startspace > TICKER_MAX_CHARS)
							ticker_current->startspace = TICKER_MAX_CHARS;
					}
					else
						return false;
					if (ticker_parse_getparam (line, "endspace", &tmp))
					{
						ticker_current->endspace = atoi (tmp);
						if (ticker_current->endspace < 0)
							ticker_current->endspace = 0;
						if (ticker_current->endspace > TICKER_MAX_CHARS)
							ticker_current->endspace = TICKER_MAX_CHARS;
					}
					else
						return false;
				}
			}
			else
				return false;
			if (ticker_parse_getparam (line, "text", &tmp))
				strcpy (ticker_current->store, tmp);
			else
				return false;
		}
	}
	else
		return false;
	if (ticker_parse_getparam (line, "times", &tmp))
	{
		ticker_current->times = atoi (tmp);
		ticker_current->remaining = ticker_current->times;
	}
	else
		return false;
//	if (ticker_parse_getparam (line, "script", &tmp))
//		strcpy (ticker_current->script, tmp);	// for chaining
//	else
//		return false;

	// set the function number
	// isolate function name
	for (tmp = line; *tmp && !isspace (*tmp); tmp++);
	*tmp = '\0';

	if (strcmp (line, "Appear") == 0)
		ticker_current->func = TICKER_SCRIPT_APPEAR;
	else if (strcmp (line, "Sleep") == 0)
		ticker_current->func = TICKER_SCRIPT_SLEEP;
	else if (strcmp (line, "ScrollLeft") == 0)
		ticker_current->func = TICKER_SCRIPT_SCROLLLEFT;
	else if (strcmp (line, "ScrollRight") == 0)
		ticker_current->func = TICKER_SCRIPT_SCROLLRIGHT;
//	else if (strcmp (line, "Random") == 0)
//		ticker_current->func = TICKER_SCRIPT_RANDOM;
	else if (strcmp (line, "Blink") == 0)
	{
		ticker_current->func = TICKER_SCRIPT_BLINK;
		if (ticker_current->times < 1)
			ticker_current->times = 1;
		ticker_current->times *= 2;
	}
//	else if (strcmp (line, "ScrollCenter") == 0)
//		ticker_current->func = TICKER_SCRIPT_SCROLLCENTER;
	else if (strcmp (line, "Do") == 0)
	{
		ticker_dos++;
		ticker_current->func = TICKER_SCRIPT_DO;	// this marks a place for the "repeats" to go back to
	}
	else if (strcmp (line, "Repeat") == 0)
	{
		ticker_dos--;
		ticker_current->func = TICKER_SCRIPT_REPEAT;
		if (ticker_dos < 0) // too many repeat statements
			return false;
	}
	else if (strcmp (line, "Reload") == 0)
		ticker_current->func = TICKER_SCRIPT_RELOAD;
//	else if (strcmp (line, "Chain") == 0)
//		ticker_current->func = TICKER_SCRIPT_CHAIN;
	return (ticker_current->func != TICKER_SCRIPT_UNKNOWN);
}


qboolean ticker_script (char *ticker_filename)
{
	int linenum = 0;
	char line[MAX_STRING_CHARS];
	FILE *rfp = NULL;
	char ticker_path[MAX_OSPATH];
	qboolean ticker_ok = true;
	qboolean done = false;

	ticker_shutdown (); 

	if (ticker_filename)
	{
		sprintf (ticker_path, "%s/%s/%s.led", 
				 basedir->string,
				 (strlen (game->string))? game->string:"baseq2",
				 ticker_filename);
		gci.dprintf ("loading ticker script \"%s\" ... ", ticker_path);
		rfp = fopen (ticker_path, "rt");
		if (rfp == NULL)
		{
			gci.dprintf ("failed\nticker_script: can't open file\n");
			return false;
		}
	}
	else
		gci.dprintf ("parsing default ticker script ... ");
	while (ticker_ok && !done)
	{
		if (ticker_filename)
			done = (fgets (line, MAX_STRING_CHARS, rfp) == NULL);
		else 
			done = (strlen (strcpy(line, ticker_default[linenum])) == 0);
		if (!done)
		{
			ticker_ok = ticker_parse_script (line);
			linenum++;
		}
	}
	if (rfp)
		fclose (rfp);
	if (ticker_ok && ticker_dos == 0)
	{
		ticker_current = ticker;
		gci.dprintf ("ok\n");
	}
	else
	{
		ticker_shutdown ();
		gci.dprintf ("failed\nticker_script: error at line %d\n", linenum);
	}
	return ticker_ok;
}


void ticker_init (void)
{
	qboolean ticker_file = false;

	ticker_text[0] = '\0';
	ticker_offset = 0;
	if (gc_ticker->string && strlen (gc_ticker->string))
		ticker_file = ticker_script (gc_ticker->string);
	if (!ticker_file)
		ticker_script (NULL);
	ticker_current = ticker;
}


void ticker_shutdown (void)
{
	ticker_dos = 0;

	while (ticker)
	{
		ticker_current = ticker;
		ticker = ticker->next;
		gci.TagFree (ticker_current);
	}
	ticker_current = NULL;
}


void ticker_update_camera (edict_t *ent, camera_t *camera)
{
	int offset_hi, offset_lo;
	char camera_location[MAX_INFO_VALUE];

	if (clients[numEdict(ent) - 1].score || intermission)
		return;

	if (camera == NULL)
	{
		ent->client->ps.stats[STAT_CAM_OFFSET] = 0;
		ent->client->ps.stats[STAT_CAM_LOCATION] = 0;
		return;
	}

	// generate camera name
	offset_hi = strlen (camera->name);
	if (offset_hi > 32)
		offset_hi = 32;
	offset_lo = offset_hi % 2;
	offset_hi = (32 - offset_hi) / 2;
	strncpy (camera_location, ticker_spaces, offset_hi);
	camera_location[offset_hi] = '\0';
	strncat (camera_location, camera->name, 32);
	camera_location[32] = '\0';
	highlightText (camera_location);
	// update HUD
	ent->client->ps.stats[STAT_CAM_OFFSET] = offset_lo;
	ent->client->ps.stats[STAT_CAM_LOCATION] = CS_CAM_LOCATION;
	gci.WriteByte (svc_configstring);
	gci.WriteShort (CS_CAM_LOCATION);
	gci.WriteString (camera_location);
	gci.unicast (ent, true);
}


void ticker_remove_statusbar (edict_t *ent)
{
	int clientID;

	clientID = numEdict (ent) - 1;

	if (!clients[clientID].statusbar_removed)
	{
		gci.WriteByte (svc_configstring);
		gci.WriteShort (CS_STATUSBAR);
		gci.WriteString ("");
		gci.unicast (ent, true);
		clients[clientID].statusbar_removed = true;
	}
}


void ticker_restore_statusbar (edict_t *ent)
{
	int clientID;

	clientID = numEdict (ent) - 1;

	if (clients[clientID].statusbar_removed)
	{
		gci.WriteByte (svc_configstring);
		gci.WriteShort (CS_STATUSBAR);
		gci.WriteString (camera_statusbar);
		gci.unicast (ent, true);
		ticker_update_camera (ent, clients[clientID].camera);
		clients[clientID].statusbar_removed = false;
	}
}

