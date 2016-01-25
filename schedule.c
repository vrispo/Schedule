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
void draw_grafic_task_base(char g);
void * grafic_task(void * arg);
void * t_task(void * arg);

//--------------------------------------------------------------------------
//GLOBAL VARIABLES
//--------------------------------------------------------------------------

struct timespec	zero_time;
int				time_scale=100;
int				x=0;

bool			run=TRUE;
int				run_task;
int				arg1;
int				arg2;
int				arg3;
int				arg4;

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
		run_task=0;
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
	run_task=0;

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
	//draw_grafic_task_base('a');

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
	t1_tp.period=1500;
	t1_tp.deadline=1000;
	t1_tp.priority=70;
	t1_tp.dmiss=0;
	arg1=1;
	pthread_attr_init(&t1_attr);
	pthread_create(&t1_tid, &t1_attr, t_task, (void*)&arg1);

	//create task 2
	t2_tp.arg=0;
	t2_tp.period=1500;
	t2_tp.deadline=500;
	t2_tp.priority=60;
	t2_tp.dmiss=0;
	arg2=2;
	pthread_attr_init(&t2_attr);
	pthread_create(&t2_tid, &t2_attr, t_task, (void*)&arg2);

	//create task 3
	t3_tp.arg=0;
	t3_tp.period=1500;
	t3_tp.deadline=800;
	t3_tp.priority=50;
	t3_tp.dmiss=0;
	arg3=3;
	pthread_attr_init(&t3_attr);
	pthread_create(&t3_tid, &t3_attr, t_task, (void*)&arg3);

	//create task 4
	t4_tp.arg=0;
	t4_tp.period=1500;
	t4_tp.deadline=700;
	t4_tp.priority=40;
	t4_tp.dmiss=0;
	arg4=4;
	pthread_attr_init(&t4_attr);
	pthread_create(&t4_tid, &t4_attr, t_task, (void*)&arg4);

	draw_grafic_task_base('a');
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

void draw_grafic_task_base(char g)
{
int	i=0, j=0;
double xd;
struct timespec at;

	clock_gettime(CLOCK_MONOTONIC, &at);
	switch(g){
		case 'a':
			for(i=0; i<=N_TASK; i++){
				line(screen, ORIGIN_GRAFIC_X,(ORIGIN_PIP_Y-(i*(H_TASK+10))),(ORIGIN_GRAFIC_X+GRAFIC_W),(ORIGIN_PIP_Y-(i*(H_TASK+10))),0);
				line(screen, ORIGIN_GRAFIC_X,(ORIGIN_PIPW_Y-(i*(H_TASK+10))),(ORIGIN_GRAFIC_X+GRAFIC_W),(ORIGIN_PIPW_Y-(i*(H_TASK+10))),0);
				line(screen, ORIGIN_GRAFIC_X,(ORIGIN_PCP_Y-(i*(H_TASK+10))),(ORIGIN_GRAFIC_X+GRAFIC_W),(ORIGIN_PCP_Y-(i*(H_TASK+10))),0);
				line(screen, ORIGIN_GRAFIC_X,(ORIGIN_PCPW_Y-(i*(H_TASK+10))),(ORIGIN_GRAFIC_X+GRAFIC_W),(ORIGIN_PCPW_Y-(i*(H_TASK+10))),0);
			}

			xd=0;
			for(j=0; xd<GRAFIC_W; j++){
				xd=((t1_tp.deadline+(j*t1_tp.period))/time_scale)*5;
				line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(1*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(1*(H_TASK+10))-H_TASK-2),12);
				line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(1*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(1*(H_TASK+10))-H_TASK-2),12);
			}

			xd=0;
			for(j=0; xd<GRAFIC_W; j++){
				xd=((t2_tp.deadline+(j*t2_tp.period))/time_scale)*5;
				line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(2*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(2*(H_TASK+10))-H_TASK-2),12);
				line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(2*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(2*(H_TASK+10))-H_TASK-2),12);
			}

			xd=0;
			for(j=0; xd<GRAFIC_W; j++){
				xd=((t3_tp.deadline+(j*t3_tp.period))/time_scale)*5;
				line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(3*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(3*(H_TASK+10))-H_TASK-2),12);
				line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(3*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(3*(H_TASK+10))-H_TASK-2),12);
			}

			xd=0;
			for(j=0; xd<GRAFIC_W; j++){
				xd=((t4_tp.deadline+(j*t4_tp.period))/time_scale)*5;
				line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(4*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(4*(H_TASK+10))-H_TASK-2),12);
				line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(4*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(4*(H_TASK+10))-H_TASK-2),12);
			}
			break;
		case 'i':
			//cancella e ridisegna i grafici pip
			rectfill(screen, (ORIGIN_GRAFIC_X-5), (ORIGIN_PIP_Y-GRAFIC_H-2), (ORIGIN_GRAFIC_X+GRAFIC_W), (ORIGIN_PIPW_Y+5), 7);
			setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PIP_Y, "PIP");
			setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PIPW_Y, "PIP workload");
			for(i=0; i<=N_TASK; i++){
				line(screen, ORIGIN_GRAFIC_X,(ORIGIN_PIP_Y-(i*(H_TASK+10))),(ORIGIN_GRAFIC_X+GRAFIC_W),(ORIGIN_PIP_Y-(i*(H_TASK+10))),0);
				line(screen, ORIGIN_GRAFIC_X,(ORIGIN_PIPW_Y-(i*(H_TASK+10))),(ORIGIN_GRAFIC_X+GRAFIC_W),(ORIGIN_PIPW_Y-(i*(H_TASK+10))),0);
			}
			//disegna le varie deadline per ogni task nel grafico pip
			time_sub_ms(t1_tp.dl, at, &xd);
			xd=(xd/time_scale)*5;
			for(j=0; xd<GRAFIC_W; j++){
				line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(1*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(1*(H_TASK+10))-H_TASK),12);
				//xd+=(t1_tp.deadline/time_scale)*5;
				time_sub_ms(t1_tp.at, at, &xd);
				xd=((xd+t1_tp.deadline+(j*t1_tp.period))/time_scale)*5;
			}

			time_sub_ms(t2_tp.dl, at, &xd);
			xd=(xd/time_scale)*5;
			for(j=0; xd<GRAFIC_W; j++){
				line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(2*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(2*(H_TASK+10))-H_TASK),12);
				time_sub_ms(t2_tp.at, at, &xd);
				xd=((xd+t2_tp.deadline+(j*t2_tp.period))/time_scale)*5;
			}

			time_sub_ms(t3_tp.dl, at, &xd);
			xd=(xd/time_scale)*5;
			for(j=0; xd<GRAFIC_W; j++){
				line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(3*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(3*(H_TASK+10))-H_TASK),12);
				//xd+=(t3_tp.deadline/time_scale)*5;
				time_sub_ms(t3_tp.at, at, &xd);
				xd=((xd+t3_tp.deadline+(j*t3_tp.period))/time_scale)*5;
			}

			time_sub_ms(t4_tp.dl, at, &xd);
			xd=(xd/time_scale)*5;
			for(j=0; xd<GRAFIC_W; j++){
				line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(4*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(4*(H_TASK+10))-H_TASK),12);
				//xd+=(t4_tp.deadline/time_scale)*5;
				time_sub_ms(t4_tp.at, at, &xd);
				xd=((xd+t4_tp.deadline+(j*t4_tp.period))/time_scale)*5;
			}

			break;
		case 'c':
			//cancella e ridisegna i grafici pcp
			rectfill(screen, (ORIGIN_GRAFIC_X-5), (ORIGIN_PCP_Y-GRAFIC_H), (ORIGIN_GRAFIC_X+GRAFIC_W), (ORIGIN_PCPW_Y+5), 7);
			setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PCP_Y, "PCP");
			setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PCPW_Y, "PCP workload");
			for(i=0; i<=N_TASK; i++){
				line(screen, ORIGIN_GRAFIC_X,(ORIGIN_PCP_Y-(i*(H_TASK+10))),(ORIGIN_GRAFIC_X+GRAFIC_W),(ORIGIN_PCP_Y-(i*(H_TASK+10))),0);
				line(screen, ORIGIN_GRAFIC_X,(ORIGIN_PCPW_Y-(i*(H_TASK+10))),(ORIGIN_GRAFIC_X+GRAFIC_W),(ORIGIN_PCPW_Y-(i*(H_TASK+10))),0);
			}
			//disegna le varie deadline per ogni task nel grafico pcp
			time_sub_ms(t1_tp.dl, at, &xd);
			xd=(xd/time_scale)*5;
			for(j=0; xd<GRAFIC_W; j++){
				line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(1*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(1*(H_TASK+10))-H_TASK),12);
				xd+=(t1_tp.deadline/time_scale)*5;
			}

			time_sub_ms(t2_tp.dl, at, &xd);
			xd=(xd/time_scale)*5;
			for(j=0; xd<GRAFIC_W; j++){
				line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(2*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(2*(H_TASK+10))-H_TASK),12);
				xd+=(t2_tp.deadline/time_scale)*5;
			}

			time_sub_ms(t3_tp.dl, at, &xd);
			xd=(xd/time_scale)*5;
			for(j=0; xd<GRAFIC_W; j++){
				line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(3*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(3*(H_TASK+10))-H_TASK),12);
				xd+=(t3_tp.deadline/time_scale)*5;
			}

			time_sub_ms(t4_tp.dl, at, &xd);
			xd=(xd/time_scale)*5;
			for(j=0; xd<GRAFIC_W; j++){
				line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(4*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(4*(H_TASK+10))-H_TASK),12);
				xd+=(t4_tp.deadline/time_scale)*5;
			}

			break;
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
		t.tv_sec=0;
		t.tv_nsec=time_scale*1000000;
		if(pip){
			clock_gettime(CLOCK_MONOTONIC, &at);
			time_sub_ms(at, zero_time, &ms);
			//x=(ms/time_scale);
			rectfill(screen, (ORIGIN_GRAFIC_X+(x*5)), (ORIGIN_PIP_Y-(run_task*(H_TASK+10))), (ORIGIN_GRAFIC_X+(x*5)+5), (ORIGIN_PIP_Y-H_TASK-(run_task*(H_TASK+10))), 10);
			clock_nanosleep(CLOCK_MONOTONIC, 0, &t, NULL);
			x++;
			if((x*5)>=GRAFIC_W){
				x=0;
				draw_grafic_task_base('i');
			}
		}
		//pcp case
		else{
			clock_gettime(CLOCK_MONOTONIC, &at);
			time_sub_ms(at, zero_time, &ms);
			//x=(ms/time_scale);
			rectfill(screen, (ORIGIN_GRAFIC_X+(x*5)), (ORIGIN_PCP_Y-(run_task*(H_TASK+10))), (ORIGIN_GRAFIC_X+(x*5)+5), (ORIGIN_PCP_Y-H_TASK-(run_task*(H_TASK+10))), 10);
			clock_nanosleep(CLOCK_MONOTONIC, 0, &t, NULL);
			x++;
			if((x*5)>=GRAFIC_W){
				x=0;
				draw_grafic_task_base('c');
			}
		}
	}
}

//--------------------------------------------------------------------------
//GENERIC TASK
//--------------------------------------------------------------------------

void * t_task(void * arg)
{
int		*i;

	i=(int*)arg;

	while(1){
		run_task=*i;
	}
}
