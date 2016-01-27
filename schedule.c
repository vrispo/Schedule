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
void setup_grafic(int x, int y, char s[], bool ng);
void analysis_key(void);
void get_keycodes(char * scan, char * ascii);
void draw_grafic_task_base(char g);
void * grafic_task(void * arg);
void * t_task_1(void * arg);
void * t_task_2(void * arg);
void * t_task_3(void * arg);
void * t_task_4(void * arg);
void draw_deadline_pip(void);
void draw_deadline_pcp(void);
void draw_activation_pip(void);
void draw_activation_pcp(void);

//--------------------------------------------------------------------------
//GLOBAL VARIABLES
//--------------------------------------------------------------------------

//struct 			timespec zero_time;
int				time_scale=50;
int				x=0;
int				task[5];
int				nu=0;

bool			run=TRUE;
int				run_task;

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
struct	timespec t;
int 	delta = time_scale;

	t.tv_sec = 0;
	t.tv_nsec = delta*1000000;
	setup();

	while(run)
	{
		run_task=0;
		task[nu]=0;
		nu++;
		analysis_key();
		clock_nanosleep(CLOCK_MONOTONIC, 0, &t, NULL);
	}

	allegro_exit();
	return 0;
}

//--------------------------------------------------------------------------
//SETUP GRAFIC
//--------------------------------------------------------------------------

void setup_grafic(int x, int y, char s[], bool ng)
{
int		i = 0;
char	l[20];

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

	if(ng){
		for(i=0; i<=N_TASK; i++){
			sprintf(l,"%i",i);
			textout_ex(screen, font, l, (x-10), (y-(i*(H_TASK+10))),0, -1);
			line(screen, x,(y-(i*(H_TASK+10))),(x+GRAFIC_W),(y-(i*(H_TASK+10))),0);
		}
	}

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

	//clock_gettime(CLOCK_MONOTONIC, &zero_time);
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

	//create grafic task
	grafic_tp.arg=0;
	grafic_tp.period=time_scale;
	grafic_tp.deadline=10;
	grafic_tp.priority=20;
	grafic_tp.dmiss=0;

	pthread_attr_init(&grafic_attr);
	pthread_create(&grafic_tid, &grafic_attr, grafic_task, &grafic_tp);

	//create task 1
	t1_tp.arg=0;
	t1_tp.period=1200;
	t1_tp.deadline=1000;
	t1_tp.priority=60;
	t1_tp.dmiss=0;
	pthread_attr_init(&t1_attr);
	pthread_create(&t1_tid, &t1_attr, t_task_1, &t1_tp);

	//create task 2
	t2_tp.arg=0;
	t2_tp.period=1400;
	t2_tp.deadline=1200;
	t2_tp.priority=70;
	t2_tp.dmiss=0;
	pthread_attr_init(&t2_attr);
	pthread_create(&t2_tid, &t2_attr, t_task_2, &t2_tp);

	//create task 3
	t3_tp.arg=0;
	t3_tp.period=1600;
	t3_tp.deadline=1400;
	t3_tp.priority=80;
	t3_tp.dmiss=0;
	pthread_attr_init(&t3_attr);
	pthread_create(&t3_tid, &t3_attr, t_task_3, &t3_tp);

	//create task 4
	t4_tp.arg=0;
	t4_tp.period=1800;
	t4_tp.deadline=1600;
	t4_tp.priority=90;
	t4_tp.dmiss=0;
	pthread_attr_init(&t4_attr);
	pthread_create(&t4_tid, &t4_attr, t_task_4, &t4_tp);

	//grafici
	H_TASK=(GRAFIC_H-(N_TASK*10))/(N_TASK+1);
	ORIGIN_PIP_Y=INSTR_Y+INSTR_H+SPACE+GRAFIC_H;
	ORIGIN_PIPW_Y=ORIGIN_PIP_Y+SPACE+GRAFIC_H;
	ORIGIN_PCP_Y=ORIGIN_PIPW_Y+SPACE+GRAFIC_H;
	ORIGIN_PCPW_Y=ORIGIN_PCP_Y+SPACE+GRAFIC_H;
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
struct timespec at;

	clock_gettime(CLOCK_MONOTONIC, &at);
	switch(g){
		case 'a':
			setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PIP_Y, "PIP", true);
			setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PIPW_Y, "PIP workload", false);
			setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PCP_Y, "PCP", true);
			setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PCPW_Y, "PCP workload", false);
			draw_activation_pip();
			draw_activation_pcp();
			draw_deadline_pcp();
			draw_deadline_pip();
			break;
		case 'i':
			//cancella e ridisegna i grafici pip
			rectfill(screen, (ORIGIN_GRAFIC_X-5), (ORIGIN_PIP_Y-GRAFIC_H-2), (ORIGIN_GRAFIC_X+GRAFIC_W), (ORIGIN_PIPW_Y+5), 7);
			setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PIP_Y, "PIP", true);
			setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PIPW_Y, "PIP workload", false);

			draw_activation_pip();
			draw_deadline_pip();
			break;
		case 'c':
			//cancella e ridisegna i grafici pcp
			rectfill(screen, (ORIGIN_GRAFIC_X-5), (ORIGIN_PCP_Y-GRAFIC_H), (ORIGIN_GRAFIC_X+GRAFIC_W), (ORIGIN_PCPW_Y+5), 7);
			setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PCP_Y, "PCP", true);
			setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_PCPW_Y, "PCP workload", false);

			draw_activation_pcp();
			draw_deadline_pcp();
			break;
	}
}

//--------------------------------------------------------------------------
//GRAFIC TASK
//--------------------------------------------------------------------------

void * grafic_task(void * arg)
{
struct timespec	at;
int				i=0;
//double			ms;

	set_period(&grafic_tp);

	x=0;
	while(1){
		if(pip){
			clock_gettime(CLOCK_MONOTONIC, &at);
			//time_sub_ms(at, zero_time, &ms);
			//x=(ms/time_scale);
			rectfill(screen, (ORIGIN_GRAFIC_X+(x*5)), (ORIGIN_PIP_Y-(run_task*(H_TASK+10))), (ORIGIN_GRAFIC_X+(x*5)+5), (ORIGIN_PIP_Y-H_TASK-(run_task*(H_TASK+10))), 10);
			x++;
			if((x*5)>=GRAFIC_W){
				x=0;
				//clock_gettime(CLOCK_MONOTONIC, &zero_time);
				draw_grafic_task_base('i');
			}
		}
		//pcp case
		else{
			clock_gettime(CLOCK_MONOTONIC, &at);
			//time_sub_ms(at, zero_time, &ms);
			//x=(ms/time_scale);
			rectfill(screen, (ORIGIN_GRAFIC_X+(x*5)), (ORIGIN_PCP_Y-(run_task*(H_TASK+10))), (ORIGIN_GRAFIC_X+(x*5)+5), (ORIGIN_PCP_Y-H_TASK-(run_task*(H_TASK+10))), 10);
			x++;
			if((x*5)>=GRAFIC_W){
				x=0;
				//clock_gettime(CLOCK_MONOTONIC, &zero_time);
				draw_grafic_task_base('c');
			}
		}
		printf("nu=%d - task %d %d %d %d %d +++",nu,task[0],task[1],task[2],task[3],task[4]);
		nu=0;
		for(i=0; i<5; i++)
			task[i]=7;
		wait_for_period(&grafic_tp);
	}
}

//--------------------------------------------------------------------------
//GENERIC TASK 1
//--------------------------------------------------------------------------

void * t_task_1(void * arg)
{
struct task_par	*tp;

	tp= (struct task_par*)arg;
	set_period(tp);


	while(1){
		run_task=1;
		task[nu]=1;
		nu++;
		wait_for_period(tp);
	}
}

//--------------------------------------------------------------------------
//GENERIC TASK 2
//--------------------------------------------------------------------------

void * t_task_2(void * arg)
{
struct task_par	*tp;

	tp= (struct task_par*)arg;
	set_period(tp);

	while(1){
		run_task=2;
		task[nu]=2;
		nu++;
		wait_for_period(tp);
	}
}

//--------------------------------------------------------------------------
//GENERIC TASK 3
//--------------------------------------------------------------------------

void * t_task_3(void * arg)
{
struct task_par	*tp;

	tp= (struct task_par*)arg;
	set_period(tp);

	while(1){
		run_task=3;
		task[nu]=3;
		nu++;
		wait_for_period(tp);
	}
}

//--------------------------------------------------------------------------
//GENERIC TASK 4
//--------------------------------------------------------------------------

void * t_task_4(void * arg)
{
struct task_par	*tp;

	tp= (struct task_par*)arg;
	set_period(tp);

	while(1){
		run_task=4;
		task[nu]=4;
		nu++;
		wait_for_period(tp);
	}
}

//--------------------------------------------------------------------------
//DRAW DEADLINE OF A TASK
//--------------------------------------------------------------------------

void draw_deadline_pip(void)
{
int				j=0;
double			xd,nat;
struct timespec	at;

	clock_gettime(CLOCK_MONOTONIC, &at);

	//disegna le varie deadline per ogni task nel grafico pip
	xd=0;
	time_sub_ms(t1_tp.dl, at, &xd);
	time_sub_ms(t1_tp.at, at, &nat);
	xd=(xd/time_scale)*5;
	for(j=0; xd<GRAFIC_W; j++){
		line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(1*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(1*(H_TASK+10))-H_TASK),12);
		xd=((nat+t1_tp.deadline+(j*t1_tp.period))/time_scale)*5;
	}

	xd=0;
	time_sub_ms(t2_tp.dl, at, &xd);
	time_sub_ms(t2_tp.at, at, &nat);
	xd=(xd/time_scale)*5;
	for(j=0; xd<GRAFIC_W; j++){
		line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(2*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(2*(H_TASK+10))-H_TASK),12);
		xd=((nat+t2_tp.deadline+(j*t2_tp.period))/time_scale)*5;
	}

	xd=0;
	time_sub_ms(t3_tp.dl, at, &xd);
	time_sub_ms(t3_tp.at, at, &nat);
	xd=(xd/time_scale)*5;
	for(j=0; xd<GRAFIC_W; j++){
		line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(3*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(3*(H_TASK+10))-H_TASK),12);
		xd=((nat+t3_tp.deadline+(j*t3_tp.period))/time_scale)*5;
	}

	xd=0;
	time_sub_ms(t4_tp.dl, at, &xd);
	time_sub_ms(t4_tp.at, at, &nat);
	xd=(xd/time_scale)*5;
	for(j=0; xd<GRAFIC_W; j++){
		line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(4*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(4*(H_TASK+10))-H_TASK),12);
		xd=((nat+t4_tp.deadline+(j*t4_tp.period))/time_scale)*5;
	}
}


//--------------------------------------------------------------------------
//DRAW DEADLINE OF A TASK PCP
//--------------------------------------------------------------------------

void draw_deadline_pcp(void)
{
int				j=0;
double			xd,nat;
struct timespec	at;

	clock_gettime(CLOCK_MONOTONIC, &at);

	//disegna le varie deadline per ogni task nel grafico pcp
	xd=0;
	time_sub_ms(t1_tp.dl, at, &xd);
	time_sub_ms(t1_tp.at, at, &nat);
	xd=(xd/time_scale)*5;
	for(j=0; xd<GRAFIC_W; j++){
		line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(1*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(1*(H_TASK+10))-H_TASK),12);
		xd=((nat+t1_tp.deadline+(j*t1_tp.period))/time_scale)*5;
	}

	xd=0;
	time_sub_ms(t2_tp.dl, at, &xd);
	time_sub_ms(t2_tp.at, at, &nat);
	xd=(xd/time_scale)*5;
	for(j=0; xd<GRAFIC_W; j++){
		line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(2*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(2*(H_TASK+10))-H_TASK),12);
		xd=((nat+t2_tp.deadline+(j*t2_tp.period))/time_scale)*5;
	}

	xd=0;
	time_sub_ms(t3_tp.dl, at, &xd);
	time_sub_ms(t3_tp.at, at, &nat);
	xd=(xd/time_scale)*5;
	for(j=0; xd<GRAFIC_W; j++){
		line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(3*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(3*(H_TASK+10))-H_TASK),12);
		xd=((nat+t3_tp.deadline+(j*t3_tp.period))/time_scale)*5;
	}

	xd=0;
	time_sub_ms(t4_tp.dl, at, &xd);
	time_sub_ms(t4_tp.at, at, &nat);
	xd=(xd/time_scale)*5;
	for(j=0; xd<GRAFIC_W; j++){
		line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(4*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(4*(H_TASK+10))-H_TASK),12);
		xd=((nat+t4_tp.deadline+(j*t4_tp.period))/time_scale)*5;
	}
}

//--------------------------------------------------------------------------
//DRAW ACTIVATION OF A TASK
//--------------------------------------------------------------------------

void draw_activation_pip(void)
{
int				j=0;
double			xd,nat;
struct timespec	at;

	clock_gettime(CLOCK_MONOTONIC, &at);

	//disegna le varie deadline per ogni task nel grafico pip
	xd=0;
	time_sub_ms(t1_tp.at, at, &nat);
	xd=(nat/time_scale)*5;
	for(j=0; xd<GRAFIC_W; j++){
		line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(1*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(1*(H_TASK+10))-H_TASK),14);
		xd=((nat+(j*t1_tp.period))/time_scale)*5;
	}

	xd=0;
	time_sub_ms(t2_tp.at, at, &nat);
	xd=(nat/time_scale)*5;
	for(j=0; xd<GRAFIC_W; j++){
		line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(2*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(2*(H_TASK+10))-H_TASK),14);
		xd=((nat+(j*t2_tp.period))/time_scale)*5;
	}

	xd=0;
	time_sub_ms(t3_tp.at, at, &nat);
	xd=(nat/time_scale)*5;
	for(j=0; xd<GRAFIC_W; j++){
		line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(3*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(3*(H_TASK+10))-H_TASK),14);
		xd=((nat+(j*t3_tp.period))/time_scale)*5;
	}

	xd=0;
	time_sub_ms(t4_tp.at, at, &nat);
	xd=(nat/time_scale)*5;
	for(j=0; xd<GRAFIC_W; j++){
		line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(4*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PIP_Y-(4*(H_TASK+10))-H_TASK),14);
		xd=((nat+(j*t4_tp.period))/time_scale)*5;
	}
}


//--------------------------------------------------------------------------
//DRAW ACTIVATION OF A TASK PCP
//--------------------------------------------------------------------------

void draw_activation_pcp(void)
{
int				j=0;
double			xd,nat;
struct timespec	at;

	clock_gettime(CLOCK_MONOTONIC, &at);

	//disegna le varie deadline per ogni task nel grafico pcp
	xd=0;
	time_sub_ms(t1_tp.at, at, &nat);
	xd=(xd/time_scale)*5;
	for(j=0; xd<GRAFIC_W; j++){
		line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(1*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(1*(H_TASK+10))-H_TASK),14);
		xd=((nat+(j*t1_tp.period))/time_scale)*5;
	}

	xd=0;
	time_sub_ms(t2_tp.at, at, &nat);
	xd=(nat/time_scale)*5;
	for(j=0; xd<GRAFIC_W; j++){
		line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(2*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(2*(H_TASK+10))-H_TASK),14);
		xd=((nat+(j*t2_tp.period))/time_scale)*5;
	}

	xd=0;
	time_sub_ms(t3_tp.at, at, &nat);
	xd=(nat/time_scale)*5;
	for(j=0; xd<GRAFIC_W; j++){
		line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(3*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(3*(H_TASK+10))-H_TASK),14);
		xd=((nat+(j*t3_tp.period))/time_scale)*5;
	}

	xd=0;
	time_sub_ms(t4_tp.at, at, &nat);
	xd=(nat/time_scale)*5;
	for(j=0; xd<GRAFIC_W; j++){
		line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(4*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_PCP_Y-(4*(H_TASK+10))-H_TASK),14);
		xd=((nat+(j*t4_tp.period))/time_scale)*5;
	}
}

