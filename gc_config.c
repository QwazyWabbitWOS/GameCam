// GameCam v1.02 - camera proxy game module for Quake II
// Copyright (c) 1998-99, by Avi Rozen (Zung!)
// e-mail: zungbang@telefragged.com

// gc_config.c		track config string database

#include "gamecam.h"

config_strings_t ConfigStrings;

void configstring(int num, char* string)
{
	unsigned int i;

	if (string == NULL)
		ConfigStrings[num][0] = '\0';
	else
		for (i = 0; i < strlen(string); i += MAX_QPATH)
			strncpy(ConfigStrings[num + i / MAX_QPATH], &string[i], MAX_QPATH);
	gci.configstring(num, string);
}


void setmodel(edict_t* ent, char* name)
{
	int num;

	gci.setmodel(ent, name);
	num = gci.modelindex(name);
	if (ConfigStrings[CS_MODELS + num][0] == '\0')
		if (name != NULL)
			strncpy(ConfigStrings[CS_MODELS + num], name, MAX_QPATH);
}


int modelindex(char* name)
{
	int num;

	num = gci.modelindex(name);
	if (ConfigStrings[CS_MODELS + num][0] == '\0')
		if (name != NULL)
			strncpy(ConfigStrings[CS_MODELS + num], name, MAX_QPATH);
	return num;
}

/*
int	soundindex (char *name)
{
	int num;

	num = gci.soundindex (name);
	if (ConfigStrings[CS_SOUNDS + num][0] == '\0')
		if (name != NULL)
			strncpy (ConfigStrings[CS_SOUNDS + num], name, MAX_QPATH);
	return num;
}

int imageindex (char *name)
{
	int num;

	num = gci.imageindex (name);
	if (ConfigStrings[CS_IMAGES + num][0] == '\0')
		if (name != NULL)
			strncpy (ConfigStrings[CS_IMAGES + num], name, MAX_QPATH);
	return num;
}
*/

