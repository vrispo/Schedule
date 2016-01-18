#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <allegro.h>
#include <stdbool.h>
#include <pthread.h>
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

#define N_TASK			4

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
void draw_grafic_task_base(void);
void * grafic_task(void * arg);
void * t_task(void * arg);

//--------------------------------------------------------------------------
//GLOBAL VARIABLES
//--------------------------------------------------------------------------

struct timespec	zero_time;
int				time_scale=250;
int				x=0;

bool			run=TRUE;
char			run_task;

bool			pip=TRUE;

int				ORIGIN_PIP_Y;
int				ORIGIN_PIPW_Y;
int				ORIGIN_PCP_Y;
int				ORIGIN_PCPW_Y;

int				H_TASK;

pthread_t		grafic_tid;
struct task_par	grafic_tp;
pthread_attr_t	grafic_attr;

pthread_t		t1_tid;
struct task_par	t1_tp;
pthread_attr_t	t1_attr;

pthread_t		t2_tid;
struct task_par	t2_tp;
pthread_attr_t	t2_attr;

pthread_t		t3_tid;
struct task_par	t3_tp;
pthread_attr_t	t3_attr;

pthread_t		t4_tid;
struct task_par	t4_tp;
pthread_attr_t	t4_attr;

//--------------------------------------------------------------------------
//FUNCTION DEFINITIONS
//--------------------------------------------------------------------------

int main(int argc, char * argv[])
{
	setup();

	while(run)
	{
		run_task='m';
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
char s[30];

	allegro_init();
	install_keyboard();

	set_color_depth(8);

	set_gfx_mode(GFX_AUTODETECT_WINDOWED, WINDOW_W, WINDOW_H, 0, 0);

	clear_to_color(screen, 7);

	clock_gettime(CLOCK_MONOTONIC, &zero_time);
	run_task='m';

	//istruzioni
	textout_ex(screen, font, "INSTRUCTIONS", (INSTR_X+7), (INSTR_Y-5), 0, -1);
	line(screen, INSTR_X, INSTR_Y, (INSTR_X+5), INSTR_Y, 0);
	line(screen, (INSTR_X+5+INSTR_L), INSTR_Y, (INSTR_X+INSTR_W), INSTR_Y, 0);
	line(screen, INSTR_X, INSTR_Y, INSTR_X, (INSTR_Y+INSTR_H), 0);
	line(screen, INSTR_X, (INSTR_Y+INSTR_H), (INSTR_X+INSTR_W), (INSTR_Y+INSTR_H), 0);
	line(screen, (INSTR_X+INSTR_W), (INSTR_Y+INSTR_H), (INSTR_X+INSTR_W), 10, 0);

	textout_ex(screen, font, "PRESS KEY ESC TO EXIT", (INSTR_X+10), (INSTR_Y+10), 0, -1);

	sprintf(s, "UNITA' MISURA -> %i ms = ", time_scale);
	textout_ex(screen, font, s, (INSTR_X+10), (INSTR_Y+20), 0, -1);
	line(screen, (INSTR_X+210), (INSTR_Y+22.5), (INSTR_X+215), (INSTR_Y+22.5), 0);

	//grafici
	ORIGIN_PIP_Y=INSTR_Y+INSTR_H+SPACE+GRAFIC_H;
	ORIGIN_PIPW_Y=ORIGIN_PIP_Y+SPACE+GRAFIC_H;
	ORIGIN_PCP_Y=ORIGIN_PIPW_Y+SPACE+GRAFIC_H;
	ORIGIN_PCPW_Y=ORIGIN_PCP_Y+SPACE+GRAFIC_H;
	setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PIP_Y, "PIP");
	setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PIPW_Y, "PIP workload");
	setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PCP_Y, "PCP");
	setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PCPW_Y, "PCP workload");

	H_TASK=(GRAFIC_H-(N_TASK*10))/(N_TASK+1);
	draw_grafic_task_base();

	//create grafic task
	grafic_tp.arg=0;
	grafic_tp.period=time_scale;
	grafic_tp.deadline=10;
	grafic_tp.priority=90;
	grafic_tp.dmiss=0;

	pthread_attr_init(&grafic_attr);
	pthread_create(&grafic_tid, &grafic_attr, grafic_task, &grafic_tp);

	//create task 1
	t1_tp.arg=0;
	t1_tp.period=1000;
	t1_tp.deadline=1000;
	t1_tp.priority=70;
	t1_tp.dmiss=0;

	pthread_attr_init(&t1_attr);
	pthread_create(&t1_tid, &t1_attr, t_task, &t1_tp);

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
bool	keyp=FALSE;

	keyp=keypressed();
	if(keyp){
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
}

//--------------------------------------------------------------------------
//DRAW GRAFIC TASK BASE (relativo asse x per ogni task)
//--------------------------------------------------------------------------

void draw_grafic_task_base()
{
int	i=0;

	for(i=0; i<=N_TASK; i++){
		line(screen, ORIGIN_GRAFIC_X,(ORIGIN_PIP_Y-(i*(H_TASK+10))),(ORIGIN_GRAFIC_X+GRAFIC_W),(ORIGIN_PIP_Y-(i*(H_TASK+10))),0);
		line(screen, ORIGIN_GRAFIC_X,(ORIGIN_PIPW_Y-(i*(H_TASK+10))),(ORIGIN_GRAFIC_X+GRAFIC_W),(ORIGIN_PIPW_Y-(i*(H_TASK+10))),0);
		line(screen, ORIGIN_GRAFIC_X,(ORIGIN_PCP_Y-(i*(H_TASK+10))),(ORIGIN_GRAFIC_X+GRAFIC_W),(ORIGIN_PCP_Y-(i*(H_TASK+10))),0);
		line(screen, ORIGIN_GRAFIC_X,(ORIGIN_PCPW_Y-(i*(H_TASK+10))),(ORIGIN_GRAFIC_X+GRAFIC_W),(ORIGIN_PCPW_Y-(i*(H_TASK+10))),0);
	}
}

//--------------------------------------------------------------------------
//GRAFIC TASK
//--------------------------------------------------------------------------

void * grafic_task(void * arg)
{
struct timespec	at,t;
double			ms;

	x=0;
	while(1){
		time_add_ms(&t, time_scale+1);
		if(pip){
			switch(run_task)
			{
				case 'm':
					clock_gettime(CLOCK_MONOTONIC, &at);
					time_sub_ms(at, zero_time, &ms);
					//x=(ms/time_scale);
					rectfill(screen, (ORIGIN_GRAFIC_X+(x*5)), ORIGIN_PIP_Y, (ORIGIN_GRAFIC_X+(x*5)+5), (ORIGIN_PIP_Y-H_TASK), 10);
					clock_nanosleep(CLOCK_MONOTONIC, 0, &t, NULL);
					break;
				case '1':
					clock_gettime(CLOCK_MONOTONIC, &at);
					time_sub_ms(at, zero_time, &ms);
					//x=(ms/time_scale);
					rectfill(screen, (ORIGIN_GRAFIC_X+(x*5)), (ORIGIN_PIP_Y-(H_TASK+10)), (ORIGIN_GRAFIC_X+(x*5)+5), (ORIGIN_PIP_Y-(H_TASK+10)-H_TASK), 10);
					clock_nanosleep(CLOCK_MONOTONIC, 0, &t, NULL);
					break;
				default:
					break;
			}
		}
		//pcp case
		else{

		}
		x++;
	}
}

//--------------------------------------------------------------------------
//GENERIC TASK
//--------------------------------------------------------------------------

void * t_task(void * arg)
{
	while(1){
		run_task='1';
	}
}
