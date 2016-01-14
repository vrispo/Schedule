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

#define WINDOW_H	768
#define WINDOW_W	1024

//--------------------------------------------------------------------------
//TYPE DEFINITIONS
//--------------------------------------------------------------------------



//--------------------------------------------------------------------------
//FUNCTION DECLARATIONS
//--------------------------------------------------------------------------

void setup (void);
void analysis_key(void);
void get_keycodes(char * scan, char * ascii);

//--------------------------------------------------------------------------
//GLOBAL VARIABLES
//--------------------------------------------------------------------------

bool	run=TRUE;

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
//SETUP
//--------------------------------------------------------------------------

void setup(void)
{
	allegro_init();
	install_keyboard();

	set_color_depth(8);

	set_gfx_mode(GFX_AUTODETECT_WINDOWED, WINDOW_W, WINDOW_H, 0, 0);

	clear_to_color(screen, 7);
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
