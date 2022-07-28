/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// screen.c -- master for refresh, status bar, console, chat, notify, etc

#include "quakedef.h"

/*

background clear
rendering
turtle/net/ram icons
sbar
centerprint / slow centerprint
notify lines
intermission / finale overlay
loading plaque
console
menu

required background clears
required update regions


syncronous draw mode or async
One off screen buffer, with updates either copied or xblited
Need to double buffer?


async draw will require the refresh area to be cleared, because it will be
xblited, but sync draw can just ignore it.

sync
draw

CenterPrint ()
SlowPrint ()
Screen_Update ();
Con_Printf ();

net
turn off messages option

the refresh is allways rendered, unless the console is full screen


console is:
	notify lines
	half
	full

*/


int			glx, gly, glwidth, glheight;

float		scr_con_current;
float		scr_conlines;		// lines of console to display

//johnfitz -- new cvars
cvar_t		scr_menuscale = {"scr_menuscale", "1", CVAR_ARCHIVE};
cvar_t		scr_sbarscale = {"scr_sbarscale", "1", CVAR_ARCHIVE};
cvar_t		scr_sbaralpha = {"scr_sbaralpha", "0.75", CVAR_ARCHIVE};
cvar_t		scr_conwidth = {"scr_conwidth", "0", CVAR_ARCHIVE};
cvar_t		scr_conscale = {"scr_conscale", "1", CVAR_ARCHIVE};
cvar_t		scr_crosshairscale = {"scr_crosshairscale", "1", CVAR_ARCHIVE};
cvar_t		scr_showfps = {"scr_showfps", "0", CVAR_NONE};
cvar_t		scr_clock = {"scr_clock", "0", CVAR_NONE};
cvar_t		scr_extendedhud = {"scr_extendedhud", "0", CVAR_ARCHIVE};
cvar_t		scr_extendedhud_loads = {"scr_extendedhud_loads", "0", CVAR_NONE};
cvar_t		radar_monsters = {"radar_monsters", "0", CVAR_NONE};
cvar_t		radar_secrets = {"radar_secrets", "0", CVAR_NONE};
cvar_t		radar_scale = {"radar_scale", "1", CVAR_ARCHIVE};
cvar_t		scr_speed = {"scr_speed", "0", CVAR_ARCHIVE};
cvar_t		scr_speed_scale = {"scr_speed_scale", "4", CVAR_ARCHIVE};
cvar_t		scr_speed_history = {"scr_speed_history", "100", CVAR_ARCHIVE};
cvar_t		scr_speed_angles = {"scr_speed_angles", "180", CVAR_ARCHIVE};
cvar_t		scr_speed_minspeed = {"scr_speed_minspeed", "no", CVAR_ARCHIVE};
cvar_t		scr_speed_maxspeed = {"scr_speed_maxspeed", "no", CVAR_ARCHIVE};
cvar_t		scr_speed_scale_minspeed = {"scr_speed_scale_minspeed", "no", CVAR_ARCHIVE};
cvar_t		scr_speed_scale_maxspeed = {"scr_speed_scale_maxspeed", "no", CVAR_ARCHIVE};

//johnfitz
cvar_t		scr_usekfont = {"scr_usekfont", "0", CVAR_NONE}; // 2021 re-release

cvar_t		scr_viewsize = {"viewsize","100", CVAR_ARCHIVE};
cvar_t		scr_fov = {"fov","90",CVAR_NONE};	// 10 - 170
cvar_t		scr_fov_adapt = {"fov_adapt","1",CVAR_ARCHIVE};
cvar_t		scr_conspeed = {"scr_conspeed","500",CVAR_ARCHIVE};
cvar_t		scr_centertime = {"scr_centertime","2",CVAR_NONE};
cvar_t		scr_showturtle = {"showturtle","0",CVAR_NONE};
cvar_t		scr_showpause = {"showpause","1",CVAR_NONE};
cvar_t		scr_printspeed = {"scr_printspeed","8",CVAR_NONE};
cvar_t		gl_triplebuffer = {"gl_triplebuffer", "1", CVAR_ARCHIVE};

cvar_t		cl_gun_fovscale = {"cl_gun_fovscale","1",CVAR_ARCHIVE}; // Qrack

extern	cvar_t	crosshair;

qboolean	scr_initialized;		// ready to draw

qpic_t		*scr_net;
qpic_t		*scr_turtle;

int			clearconsole;
int			clearnotify;

vrect_t		scr_vrect;

qboolean	scr_disabled_for_loading;
qboolean	scr_drawloading;
float		scr_disabled_time;

int	scr_tileclear_updates = 0; //johnfitz

void SCR_ScreenShot_f (void);

extern void GetEdictCenter(edict_t *ed, vec3_t pos);

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;	// for slow victory printing
float		scr_centertime_off;
int			scr_center_lines;
int			scr_erase_lines;
int			scr_erase_center;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (const char *str) //update centerprint data
{
	strncpy (scr_centerstring, str, sizeof(scr_centerstring)-1);
	scr_centertime_off = scr_centertime.value;
	scr_centertime_start = cl.time;

// count the number of lines for centering
	scr_center_lines = 1;
	str = scr_centerstring;
	while (*str)
	{
		if (*str == '\n')
			scr_center_lines++;
		str++;
	}
}

void SCR_DrawCenterString (void) //actually do the drawing
{
	char	*start;
	int		l;
	int		j;
	int		x, y;
	int		remaining;

	GL_SetCanvas (CANVAS_MENU); //johnfitz

// the finale prints the characters one at a time
	if (cl.intermission)
		remaining = scr_printspeed.value * (cl.time - scr_centertime_start);
	else
		remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	if (scr_center_lines <= 4)
		y = 200*0.35;	//johnfitz -- 320x200 coordinate system
	else
		y = 48;
	if (crosshair.value)
		y -= 8;

	do
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (320 - l*8)/2;	//johnfitz -- 320x200 coordinate system
		for (j=0 ; j<l ; j++, x+=8)
		{
			Draw_Character (x, y, start[j]);	//johnfitz -- stretch overlays
			if (!remaining--)
				return;
		}

		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

void SCR_CheckDrawCenterString (void)
{
	if (scr_center_lines > scr_erase_lines)
		scr_erase_lines = scr_center_lines;

	scr_centertime_off -= host_frametime;

	if (scr_centertime_off <= 0 && !cl.intermission)
		return;
	if (key_dest != key_game)
		return;
	if (cl.paused) //johnfitz -- don't show centerprint during a pause
		return;

	SCR_DrawCenterString ();
}

//=============================================================================

/*
====================
AdaptFovx
Adapt a 4:3 horizontal FOV to the current screen size using the "Hor+" scaling:
2.0 * atan(width / height * 3.0 / 4.0 * tan(fov_x / 2.0))
====================
*/
float AdaptFovx (float fov_x, float width, float height)
{
	float	a, x;

	if (fov_x < 1 || fov_x > 179)
		Sys_Error ("Bad fov: %f", fov_x);

	if (!scr_fov_adapt.value)
		return fov_x;
	if ((x = height / width) == 0.75)
		return fov_x;
	a = atan(0.75 / x * tan(fov_x / 360 * M_PI));
	a = a * 360 / M_PI;
	return a;
}

/*
====================
CalcFovy
====================
*/
float CalcFovy (float fov_x, float width, float height)
{
	float	a, x;

	if (fov_x < 1 || fov_x > 179)
		Sys_Error ("Bad fov: %f", fov_x);

	x = width / tan(fov_x / 360 * M_PI);
	a = atan(height / x);
	a = a * 360 / M_PI;
	return a;
}

/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/
static void SCR_CalcRefdef (void)
{
	float		size, scale; //johnfitz -- scale

// force the status bar to redraw
	Sbar_Changed ();

	scr_tileclear_updates = 0; //johnfitz

// bound viewsize
	if (scr_viewsize.value < 30)
		Cvar_SetQuick (&scr_viewsize, "30");
	if (scr_viewsize.value > 120)
		Cvar_SetQuick (&scr_viewsize, "120");

// bound fov
	if (scr_fov.value < 10)
		Cvar_SetQuick (&scr_fov, "10");
	if (scr_fov.value > 170)
		Cvar_SetQuick (&scr_fov, "170");

	vid.recalc_refdef = 0;

	//johnfitz -- rewrote this section
	size = scr_viewsize.value;
	scale = CLAMP (1.0f, scr_sbarscale.value, (float)glwidth / 320.0f);

	if (size >= 120 || cl.intermission || scr_sbaralpha.value < 1) //johnfitz -- scr_sbaralpha.value
		sb_lines = 0;
	else if (size >= 110)
		sb_lines = 24 * scale;
	else
		sb_lines = 48 * scale;

	size = q_min(scr_viewsize.value, 100.f) / 100;
	//johnfitz

	//johnfitz -- rewrote this section
	r_refdef.vrect.width = q_max(glwidth * size, 96.0f); //no smaller than 96, for icons
	r_refdef.vrect.height = q_min((int)(glheight * size), glheight - sb_lines); //make room for sbar
	r_refdef.vrect.x = (glwidth - r_refdef.vrect.width)/2;
	r_refdef.vrect.y = (glheight - sb_lines - r_refdef.vrect.height)/2;
	//johnfitz

	r_refdef.fov_x = AdaptFovx(scr_fov.value, vid.width, vid.height);
	r_refdef.fov_y = CalcFovy (r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height);

	scr_vrect = r_refdef.vrect;
}


/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f (void)
{
	Cvar_SetValueQuick (&scr_viewsize, scr_viewsize.value+10);
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f (void)
{
	Cvar_SetValueQuick (&scr_viewsize, scr_viewsize.value-10);
}

static void SCR_Callback_refdef (cvar_t *var)
{
	vid.recalc_refdef = 1;
}

/*
==================
SCR_Conwidth_f -- johnfitz -- called when scr_conwidth or scr_conscale changes
==================
*/
void SCR_Conwidth_f (cvar_t *var)
{
	vid.recalc_refdef = 1;
	vid.conwidth = (scr_conwidth.value > 0) ? (int)scr_conwidth.value : (scr_conscale.value > 0) ? (int)(vid.width/scr_conscale.value) : vid.width;
	vid.conwidth = CLAMP (320, vid.conwidth, vid.width);
	vid.conwidth &= 0xFFFFFFF8;
	vid.conheight = vid.conwidth * vid.height / vid.width;
}

//============================================================================

/*
==================
SCR_LoadPics -- johnfitz
==================
*/
void SCR_LoadPics (void)
{
	scr_net = Draw_PicFromWad ("net");
	scr_turtle = Draw_PicFromWad ("turtle");
}

/*
==================
SCR_Init
==================
*/
void SCR_Init (void)
{
	//johnfitz -- new cvars
	Cvar_RegisterVariable (&scr_menuscale);
	Cvar_RegisterVariable (&scr_sbarscale);
	Cvar_SetCallback (&scr_sbaralpha, SCR_Callback_refdef);
	Cvar_RegisterVariable (&scr_sbaralpha);
	Cvar_SetCallback (&scr_conwidth, &SCR_Conwidth_f);
	Cvar_SetCallback (&scr_conscale, &SCR_Conwidth_f);
	Cvar_RegisterVariable (&scr_conwidth);
	Cvar_RegisterVariable (&scr_conscale);
	Cvar_RegisterVariable (&scr_crosshairscale);
	Cvar_RegisterVariable (&scr_showfps);
	Cvar_RegisterVariable (&scr_clock);

        Cvar_RegisterVariable (&scr_extendedhud);
        Cvar_RegisterVariable (&scr_extendedhud_loads);
        Cvar_RegisterVariable (&radar_monsters);
        Cvar_RegisterVariable (&radar_secrets);
        Cvar_RegisterVariable (&radar_scale);
        Cvar_RegisterVariable (&scr_speed);
        Cvar_RegisterVariable (&scr_speed_scale);
        Cvar_RegisterVariable (&scr_speed_history);
        Cvar_RegisterVariable (&scr_speed_angles);
        Cvar_RegisterVariable (&scr_speed_minspeed);
        Cvar_RegisterVariable (&scr_speed_maxspeed);
        Cvar_RegisterVariable (&scr_speed_scale_minspeed);
        Cvar_RegisterVariable (&scr_speed_scale_maxspeed);

	//johnfitz
	Cvar_RegisterVariable (&scr_usekfont); // 2021 re-release
	Cvar_SetCallback (&scr_fov, SCR_Callback_refdef);
	Cvar_SetCallback (&scr_fov_adapt, SCR_Callback_refdef);
	Cvar_SetCallback (&scr_viewsize, SCR_Callback_refdef);
	Cvar_RegisterVariable (&scr_fov);
	Cvar_RegisterVariable (&scr_fov_adapt);
	Cvar_RegisterVariable (&scr_viewsize);
	Cvar_RegisterVariable (&scr_conspeed);
	Cvar_RegisterVariable (&scr_showturtle);
	Cvar_RegisterVariable (&scr_showpause);
	Cvar_RegisterVariable (&scr_centertime);
	Cvar_RegisterVariable (&scr_printspeed);
	Cvar_RegisterVariable (&gl_triplebuffer);
	Cvar_RegisterVariable (&cl_gun_fovscale);

	Cmd_AddCommand ("screenshot",SCR_ScreenShot_f);
	Cmd_AddCommand ("sizeup",SCR_SizeUp_f);
	Cmd_AddCommand ("sizedown",SCR_SizeDown_f);

	SCR_LoadPics (); //johnfitz

	scr_initialized = true;
}

//============================================================================

/*
==============
SCR_DrawFPS -- johnfitz
==============
*/
void SCR_DrawFPS (void)
{
	static double	oldtime = 0;
	static double	lastfps = 0;
	static int	oldframecount = 0;
	double	elapsed_time;
	int	frames;

	elapsed_time = realtime - oldtime;
	frames = r_framecount - oldframecount;

	if (elapsed_time < 0 || frames < 0)
	{
		oldtime = realtime;
		oldframecount = r_framecount;
		return;
	}
	// update value every 3/4 second
	if (elapsed_time > 0.75)
	{
		lastfps = frames / elapsed_time;
		oldtime = realtime;
		oldframecount = r_framecount;
	}

	if (scr_showfps.value)
	{
		char	st[16];
		int	x, y;
		sprintf (st, "%4.0f fps", lastfps);
		x = 320 - (strlen(st)<<3);
		y = 200 - 8;
		if (scr_clock.value) y -= 8; //make room for clock
		GL_SetCanvas (CANVAS_BOTTOMRIGHT);
		Draw_String (x, y, st);
		scr_tileclear_updates = 0;
	}
}

/*
==============
SCR_DrawClock -- johnfitz
==============
*/
void SCR_DrawClock (void)
{
	char	str[12];

	if (scr_clock.value == 1)
	{
		int minutes, seconds;

		minutes = cl.time / 60;
		seconds = ((int)cl.time)%60;

		sprintf (str,"%i:%i%i", minutes, seconds/10, seconds%10);
	}
	else
		return;

	//draw it
	GL_SetCanvas (CANVAS_BOTTOMRIGHT);
	Draw_String (320 - (strlen(str)<<3), 200 - 8, str);

	scr_tileclear_updates = 0;
}

/*
==============
SCR_DrawDevStats
==============
*/
void SCR_DrawDevStats (void)
{
	char	str[40];
	int		y = 25-9; //9=number of lines to print
	int		x = 0; //margin

	if (!devstats.value)
		return;

	GL_SetCanvas (CANVAS_BOTTOMLEFT);

	Draw_Fill (x, y*8, 19*8, 9*8, 0, 0.5); //dark rectangle

	sprintf (str, "devstats |Curr Peak");
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "---------+---------");
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Edicts   |%4i %4i", dev_stats.edicts, dev_peakstats.edicts);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Packet   |%4i %4i", dev_stats.packetsize, dev_peakstats.packetsize);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Visedicts|%4i %4i", dev_stats.visedicts, dev_peakstats.visedicts);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Efrags   |%4i %4i", dev_stats.efrags, dev_peakstats.efrags);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Dlights  |%4i %4i", dev_stats.dlights, dev_peakstats.dlights);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Beams    |%4i %4i", dev_stats.beams, dev_peakstats.beams);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Tempents |%4i %4i", dev_stats.tempents, dev_peakstats.tempents);
	Draw_String (x, (y++)*8-x, str);
}

/*
==============
SCR_DrawTurtle
==============
*/
void SCR_DrawTurtle (void)
{
	static int	count;

	if (!scr_showturtle.value)
		return;

	if (host_frametime < 0.1)
	{
		count = 0;
		return;
	}

	count++;
	if (count < 3)
		return;

	GL_SetCanvas (CANVAS_DEFAULT); //johnfitz

	Draw_Pic (scr_vrect.x, scr_vrect.y, scr_turtle);
}

/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void)
{
	if (realtime - cl.last_received_message < 0.3)
		return;
	if (cls.demoplayback)
		return;

	GL_SetCanvas (CANVAS_DEFAULT); //johnfitz

	Draw_Pic (scr_vrect.x+64, scr_vrect.y, scr_net);
}

/*
==============
DrawPause
==============
*/
void SCR_DrawPause (void)
{
	qpic_t	*pic;

	if (!cl.paused)
		return;

	if (!scr_showpause.value)		// turn off for screenshots
		return;

	GL_SetCanvas (CANVAS_MENU); //johnfitz

	pic = Draw_CachePic ("gfx/pause.lmp");
	Draw_Pic ( (320 - pic->width)/2, (240 - 48 - pic->height)/2, pic); //johnfitz -- stretched menus

	scr_tileclear_updates = 0; //johnfitz
}

/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading (void)
{
	qpic_t	*pic;

	if (!scr_drawloading)
		return;

	GL_SetCanvas (CANVAS_MENU); //johnfitz

	pic = Draw_CachePic ("gfx/loading.lmp");
	Draw_Pic ( (320 - pic->width)/2, (240 - 48 - pic->height)/2, pic); //johnfitz -- stretched menus

	scr_tileclear_updates = 0; //johnfitz
}

/*
==============
SCR_DrawCrosshair -- johnfitz
==============
*/
void SCR_DrawCrosshair (void)
{
	if (!crosshair.value)
		return;

	GL_SetCanvas (CANVAS_CROSSHAIR);
	Draw_Character (-4, -4, '+'); //0,0 is center of viewport
}

void SCR_DrawExtendedHud (void)
{
    char str[256];
    int	minutes, seconds, tens, units;

    if (!scr_extendedhud.value)
        return;

    // manual GL_SetCanvas to avoid changing multiple files
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
    float s = scr_extendedhud.value;
    if (s < 0) s = -s;
    glOrtho (0, 640, 400, 0, -99999, 99999);
    glViewport (glx, gly, 640*s, 400*s);

    minutes = cl.time / 60;
    seconds = cl.time - 60*minutes;
    tens = seconds / 10;
    units = seconds - 10*tens;
    if (scr_extendedhud.value < 0) {
        // 'A' = 65, fancy A = 193, 193-65 = 128
        sprintf (str,"%i:%i%i %c%c%i/%i %c%c%i/%i %c%c%c%c%c%c%i %c%c%c%c%s %c%c%c%c%c%c%i",
                 minutes, tens, units,
                 'K'+128, ':'+128, cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS],
                 'S'+128, ':'+128, cl.stats[STAT_SECRETS], cl.stats[STAT_TOTALSECRETS],
                 'S'+128, 'K'+128, 'I'+128, 'L'+128, 'L'+128, ':'+128, (int)(skill.value + 0.5),
                 'M'+128, 'A'+128, 'P'+128, ':'+128, cl.mapname,
                 'L'+128, 'O'+128, 'A'+128, 'D'+128, 'S'+128, ':'+128, (int)scr_extendedhud_loads.value);
        Draw_String (0, 400 - 8, str);
    } else {
        sprintf (str,"%i:%i%i %c%c%i/%i %c%c%i/%i",
                 minutes, tens, units,
                 'K'+128, ':'+128, cl.stats[STAT_MONSTERS], cl.stats[STAT_TOTALMONSTERS],
                 'S'+128, ':'+128, cl.stats[STAT_SECRETS], cl.stats[STAT_TOTALSECRETS]);
        Draw_String (0, 400 - 32, str);
        sprintf (str,"%c%c%c%c %s", 'M'+128, 'A'+128, 'P'+128, ':'+128, cl.mapname);
        Draw_String (0, 400 - 24, str);
        sprintf (str,"%c%c%c%c%c%c %i", 'S'+128, 'K'+128, 'I'+128, 'L'+128, 'L'+128, ':'+128,
                 (int)(skill.value + 0.5));
        Draw_String (0, 400 - 16, str);
        sprintf (str,"%c%c%c%c%c%c %i", 'L'+128, 'O'+128, 'A'+128, 'D'+128, 'S'+128, ':'+128,
                 (int)scr_extendedhud_loads.value);
        Draw_String (0, 400 - 8, str);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();
}

void SCR_DrawRadar (void)
{
    if (!radar_monsters.value && !radar_secrets.value)
        return;

    // manual GL_SetCanvas to avoid changing multiple files
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
    float s = radar_scale.value;
    glOrtho (0, 320, 200, 0, -99999, 99999);
    glViewport (glx+glwidth-320*s, gly+glheight-200*s, 320*s, 200*s);

    float org[3];
    org[0] = cl_entities[cl.viewentity].origin[0];
    org[1] = cl_entities[cl.viewentity].origin[1];
    org[2] = cl_entities[cl.viewentity].origin[2];

    int nearby_monster = 0;
    int nearby_secret = 0;
    int i;
    edict_t *ed;
    for (i=0, ed=NEXT_EDICT(sv.edicts) ; i<sv.num_edicts ; i++, ed=NEXT_EDICT(ed))
    {
        if (ed->free) continue;

        float pos[3];
        GetEdictCenter(ed, pos);

        float distsquared = (org[0]-pos[0])*(org[0]-pos[0])
            + (org[1]-pos[1])*(org[1]-pos[1])
            + (org[2]-pos[2])*(org[2]-pos[2]);

        const char* classname = PR_GetString(ed->v.classname);
        if (!nearby_secret && radar_secrets.value && distsquared <= radar_secrets.value*radar_secrets.value) {
            eval_t *estate = GetEdictFieldValue(ed, "estate");
            if (!estate || estate->_float != 2.0) { // hide ad disabled secret triggers
                if (strncmp(classname, "trigger_secret", strlen("trigger_secret")) == 0) {
                    nearby_secret = 1;
                }
            }
        }
        if (!nearby_monster && radar_monsters.value && distsquared <= radar_monsters.value*radar_monsters.value) {
            if (strncmp(classname, "monster_", strlen("monster_")) == 0) {
                if (GetEdictFieldValue(ed, "health")->_float > 0) {
                    eval_t *takedamage = GetEdictFieldValue(ed, "takedamage");
                    int teleports_in = ((int)ed->v.spawnflags & 8) // smp mod monster spawn-in
                        || ((int)ed->v.spawnflags & (1<<6)); // ad mod monster spawn-in
                    if (teleports_in || (takedamage && takedamage->_float >= 1)) { // exclude crucified zombies
                        nearby_monster = 1;
                    }
                }
            }
        }
    }

    int x = 320 - 8;
    if (nearby_monster) {
        Draw_Character(x, 0, 'M');
        x -= 8;
    }
    if (nearby_secret) {
        Draw_Character(x, 0, 'S');
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();
}

// copy-pasted from sv_user.c
float Get_Wishdir_Speed_Delta(float angle) {
    extern cvar_t sv_maxspeed;
    extern cvar_t sv_friction;
    extern cvar_t sv_edgefriction;
    extern cvar_t sv_stopspeed;
    extern cvar_t sv_accelerate;
    extern usercmd_t cmd;
    extern vec3_t velocitybeforethink;
    extern qboolean onground;

        int		i;
	vec3_t		wishvel, wishdir, velocity, angles, origin, forward, right, up;
	float		wishspeed, fmove, smove;

        if (angle < -180) angle += 360;
        if (angle > 180) angle -= 360;

        for (i=0;i<3;i++) angles[i] = sv_player->v.angles[i];
        angles[1] = angle;

	AngleVectors (angles, forward, right, up);

	fmove = cmd.forwardmove;
	smove = cmd.sidemove;

// hack to not let you back into teleporter
	if (sv.time < sv_player->v.teleport_time && fmove < 0)
		fmove = 0;

	for (i=0 ; i<3 ; i++)
		wishvel[i] = forward[i]*fmove + right[i]*smove;

	if ( (int)sv_player->v.movetype != MOVETYPE_WALK)
		wishvel[2] = cmd.upmove;
	else
		wishvel[2] = 0;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	if (wishspeed > sv_maxspeed.value)
	{
		VectorScale (wishvel, sv_maxspeed.value/ wishspeed, wishvel);
		wishspeed = sv_maxspeed.value;
	}

        for (i=0;i<3;i++) velocity[i] = velocitybeforethink[i];
        for (i=0;i<3;i++) origin[i] = sv_player->v.origin[i];

        float oldspeed = sqrt(velocity[0]*velocity[0] + velocity[1]*velocity[1]);

	if ( sv_player->v.movetype == MOVETYPE_NOCLIP)
	{	// noclip
		//VectorCopy (wishvel, velocity);
	}
	else if ( onground )
	{
            while(1) { // SV_UserFriction ();
                float	*vel;
                float	speed, newspeed, control;
                vec3_t	start, stop;
                float	friction;
                trace_t	trace;

                vel = velocity;

                speed = sqrt(vel[0]*vel[0] +vel[1]*vel[1]);
                if (!speed)
                    break;

// if the leading edge is over a dropoff, increase friction
                start[0] = stop[0] = origin[0] + vel[0]/speed*16;
                start[1] = stop[1] = origin[1] + vel[1]/speed*16;
                start[2] = origin[2] + sv_player->v.mins[2];
                stop[2] = start[2] - 34;

                trace = SV_Move (start, vec3_origin, vec3_origin, stop, true, sv_player);

                if (trace.fraction == 1.0)
                    friction = sv_friction.value*sv_edgefriction.value;
                else
                    friction = sv_friction.value;

// apply friction
                control = speed < sv_stopspeed.value ? sv_stopspeed.value : speed;
                newspeed = speed - host_frametime*control*friction;

                if (newspeed < 0)
                    newspeed = 0;
                newspeed /= speed;

                vel[0] = vel[0] * newspeed;
                vel[1] = vel[1] * newspeed;
                vel[2] = vel[2] * newspeed;
                break;
            }

            while(1) { // SV_Accelerate (wishspeed, wishdir);
                int			i;
                float		addspeed, accelspeed, currentspeed;

                currentspeed = DotProduct (velocity, wishdir);
                addspeed = wishspeed - currentspeed;
                if (addspeed <= 0)
                    break;
                accelspeed = sv_accelerate.value*host_frametime*wishspeed;
                if (accelspeed > addspeed)
                    accelspeed = addspeed;

                for (i=0 ; i<3 ; i++)
                    velocity[i] += accelspeed*wishdir[i];
                break;
            }
	}
	else
	{	// not on ground, so little effect on velocity
            while(1) { // SV_AirAccelerate (wishspeed, wishvel);
                int			i;
                float		addspeed, wishspd, accelspeed, currentspeed;

                wishspd = VectorNormalize (wishvel);
                if (wishspd > 30)
                    wishspd = 30;
                currentspeed = DotProduct (velocity, wishvel);
                addspeed = wishspd - currentspeed;
                if (addspeed <= 0)
                    break;
                // accelspeed = sv_accelerate.value * host_frametime;
                accelspeed = sv_accelerate.value*wishspeed * host_frametime;
                if (accelspeed > addspeed)
                    accelspeed = addspeed;

                for (i=0 ; i<3 ; i++)
                    velocity[i] += accelspeed*wishvel[i];
                break;
            }
	}
        float newspeed = sqrt(velocity[0]*velocity[0] + velocity[1]*velocity[1]);
        return newspeed - oldspeed;
}
void SCR_DrawSpeed_angleplot_setcolor(char color) {
    switch (color) {
    case 0: glColor3f(1,1,1); break;
    case 1: glColor3f(1,0.5,0.5); break;
    case 2: glColor3f(0.5,0.5,0.5); break;
    case 3: glColor3f(0.3,0.3,0.6); break;
    case 4: glColor3f(0.6,0.3,0.3); break;
    case 5: glColor3f(0.9,0.3,0.3); break;
    case 6: glColor3f(1,0.1,0.1); break;
    default: glColor3f(1,0,0); break;
    }
}
void SCR_DrawSpeed_plot(float x, float y, float w, float h,
                        float setminvalue, float setmaxvalue,
                        float *values, int valuesstart, int valuesend, int valueslen,
                        char *colors,
                        float *horlines, char *horlinescolors, int horlineslen, char verline) {
    int valuesshown = 0;
    float minvalue = 1.0/0.0;
    float maxvalue = -1.0/0.0;
    for (int i = 0; i < valueslen; i++) {
        int index = (valuesstart + i) % valueslen;
        if (index == valuesend) break;
        valuesshown++;
        if (!isfinite(values[index])) continue;
        if (values[index] > maxvalue) maxvalue = values[index];
        if (values[index] < minvalue) minvalue = values[index];
    }
    maxvalue++; // add top-room in plot
    if (!isfinite(maxvalue)) maxvalue = 0;
    if (!isfinite(minvalue)) minvalue = 0;
    if (isfinite(setminvalue)) minvalue = setminvalue;
    if (isfinite(setmaxvalue)) maxvalue = setmaxvalue;
    // show plot max-y and min-y
    char str[50];
    sprintf(str, "%.0f", maxvalue - 1 + 0.5);
    Draw_String(x, y, str);
    sprintf(str, "%.0f", minvalue + 0.5);
    Draw_String(x, y+h-8, str);

    if (verline) {
        int chars = sprintf(str, "%.0f", values[valueslen/2]);
        Draw_String(x+w-chars*8, y+h-8, str);
    }

    glDisable (GL_TEXTURE_2D);
    // frame
    glColor3f(1,1,1);
    glBegin(GL_LINE_STRIP);
    glVertex2d(x, y);
    glVertex2d(x+w, y);
    glVertex2d(x+w, y+h);
    glVertex2d(x, y+h);
    glVertex2d(x, y);
    glEnd();

    h--; // dont overwrite frame

    for (int i = 0; i < horlineslen; i++) {
        if (horlines[i] > minvalue && horlines[i] < maxvalue) {
            SCR_DrawSpeed_angleplot_setcolor(horlinescolors[i]);
            float hy = y + h - h * (horlines[i] - minvalue) / (maxvalue - minvalue);
            glBegin(GL_LINE_STRIP);
            glVertex2d(x, hy);
            glVertex2d(x+w, hy);
            glEnd();
        }
    }
    if (verline) {
        SCR_DrawSpeed_angleplot_setcolor(2);
        glBegin (GL_LINE_STRIP);
        glVertex2d(x+w/2, y);
        glVertex2d(x+w/2, y+h);
        glEnd();
    }

    SCR_DrawSpeed_angleplot_setcolor(0);
    glBegin(GL_LINE_STRIP);
    float px = x;
    char prevcolor = -1;
    for (int i = 0; i < valueslen; i++) {
        int index = (valuesstart + i) % valueslen;
        if (index == valuesend) break;
        px += w / valuesshown;
        float py = y + h - h * (values[index] - minvalue) / (maxvalue - minvalue);
        if (colors && prevcolor != colors[index]) {
            prevcolor = colors[index];
            if (py < y) py = y;
            if (py > y+h) py = y+h;
            glVertex2d(px, py);
            glEnd();
            SCR_DrawSpeed_angleplot_setcolor(colors[index]);
            glBegin(GL_LINE_STRIP);
        }
        if (!isfinite(py)) continue;
        if (py < y) py = y;
        if (py > y+h) py = y+h;
        glVertex2d(px, py);
    }
    glEnd();
    glEnable(GL_TEXTURE_2D);
}
float getValueOrNan(cvar_t cvar) {
    if (isdigit(cvar.string[0]) || cvar.string[0] == '-')
        return cvar.value;
    else
        return 0.0/0.0;
}
void SCR_DrawSpeedHelp(void) {
    // draw speed info for each jump
    if (!sv.active && !cls.demoplayback) return;
    if ((int)scr_speed.value / 100 != 1) return;
    const char *fmt = "%c%c %3.0f %c %3.0f = %3.0f %c%3.0f %2.0f%%";
    char str[100];
    int fmtlen = 1 + sprintf(str, fmt, ' ', ' ', 0.0, '+', 0.0, 0.0, '+', 0.0, 0.0);
    int rows = 10;

    float s = scr_speed_scale.value;
    int w = fmtlen*8;
    int h = rows*8;

    extern usercmd_t cmd;

    static qboolean clprevonground;
    static qboolean clprevprevonground;
    static float prevspeed = 0;
    static float airspeedsum = 0;
    static float airspeedsummax = 0;
    static char *hist = NULL;
    static double *histtimeout = NULL;
    static int histrow = 0;
    static double prevcltime = -1;

    if (!hist) {
        hist = (char*) malloc(sizeof(char)*fmtlen*rows);
        for (int i = 0; i < fmtlen*rows; i++) hist[i] = 0;
        histtimeout = (double*) malloc(sizeof(double)*rows);
    }

    if (cl.time < prevcltime) {
        for (int i = 0; i < rows; i++) histtimeout[i] = 0.0;
        prevcltime = cl.time;
    }
    prevcltime = cl.time;


    {
        int angles = scr_speed_angles.value;
        float bestspeed = 0.0/0.0;
        if (sv.active) { // does not work in demo
            float bestangle = 0;
            bestspeed = Get_Wishdir_Speed_Delta(bestangle);
            for (int i = 1; i < angles; i++) {
                for (int neg = -1; neg <= 1; neg += 2) {
                    float curangle = sv_player->v.angles[1] + i*neg;
                    float curspeed = Get_Wishdir_Speed_Delta(curangle);
                    if (curspeed > bestspeed) {
                        bestspeed = curspeed;
                        bestangle = curangle;
                    }
                }
            }
            for (int i = 1; i < 100; i++) {
                for (int neg = -1; neg <= 1; neg += 2) {
                    float curangle = bestangle + neg/100;
                    float curspeed = Get_Wishdir_Speed_Delta(curangle);
                    if (curspeed > bestspeed) {
                        bestspeed = curspeed;
                        bestangle = curangle;
                    }
                }
            }
            if (!cl.onground) {
                airspeedsum += Get_Wishdir_Speed_Delta(sv_player->v.angles[1]);
                airspeedsummax += bestspeed;
            }
        }
        float speed = sqrt(cl.velocity[0]*cl.velocity[0]+cl.velocity[1]*cl.velocity[1]);
        if ((cl.onground && !clprevonground) || (!cl.onground && clprevonground && clprevprevonground)) {
            float addspeed = sv.active ? Get_Wishdir_Speed_Delta(sv_player->v.angles[1]) : 0.0;
            float addspeedpct = 100.0 * (airspeedsum + addspeed) / (airspeedsummax + bestspeed);
            if (!sv.active) { // demo
                airspeedsum = 0.0;
                addspeedpct = 0.0;
            }
            sprintf(str, fmt,
                    cmd.sidemove < 0 ? 127 : cmd.sidemove > 0 ? 141 : ' ',
                    cmd.forwardmove > 0 ? '^' : cmd.forwardmove < 0 ? 'v' : ' ',
                    airspeedsum,
                    addspeed >= 0 ? '+' : '-', fabs(addspeed),
                    fmin(999.0, speed),
                    speed-prevspeed >= 0 ? '+' : '-', fmin(999.0, fabs(speed-prevspeed)),
                   fmin(99, fmax(0, addspeedpct)));
            for (int i = 0; i < fmtlen; i++) {
                hist[histrow*fmtlen + i] = str[i];
            }
            histtimeout[histrow] = realtime + scr_speed_history.value/10;
            histrow = (histrow+1) % rows;
            prevspeed = speed;
            airspeedsum = 0;
            airspeedsummax = 0;
        }
        if (speed < 1) prevspeed = 0;

        clprevprevonground = clprevonground;
        clprevonground = cl.onground;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -99999, 99999);
    glViewport(glx + glwidth/2 - w*s/2, gly + glheight/2 - h*s, w*s, h*s);

    for (int i = 0; i < rows; i++) {
        int row = histrow - 1 - i;
        if (row < 0) row += rows;
        if (realtime <= histtimeout[row])
            Draw_String(0, i*8, hist + row*fmtlen);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();
}
// draw speed information
void SCR_DrawSpeed (void)
{
    SCR_DrawSpeedHelp();
    extern qboolean onground;
    extern qboolean prevonground;
    extern int gametick;

    static double prevcltime = -1;
    static int historylen = 0;
    static int historyminspeed = 0;
    static float *history = NULL;
    static char *history_onground = NULL;
    static float *history_angleoffset = NULL;
    static int historystart = -1;
    static int historyend = -1;
    static char onground_seen = 0;
    static double oldtime = 0;
    static float prevspeed = 0;
    static float maxspeed = 0;
    static int prevgametick = 0;

    if ((int)scr_speed.value % 100 <= 0) return;

    if (scr_speed_history.value != historylen
        || scr_speed_minspeed.value != historyminspeed
        || cl.time < prevcltime) {
        free(history);
        free(history_onground);
        free(history_angleoffset);
        historylen = scr_speed_history.value;
        historyminspeed = scr_speed_minspeed.value;
        historystart = 0;
        historyend = 0;
        history = (float*) malloc(sizeof(float)*historylen);
        history_onground = (char*) malloc(sizeof(char)*historylen);
        history_angleoffset = (float*) malloc(sizeof(float)*historylen);
    }
    prevcltime = cl.time;

    // manual GL_SetCanvas to avoid changing multiple files
    // attach to middle right of screen
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
    float s = scr_speed_scale.value;
    glOrtho (0, 100, 200, 0, -99999, 99999);
    glViewport (glx + (glwidth - 100*s), gly + glheight/2 - 200*s/2, 100*s, 200*s);

    if (onground) onground_seen = 1;

    // current speed
    float speed = sqrt(cl.velocity[0]*cl.velocity[0] +cl.velocity[1]*cl.velocity[1]);
    char str[50];
    sprintf (str, "speed=%.0f", prevspeed+0.5f);
    Draw_String (0, 0, str);

    // average speed
    float avgspeed = 0;
    int avgspeedcount = 0;
    for (int i = 0; i < historylen; i++) {
        int index = (historystart + i) % historylen;
        if (index == historyend) break;
        avgspeed += history[index];
        avgspeedcount++;
    }
    avgspeed /= avgspeedcount;
    sprintf (str, "avgspeed=%.0f", avgspeed+0.5f);
    Draw_String (0, 8, str);

    // max speed since last complete halt
    if (speed == 0) maxspeed = 0;
    if (speed > maxspeed) maxspeed = speed;
    sprintf (str, "maxspeed=%.0f", maxspeed+0.5f);
    Draw_String (0, 16, str);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity ();

    float cury = 24 + 1;
    float plotheights = (200 - cury - 5) / 5;
    if ((int)scr_speed.value % 100 >= 2) { // draw histogram of speed over the last bit of time
        // color of line shows whether player was on ground
        float x = 1;
        float y = cury;
        float w = 99;
        float h = plotheights;
        cury+=h+1;

        float horlines[] = { 400, 500, 600, 700 };
        char horlinescolors[] = { 3, 4, 5, 6 };
        SCR_DrawSpeed_plot(x, y, w, h,
                           getValueOrNan(scr_speed_minspeed), getValueOrNan(scr_speed_maxspeed), 
                           history, historystart, historyend, historylen,
                           history_onground,
                           horlines, horlinescolors, sizeof(horlines)/sizeof(horlines[0]), false);
    }
    float bestangle = -1.0/0.0;
    if ((int)scr_speed.value % 100 >= 3 && sv.active) {
        // show view angles and how much speed they would give
        // x-axis is speed if angle was more left/right
        // keep highest part of plot in the center to maximize speed

        int angles = scr_speed_angles.value;
        int len = angles * 2 + 1;

        static float *angleoffsetspeedsprev = NULL;
        static int angleoffsetspeedsprevlen = 0;
        if (angleoffsetspeedsprevlen != len) {
            free(angleoffsetspeedsprev);
            angleoffsetspeedsprev = (float*) malloc(sizeof(float)*len);
            angleoffsetspeedsprevlen = len;
        }

        float *angleoffsetspeeds = (float*) malloc(sizeof(float)*len);
        float playerangle = sv_player->v.angles[1];
        float bestspeed = -1.0/0.0;
        for (int angle = -angles; angle <= angles; angle++) {
            float addspeed = Get_Wishdir_Speed_Delta(playerangle + angle);
            if (addspeed < -20) addspeed = -20;
            if (addspeed > bestspeed
                || (addspeed == bestspeed && abs(angle) < abs(bestangle))) {
                bestspeed = addspeed; bestangle = angle;
            }
            angleoffsetspeeds[angles+angle] = addspeed;
        }

        float horlines[] = { 0 };
        char horlinescolors[] = { 2 };
        { // live update plot
            float x = 1;
            float y = cury;
            float w = 99;
            float h = plotheights;
            cury += h+1;
            SCR_DrawSpeed_plot(x, y, w, h,
                               getValueOrNan(scr_speed_scale_minspeed), getValueOrNan(scr_speed_scale_maxspeed),
                               angleoffsetspeeds, 0, -1, len,
                               NULL,
                               horlines, horlinescolors, sizeof(horlines)/sizeof(horlines[0]), true);
        }
        { // plot for latest jumping tick
            static float *angleoffsetspeedsjumping = NULL;
            static int angleoffsetspeedsjumpinglen = 0;
            if (angleoffsetspeedsjumpinglen != len) {
                free(angleoffsetspeedsjumping);
                angleoffsetspeedsjumping = (float*) malloc(sizeof(float)*len);
                angleoffsetspeedsjumpinglen = len;
                for (int i = 0; i < len; i++) angleoffsetspeedsjumping[i] = 0;
            }
            if (gametick != prevgametick && !onground && prevonground) {
                for (int i = 0; i < len; i++) {
                    angleoffsetspeedsjumping[i] = angleoffsetspeedsprev[i];
                }
            }

            float x = 1;
            float y = cury;
            float w = 99;
            float h = plotheights;
            cury += h+1;
            SCR_DrawSpeed_plot(x, y, w, h,
                               getValueOrNan(scr_speed_scale_minspeed), getValueOrNan(scr_speed_scale_maxspeed),
                               angleoffsetspeedsjumping, 0, -1, len,
                               NULL,
                               horlines, horlinescolors, sizeof(horlines)/sizeof(horlines[0]), true);
        }
        { // plot for latest landing tick
            static float *angleoffsetspeedslanding = NULL;
            static int angleoffsetspeedslandinglen = 0;
            if (angleoffsetspeedslandinglen != len) {
                free(angleoffsetspeedslanding);
                angleoffsetspeedslanding = (float*) malloc(sizeof(float)*len);
                angleoffsetspeedslandinglen = len;
                for (int i = 0; i < len; i++) angleoffsetspeedslanding[i] = 0;
            }
            if (gametick != prevgametick && onground && !prevonground) {
                for (int i = 0; i < len; i++) {
                    angleoffsetspeedslanding[i] = angleoffsetspeeds[i];
                }
            }

            float x = 1;
            float y = cury;
            float w = 99;
            float h = plotheights;
            cury += h+1;
            SCR_DrawSpeed_plot(x, y, w, h,
                               getValueOrNan(scr_speed_scale_minspeed), getValueOrNan(scr_speed_scale_maxspeed),
                               angleoffsetspeedslanding, 0, -1, len,
                               NULL,
                               horlines, horlinescolors, sizeof(horlines)/sizeof(horlines[0]), true);
        }

        for (int i = 0; i < len; i++) {
            angleoffsetspeedsprev[i] = angleoffsetspeeds[i];
        }
        free(angleoffsetspeeds);

        if ((int)scr_speed.value % 100 >= 4) {
            // histogram over how close the view angle has been to one giving most speed
            // perfect speed is when this plot is at 0.0
            float x = 1;
            float y = cury;
            float w = 99;
            float h = plotheights;
            cury += h+1;

            float horlines[] = { 0 };
            char horlinescolors[] = { 2 };
            SCR_DrawSpeed_plot(x, y, w, h,
                               0.0/0.0, 0.0/0.0,
                               history_angleoffset, historystart, historyend, historylen,
                               NULL,
                               horlines, horlinescolors, sizeof(horlines)/sizeof(horlines[0]), false);
        }
    }

    if (cls.state == ca_connected
        && (oldtime < 0 || realtime - oldtime > 0.1)) { // 10 readings per second
        // history of speed
        history[historyend] = speed;
        if (history[historyend] < scr_speed_minspeed.value)
            history[historyend] = scr_speed_minspeed.value;
        history_onground[historyend] = onground_seen;
        history_angleoffset[historyend] = bestangle;
        onground_seen = 0;
        historyend = (historyend + 1) % historylen;
        if (historyend == historystart) historystart = (historystart + 1) % historylen;
        prevspeed = speed;
        oldtime = realtime;
    }
    prevgametick = gametick;
}

//=============================================================================


/*
==================
SCR_SetUpToDrawConsole
==================
*/
void SCR_SetUpToDrawConsole (void)
{
	//johnfitz -- let's hack away the problem of slow console when host_timescale is <0
	extern cvar_t host_timescale;
	float timescale;
	//johnfitz

	Con_CheckResize ();

	if (scr_drawloading)
		return;		// never a console with loading plaque

// decide on the height of the console
	con_forcedup = !cl.worldmodel || cls.signon != SIGNONS;

	if (con_forcedup)
	{
		scr_conlines = glheight; //full screen //johnfitz -- glheight instead of vid.height
		scr_con_current = scr_conlines;
	}
	else if (key_dest == key_console)
		scr_conlines = glheight/2; //half screen //johnfitz -- glheight instead of vid.height
	else
		scr_conlines = 0; //none visible

	timescale = (host_timescale.value > 0) ? host_timescale.value : 1; //johnfitz -- timescale

	if (scr_conlines < scr_con_current)
	{
		// ericw -- (glheight/600.0) factor makes conspeed resolution independent, using 800x600 as a baseline
		scr_con_current -= scr_conspeed.value*(glheight/600.0)*host_frametime/timescale; //johnfitz -- timescale
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;
	}
	else if (scr_conlines > scr_con_current)
	{
		// ericw -- (glheight/600.0)
		scr_con_current += scr_conspeed.value*(glheight/600.0)*host_frametime/timescale; //johnfitz -- timescale
		if (scr_conlines < scr_con_current)
			scr_con_current = scr_conlines;
	}

	if (clearconsole++ < vid.numpages)
		Sbar_Changed ();

	if (!con_forcedup && scr_con_current)
		scr_tileclear_updates = 0; //johnfitz
}

/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole (void)
{
	if (scr_con_current)
	{
		Con_DrawConsole (scr_con_current, true);
		clearconsole = 0;
	}
	else
	{
		if (key_dest == key_game || key_dest == key_message)
			Con_DrawNotify ();	// only draw notify in game
	}
}


/*
==============================================================================

SCREEN SHOTS

==============================================================================
*/

static void SCR_ScreenShot_Usage (void)
{
	Con_Printf ("usage: screenshot <format> <quality>\n");
	Con_Printf ("   format must be \"png\" or \"tga\" or \"jpg\"\n");
	Con_Printf ("   quality must be 1-100\n");
	return;
}

/*
==================
SCR_ScreenShot_f -- johnfitz -- rewritten to use Image_WriteTGA
==================
*/
void SCR_ScreenShot_f (void)
{
	byte	*buffer;
	char	ext[4];
	char	imagename[16];  //johnfitz -- was [80]
	char	checkname[MAX_OSPATH];
	int	i, quality;
	qboolean	ok;

	Q_strncpy (ext, "png", sizeof(ext));

	if (Cmd_Argc () >= 2)
	{
		const char	*requested_ext = Cmd_Argv (1);

		if (!q_strcasecmp ("png", requested_ext)
		    || !q_strcasecmp ("tga", requested_ext)
		    || !q_strcasecmp ("jpg", requested_ext))
			Q_strncpy (ext, requested_ext, sizeof(ext));
		else
		{
			SCR_ScreenShot_Usage ();
			return;
		}
	}

// read quality as the 3rd param (only used for JPG)
	quality = 90;
	if (Cmd_Argc () >= 3)
		quality = Q_atoi (Cmd_Argv(2));
	if (quality < 1 || quality > 100)
	{
		SCR_ScreenShot_Usage ();
		return;
	}
	
// find a file name to save it to
	for (i=0; i<10000; i++)
	{
		q_snprintf (imagename, sizeof(imagename), "spasm%04i.%s", i, ext);	// "fitz%04i.tga"
		q_snprintf (checkname, sizeof(checkname), "%s/%s", com_gamedir, imagename);
		if (Sys_FileType(checkname) == FS_ENT_NONE)
			break;	// file doesn't exist
	}
	if (i == 10000)
	{
		Con_Printf ("SCR_ScreenShot_f: Couldn't find an unused filename\n");
		return;
	}

//get data
	if (!(buffer = (byte *) malloc(glwidth*glheight*3)))
	{
		Con_Printf ("SCR_ScreenShot_f: Couldn't allocate memory\n");
		return;
	}

	glPixelStorei (GL_PACK_ALIGNMENT, 1);/* for widths that aren't a multiple of 4 */
	glReadPixels (glx, gly, glwidth, glheight, GL_RGB, GL_UNSIGNED_BYTE, buffer);

// now write the file
	if (!q_strncasecmp (ext, "png", sizeof(ext)))
		ok = Image_WritePNG (imagename, buffer, glwidth, glheight, 24, false);
	else if (!q_strncasecmp (ext, "tga", sizeof(ext)))
		ok = Image_WriteTGA (imagename, buffer, glwidth, glheight, 24, false);
	else if (!q_strncasecmp (ext, "jpg", sizeof(ext)))
		ok = Image_WriteJPG (imagename, buffer, glwidth, glheight, 24, quality, false);
	else
		ok = false;

	if (ok)
		Con_Printf ("Wrote %s\n", imagename);
	else
		Con_Printf ("SCR_ScreenShot_f: Couldn't create %s\n", imagename);

	free (buffer);
}


//=============================================================================


/*
===============
SCR_BeginLoadingPlaque

================
*/
void SCR_BeginLoadingPlaque (void)
{
	S_StopAllSounds (true);

	if (cls.state != ca_connected)
		return;
	if (cls.signon != SIGNONS)
		return;

// redraw with no console and the loading plaque
	Con_ClearNotify ();
	scr_centertime_off = 0;
	scr_con_current = 0;

	scr_drawloading = true;
	Sbar_Changed ();
	SCR_UpdateScreen ();
	scr_drawloading = false;

	scr_disabled_for_loading = true;
	scr_disabled_time = realtime;
}

/*
===============
SCR_EndLoadingPlaque

================
*/
void SCR_EndLoadingPlaque (void)
{
	scr_disabled_for_loading = false;
	Con_ClearNotify ();
}

//=============================================================================

const char	*scr_notifystring;
qboolean	scr_drawdialog;

void SCR_DrawNotifyString (void)
{
	const char	*start;
	int		l;
	int		j;
	int		x, y;

	GL_SetCanvas (CANVAS_MENU); //johnfitz

	start = scr_notifystring;

	y = 200 * 0.35; //johnfitz -- stretched overlays

	do
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (320 - l*8)/2; //johnfitz -- stretched overlays
		for (j=0 ; j<l ; j++, x+=8)
			Draw_Character (x, y, start[j]);

		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

/*
==================
SCR_ModalMessage

Displays a text string in the center of the screen and waits for a Y or N
keypress.
==================
*/
int SCR_ModalMessage (const char *text, float timeout) //johnfitz -- timeout
{
	double time1, time2; //johnfitz -- timeout
	int lastkey, lastchar;

	if (cls.state == ca_dedicated)
		return true;

	scr_notifystring = text;

// draw a fresh screen
	scr_drawdialog = true;
	SCR_UpdateScreen ();
	scr_drawdialog = false;

	S_ClearBuffer ();		// so dma doesn't loop current sound

	time1 = Sys_DoubleTime () + timeout; //johnfitz -- timeout
	time2 = 0.0f; //johnfitz -- timeout

	Key_BeginInputGrab ();
	do
	{
		Sys_SendKeyEvents ();
		Key_GetGrabbedInput (&lastkey, &lastchar);
		Sys_Sleep (16);
		if (timeout) time2 = Sys_DoubleTime (); //johnfitz -- zero timeout means wait forever.
	} while (lastchar != 'y' && lastchar != 'Y' &&
		 lastchar != 'n' && lastchar != 'N' &&
		 lastkey != K_ESCAPE &&
		 lastkey != K_ABUTTON &&
		 lastkey != K_BBUTTON &&
		 time2 <= time1);
	Key_EndInputGrab ();

//	SCR_UpdateScreen (); //johnfitz -- commented out

	//johnfitz -- timeout
	if (time2 > time1)
		return false;
	//johnfitz

	return (lastchar == 'y' || lastchar == 'Y' || lastkey == K_ABUTTON);
}


//=============================================================================

//johnfitz -- deleted SCR_BringDownConsole


/*
==================
SCR_TileClear
johnfitz -- modified to use glwidth/glheight instead of vid.width/vid.height
	    also fixed the dimentions of right and top panels
	    also added scr_tileclear_updates
==================
*/
void SCR_TileClear (void)
{
	//ericw -- added check for glsl gamma. TODO: remove this ugly optimization?
	if (scr_tileclear_updates >= vid.numpages && !gl_clear.value && !(gl_glsl_gamma_able && vid_gamma.value != 1))
		return;
	scr_tileclear_updates++;

	if (r_refdef.vrect.x > 0)
	{
		// left
		Draw_TileClear (0,
						0,
						r_refdef.vrect.x,
						glheight - sb_lines);
		// right
		Draw_TileClear (r_refdef.vrect.x + r_refdef.vrect.width,
						0,
						glwidth - r_refdef.vrect.x - r_refdef.vrect.width,
						glheight - sb_lines);
	}

	if (r_refdef.vrect.y > 0)
	{
		// top
		Draw_TileClear (r_refdef.vrect.x,
						0,
						r_refdef.vrect.width,
						r_refdef.vrect.y);
		// bottom
		Draw_TileClear (r_refdef.vrect.x,
						r_refdef.vrect.y + r_refdef.vrect.height,
						r_refdef.vrect.width,
						glheight - r_refdef.vrect.y - r_refdef.vrect.height - sb_lines);
	}
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.

WARNING: be very careful calling this from elsewhere, because the refresh
needs almost the entire 256k of stack space!
==================
*/
void SCR_UpdateScreen (void)
{
	vid.numpages = (gl_triplebuffer.value) ? 3 : 2;

	if (scr_disabled_for_loading)
	{
		if (realtime - scr_disabled_time > 60)
		{
			scr_disabled_for_loading = false;
			Con_Printf ("load failed.\n");
		}
		else
			return;
	}

	if (!scr_initialized || !con_initialized)
		return;				// not initialized yet


	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);

	//
	// determine size of refresh window
	//
	if (vid.recalc_refdef)
		SCR_CalcRefdef ();

//
// do 3D refresh drawing, and then update the screen
//
	SCR_SetUpToDrawConsole ();

	V_RenderView ();

	GL_Set2D ();

	//FIXME: only call this when needed
	SCR_TileClear ();

	if (scr_drawdialog) //new game confirm
	{
		if (con_forcedup)
			Draw_ConsoleBackground ();
		else
			Sbar_Draw ();
		Draw_FadeScreen ();
		SCR_DrawNotifyString ();
	}
	else if (scr_drawloading) //loading
	{
		SCR_DrawLoading ();
		Sbar_Draw ();
	}
	else if (cl.intermission == 1 && key_dest == key_game) //end of level
	{
		Sbar_IntermissionOverlay ();
	}
	else if (cl.intermission == 2 && key_dest == key_game) //end of episode
	{
		Sbar_FinaleOverlay ();
		SCR_CheckDrawCenterString ();
	}
	else
	{
		SCR_DrawCrosshair (); //johnfitz
		SCR_DrawNet ();
		SCR_DrawTurtle ();
		SCR_DrawPause ();
		SCR_CheckDrawCenterString ();
		Sbar_Draw ();
		SCR_DrawDevStats (); //johnfitz
		SCR_DrawFPS (); //johnfitz
		SCR_DrawClock (); //johnfitz
                SCR_DrawExtendedHud ();
                SCR_DrawRadar ();
		SCR_DrawConsole ();
		M_Draw ();
	}
        SCR_DrawSpeed ();

	V_UpdateBlend (); //johnfitz -- V_UpdatePalette cleaned up and renamed

	GLSLGamma_GammaCorrect ();

	GL_EndRendering ();
}
