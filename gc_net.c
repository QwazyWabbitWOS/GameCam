// GameCam v1.02 - camera proxy game module for Quake II
// Copyright (c) 1998-99, by Avi Rozen (Zung!)
// e-mail: zungbang@telefragged.com

// gc_net.c		gamecam network emulation

#include "gamecam.h"

write_buffer_t write_buffer;
char first_print_message[MAX_STRING_CHARS];


void CreateBufferEntry(int type, void *buffer,long size)
{
	if ((write_buffer.length + size) >= MAX_BUF_DATA) 
	{
		gci.error("CreateBufferEntry - buffer overrun");
		return;
	}
	write_buffer.type[write_buffer.entries]=type | ((buffer==NULL)? WRITE_BUF_NULL:0);
	if (buffer != NULL)
		memcpy((void *) &(write_buffer.data[write_buffer.length]),buffer,size);
	else
		memset((void *) &(write_buffer.data[write_buffer.length]),0,size);
	write_buffer.entries++;
	write_buffer.length += size;
}


void AddToBuffer(int type, void *buffer)
{
	switch (type) 
	{
	case WRITE_BUF_CHAR:
		CreateBufferEntry(type,buffer,sizeof(char));
		break;
	case WRITE_BUF_BYTE:
		CreateBufferEntry(type,buffer,sizeof(byte));
		break;
	case WRITE_BUF_SHORT:
		CreateBufferEntry(type,buffer,sizeof(short));
		break;
	case WRITE_BUF_LONG:
		CreateBufferEntry(type,buffer,sizeof(long));
		break;
	case WRITE_BUF_FLOAT:
		CreateBufferEntry(type,buffer,sizeof(float));
		break;
	case WRITE_BUF_STRING:
		CreateBufferEntry(type,buffer,strlen((char *) buffer)+1);
		break;
	case WRITE_BUF_POSITION:
		CreateBufferEntry(type,buffer,sizeof(vec3_t));
		break;
	case WRITE_BUF_DIR:
		CreateBufferEntry(type,buffer,sizeof(vec3_t));
		break;
	case WRITE_BUF_ANGLE:
		CreateBufferEntry(type,buffer,sizeof(float));
		break;
	default:
		gci.error("AddToBuffer - unknown data type %d",type);
		break;
	}

}

void WriteBuffer(void)
{
	long i, bufpos=0;

	char	cbuf;
	byte	bbuf;
	short	sbuf;
	long	lbuf;
	float f;
	vec3_t pos;

	for (i=0; i<write_buffer.entries; i++) 
		switch (write_buffer.type[i]) 
	{
		case WRITE_BUF_CHAR:
			memcpy((void *) &cbuf,(void *) &(write_buffer.data[bufpos]),sizeof(char));
			bufpos += sizeof(char);
			gci.WriteChar((int) cbuf);
			break;
		case WRITE_BUF_BYTE:
			memcpy((void *) &bbuf,(void *) &(write_buffer.data[bufpos]),sizeof(byte));
			bufpos += sizeof(byte);
			gci.WriteByte((int) bbuf);
			break;
		case WRITE_BUF_SHORT:
			memcpy((void *) &sbuf,(void *) &(write_buffer.data[bufpos]),sizeof(short));
			bufpos += sizeof(short);
			gci.WriteShort((int) sbuf);
			break;
		case WRITE_BUF_LONG:
			memcpy((void *) &lbuf,(void *) &(write_buffer.data[bufpos]),sizeof(long));
			bufpos += sizeof(long);
			gci.WriteLong((int) lbuf);
			break;
		case WRITE_BUF_FLOAT:
			memcpy((void *) &f,(void *) &(write_buffer.data[bufpos]),sizeof(float));
			bufpos += sizeof(float);
			gci.WriteFloat(f);
			break;
		case WRITE_BUF_STRING:
			gci.WriteString((char *) &(write_buffer.data[bufpos]));
			bufpos += strlen((char *) &(write_buffer.data[bufpos])) + 1;
			break;
		case WRITE_BUF_STRING | WRITE_BUF_NULL:
			gci.WriteString(NULL);
			break;
		case WRITE_BUF_POSITION:
			memcpy((void *) pos,(void *) &(write_buffer.data[bufpos]),sizeof(vec3_t));
			bufpos += sizeof(vec3_t);
			gci.WritePosition(pos);
			break;
		case WRITE_BUF_POSITION | WRITE_BUF_NULL:
			gci.WritePosition(NULL);
			bufpos += sizeof(vec3_t); 
			break;
		case WRITE_BUF_DIR:
			memcpy((void *) pos,(void *) &(write_buffer.data[bufpos]),sizeof(vec3_t));
			bufpos += sizeof(vec3_t);
			gci.WriteDir(pos);
			break;
		case WRITE_BUF_DIR | WRITE_BUF_NULL:
			gci.WriteDir(NULL);
			bufpos += sizeof(vec3_t); 
			break;
		case WRITE_BUF_ANGLE:
			memcpy((void *) &f,(void *) &(write_buffer.data[bufpos]),sizeof(float));
			bufpos += sizeof(float);
			gci.WriteAngle(f);
			break;
		}
}


void ClearBuffer(void)
{
	write_buffer.length = 0;
	write_buffer.entries = 0;
}

void WriteChar (int c)
{
	char cbuf;

	cbuf = (char) c;
	AddToBuffer(WRITE_BUF_CHAR,(void *) &cbuf);
	//gci.WriteChar (c);
}

void WriteByte (int c)
{
	byte bbuf;

	bbuf = (byte) c;
	AddToBuffer(WRITE_BUF_BYTE,(void *) &bbuf); 
	//gci.WriteByte (c);
}

void WriteShort (int c)
{
	short sbuf;

	sbuf = (short) c;
	AddToBuffer(WRITE_BUF_SHORT,(void *) &sbuf); 
	//gci.WriteShort (c);
}

void WriteLong (int c)
{
	long lbuf;

	lbuf = (long) c;
	AddToBuffer(WRITE_BUF_LONG,(void *) &lbuf); 
	//gci.WriteLong (c);
}

void WriteFloat (float f)
{
	AddToBuffer(WRITE_BUF_FLOAT,(void *) &f);
	//gci.WriteFloat (f);
}

void WriteString (char *s)
{
	AddToBuffer(WRITE_BUF_STRING,(void *) s);
	//gci.WriteString (s);
}

void WritePosition (vec3_t pos)
{
	AddToBuffer(WRITE_BUF_POSITION,(void *) pos);
	//gci.WritePosition (pos);
}

void WriteDir (vec3_t pos)
{
	AddToBuffer(WRITE_BUF_DIR,(void *) pos);
	//gci.WriteDir (pos);
}

void WriteAngle (float f)
{
	AddToBuffer(WRITE_BUF_ANGLE,(void *) &f);
	//gci.WriteAngle (f);
}


void multicast (vec3_t origin, multicast_t to)
{
	WriteBuffer();
	gci.multicast(origin,to);
	ClearBuffer();
}


void unicast (edict_t *ent, qboolean reliable)
{
	int clientID, cameraID;
	qboolean duplicate_score;

	clientID = numEdict(ent) - 1;

	if (wait_inven == ent && 
		write_buffer.data[0] == svc_inventory)  // GameCam issued inven
	{
		WriteBuffer();
		gci.unicast (wait_camera, reliable);
		cameraID = numEdict(wait_camera) - 1;
		memcpy (clients[cameraID].items, &write_buffer.data[1], MAX_ITEMS * sizeof(short));
		ClearBuffer();
		return;
	}

	if (wait_score == ent && 
		write_buffer.data[0] == svc_layout)  // GameCam issued score
	{
		cameraID = numEdict(wait_camera) - 1;
		if (clients[cameraID].last_score != framenum)  // prevent overflow
		{
			WriteBuffer();
			gci.unicast (wait_camera, reliable);
			clients[cameraID].last_score = framenum;
		}
		ClearBuffer();
		return;
	}

	if (wait_help == ent && 
		write_buffer.data[0] == svc_layout)  // GameCam issued help
	{
				cameraID = numEdict(wait_camera) - 1;
		if (clients[cameraID].last_score != framenum)  // prevent overflow
		{
			WriteBuffer();
			gci.unicast (wait_camera, reliable);
			clients[cameraID].last_score = framenum;
		}
		ClearBuffer();
		return;
	}

	// save layout and inventory
	if (write_buffer.data[0] == svc_layout) 
	{
		strcpy (clients[clientID].layout, (char *) &write_buffer.data[1]);
		// check for duplicate score on this frame
		// (also if client is in limbo)
		duplicate_score = (clients[clientID].last_score == framenum ||  clients[clientID].limbo_time != 0);
		clients[clientID].last_score = framenum;
	} else
		duplicate_score = QFALSE;

	if (write_buffer.data[0] == svc_inventory)
		memcpy (clients[clientID].items, &write_buffer.data[1], MAX_ITEMS * sizeof(short));

	if (!duplicate_score)  // prevent overflow
	{
		WriteBuffer();
		gci.unicast (ent,reliable);
	}
	ClearBuffer();
}


void cprintf (edict_t *ent, int printlevel, char *fmt, ...) 
{
	int clientID, targetID;
	va_list ap = "";
	char print_message[MAX_STRING_CHARS];

	va_start(ap, fmt);
	vsprintf(print_message, fmt, ap);
	//QW// catch malicious input from client console
	if (strstr(print_message, "%n") || strstr(print_message, "%s"))
	{
		sprintf(print_message, "Bad input\n"); 
	}
	if (ent && wait_cprintf == ent) 
		strcpy(captured_print_message, print_message);
	else 
	{
		gci.cprintf(ent,printlevel,print_message);
		if (printlevel == PRINT_CHAT && strcmp (first_print_message, print_message)) 
		{
			strcpy (first_print_message, print_message);
			for (clientID = 0; clientID < maxclients->value; clientID++) 
				if (clients[clientID].inuse &&
					clients[clientID].spectator &&
					clients[clientID].mode != CAMERA_CHASE)
					gci.cprintf(Edict(clientID + 1),printlevel,first_print_message);
		}
		if (ent != NULL) 
		{
			targetID = numEdict(ent) - 1;
			for (clientID = 0; clientID < maxclients->value; clientID++) 
			{
				if (clients[clientID].inuse &&
					clients[clientID].spectator &&
					clients[clientID].mode == CAMERA_CHASE &&
					clients[clientID].target == targetID)
					gci.cprintf(Edict(clientID + 1),printlevel,print_message);
			}
		}
	}
	va_end(ap);
}


void centerprintf (edict_t *ent, char *fmt, ...)
{
	int clientID, targetID;
	va_list ap;
	char print_message[MAX_STRING_CHARS];

	va_start(ap, fmt);
	vsprintf(print_message,fmt,ap);
	gci.centerprintf(ent,print_message);
	if (ent != NULL) 
	{
		targetID = numEdict(ent) - 1;
		for (clientID = 0; clientID < maxclients->value; clientID++) 
		{
			if (clients[clientID].inuse &&
				clients[clientID].spectator &&
				clients[clientID].mode == CAMERA_CHASE &&
				clients[clientID].target == targetID)
				gci.centerprintf(Edict(clientID + 1),print_message);
		}
	}
	va_end(ap);
}

