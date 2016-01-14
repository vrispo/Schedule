#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <allegro.h>
#include <stdbool.h>
#include "taskRT.h"
#include "timeplus.h"

//--------------------------------------------------------------------------
// GLOBAL CONSTANTS
//--------------------------------------------------------------------------

#define WINDOW_H		820
#define WINDOW_W		1100

#define GRAFIC_H		150
#define GRAFIC_W		1000
#define ORIGIN_GRAFIC_X	15
#define SPACE			20
#define INSTR_H			110
#define INSTR_W			1000
#define INSTR_L			100
#define INSTR_X			5
#define INSTR_Y			10

//--------------------------------------------------------------------------
//TYPE DEFINITIONS
//--------------------------------------------------------------------------



//--------------------------------------------------------------------------
//FUNCTION DECLARATIONS
//--------------------------------------------------------------------------

void setup (void);
void setup_grafic(int x, int y, char s[]);
void analysis_key(void);
void get_keycodes(char * scan, char * ascii);

//--------------------------------------------------------------------------
//GLOBAL VARIABLES
//--------------------------------------------------------------------------

bool	run=TRUE;

int		ORIGIN_PIP_Y;
int		ORIGIN_PIPW_Y;
int		ORIGIN_PCP_Y;
int		ORIGIN_PCPW_Y;

//--------------------------------------------------------------------------
//FUNCTION DEFINITIONS
//--------------------------------------------------------------------------

int main(int argc, char * argv[])
{
	setup();

	while(run)
	{
		analysis_key();
	}

	allegro_exit();
	return 0;
}

//--------------------------------------------------------------------------
//SETUP GRAFIC
//--------------------------------------------------------------------------

void setup_grafic(int x, int y, char s[])
{
	//asse y
	line(screen, x, (y-GRAFIC_H), x, y, 0);
	line(screen, x, (y-GRAFIC_H), (x-5), (y-GRAFIC_H+5), 0);
	line(screen, x, (y-GRAFIC_H), (x+5), (y-GRAFIC_H+5), 0);
	textout_ex(screen, font, s, (x-5), (y-GRAFIC_H-10), 0, -1);
	//asse x
	line(screen, x, y, (x+GRAFIC_W), y, 0);
	line(screen, (x+GRAFIC_W-5), (y-5), (x+GRAFIC_W), y, 0);
	line(screen, (x+GRAFIC_W-5), (y+5), (x+GRAFIC_W), y, 0);
	textout_ex(screen, font, "t", (x+GRAFIC_W+5), (y+5), 0, -1);
}

//--------------------------------------------------------------------------
//SETUP
//--------------------------------------------------------------------------

void setup(void)
{
	allegro_init();
	install_keyboard();

	set_color_depth(8);

	set_gfx_mode(GFX_AUTODETECT_WINDOWED, WINDOW_W, WINDOW_H, 0, 0);

	clear_to_color(screen, 7);

	//istruzioni
	textout_ex(screen, font, "INSTRUCTIONS", (INSTR_X+7), (INSTR_Y-5), 0, -1);
	line(screen, INSTR_X, INSTR_Y, (INSTR_X+5), INSTR_Y, 0);
	line(screen, (INSTR_X+5+INSTR_L), INSTR_Y, (INSTR_X+INSTR_W), INSTR_Y, 0);
	line(screen, INSTR_X, INSTR_Y, INSTR_X, (INSTR_Y+INSTR_H), 0);
	line(screen, INSTR_X, (INSTR_Y+INSTR_H), (INSTR_X+INSTR_W), (INSTR_Y+INSTR_H), 0);
	line(screen, (INSTR_X+INSTR_W), (INSTR_Y+INSTR_H), (INSTR_X+INSTR_W), 10, 0);

	textout_ex(screen, font, "PRESS KEY ESC TO EXIT", (INSTR_X+10), (INSTR_Y+10), 0, -1);

	//grafici
	ORIGIN_PIP_Y=INSTR_Y+INSTR_H+SPACE+GRAFIC_H;
	ORIGIN_PIPW_Y=ORIGIN_PIP_Y+SPACE+GRAFIC_H;
	ORIGIN_PCP_Y=ORIGIN_PIPW_Y+SPACE+GRAFIC_H;
	ORIGIN_PCPW_Y=ORIGIN_PCP_Y+SPACE+GRAFIC_H;
	setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PIP_Y, "PIP");
	setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PIPW_Y, "PIP workload");
	setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PCP_Y, "PCP");
	setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PCPW_Y, "PCP workload");
}

//--------------------------------------------------------------------------
//GET THE SCAN CODE AND THE ASCII CODE FROM A READ KEY
//--------------------------------------------------------------------------

void get_keycodes(char * scan, char * ascii)
{
int k;

	k=readkey();
	*ascii=k;
	*scan=k>>8;
}

//--------------------------------------------------------------------------
//ANALYSIS KEYBOARD
//--------------------------------------------------------------------------

void analysis_key(void)
{
char	scan, ascii;

	get_keycodes(&scan, &ascii);

	switch(scan)
	{
		case KEY_ESC:
			run=FALSE;
			break;
		default:
			break;
	}
}
