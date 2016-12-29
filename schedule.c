#define _GNU_SOURCE

#include <features.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <unistd.h>
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

#define N_TASK			4

//--------------------------------------------------------------------------
//TYPE DEFINITIONS
//--------------------------------------------------------------------------



//--------------------------------------------------------------------------
//FUNCTION DECLARATIONS
//--------------------------------------------------------------------------

void setup (void);
void setup_grafic(int x, int y, char s[], bool ng);
void draw_task_parameter(int mod);
void write_instruction(void);

void analysis_key(void);
void get_keycodes(char * scan, char * ascii);

void control_CPU(char *task_name, pthread_t	thread);

void create_task(void);
void create_task_1(void);
void create_task_2(void);
void create_task_3(void);
void create_task_4(void);
void stop_task(void);

void change_time_scale(void);

void multi_exec(int n, int gx, int gy);
void current_task(int tsk);

void draw_resource(int gx, int gy);

void * grafic_task(void * arg);
void * t_task_1(void * arg);
void * t_task_2(void * arg);
void * t_task_3(void * arg);
void * t_task_4(void * arg);
void * workload_task(void * arg);

void draw_deadline(struct task_par tp, int i, int mod);
void draw_activation(struct task_par tp,int i, int mod);

void create_mux_pip(void);
void create_mux_pcp(void);

//--------------------------------------------------------------------------
//GLOBAL VARIABLES
//--------------------------------------------------------------------------

int					time_scale[3] = {50, 75, 100};
int					pox_ts = 0;

//struct 			timespec zero_time;
int					x = 0;
int					task[5];
int					nu = 0;

bool				use = false;
int					free_ms = 0;	//number of ms that the CPU is free
double				wl = 0;			//actual workload
double				pwl = 0;		//previous workload

bool				run = TRUE;
int					run_task;
int					stop=0;

bool				pip = TRUE;

//#0:PIP #1:PCP
int					ORIGIN_Y[2];
int					ORIGIN_WL_Y[2];

int					H_TASK;

pthread_t			grafic_tid;
struct task_par		grafic_tp;
pthread_attr_t		grafic_attr;
struct sched_param	grafic_par;

pthread_t			t1_tid;
struct task_par		t1_tp;
pthread_attr_t		t1_attr;
struct sched_param	t1_par;

pthread_t			t2_tid;
struct task_par		t2_tp;
pthread_attr_t		t2_attr;
struct sched_param	t2_par;

pthread_t			t3_tid;
struct task_par		t3_tp;
pthread_attr_t		t3_attr;
struct sched_param	t3_par;

pthread_t			t4_tid;
struct task_par		t4_tp;
pthread_attr_t		t4_attr;
struct sched_param	t4_par;

pthread_t			workload_tid;
struct task_par		workload_tp;
pthread_attr_t		workload_attr;
struct sched_param	workload_par;

int					max_prio_a = 90;
int					max_prio_b = 70;
int					a = 0;
int					b = 0;

pthread_mutex_t		mux_a_pip;
pthread_mutex_t 	mux_b_pip;
pthread_mutexattr_t	mattr_pip;

pthread_mutex_t		mux_a_pcp;
pthread_mutex_t 	mux_b_pcp;
pthread_mutexattr_t	mattr_pcp;

//--------------------------------------------------------------------------
//FUNCTION DEFINITIONS
//--------------------------------------------------------------------------

int main(int argc, char * argv[])
{
struct		timespec t;
int 		delta = 500;
cpu_set_t	cpuset;
pthread_t	thread;

    thread = pthread_self();
	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);
	if (pthread_setaffinity_np(thread, sizeof(cpuset), &cpuset) != 0)
		perror("pthread_setaffinity_np");

	t.tv_sec = 0;
	t.tv_nsec = delta*1000000;
	setup();

	while(run)
	{
		use = true;
		run_task=0;
		task[nu]=0;
		nu++;
		analysis_key();

		control_CPU("MAIN",thread);
		clock_nanosleep(CLOCK_MONOTONIC, 0, &t, NULL);
	}

	allegro_exit();
	return 0;
}

//--------------------------------------------------------------------------
//CONTROL IN THE TASK IS RUNNING ON CORRECT CPU
//--------------------------------------------------------------------------

void control_CPU(char *task_name, pthread_t	thread)
{
cpu_set_t	cpuget;
int			ncpu;
int			cpu;

	cpu = sched_getcpu();
	if(cpu!=0)
		printf("ERROR corrent cpu %s", task_name);
	pthread_getaffinity_np(thread,sizeof(cpuget),&cpuget);
	ncpu=CPU_COUNT(&cpuget);
	if(ncpu!=1){
		printf("ERROR more than one elegible CPU for: %s", task_name);
	}
	if(CPU_ISSET(0, &cpuget)==0){
		printf("ERROR cpu 0 not in the elegible set of: %s", task_name);
	}
}

//--------------------------------------------------------------------------
//DRAW EMPTY GRAFIC
//--------------------------------------------------------------------------

void setup_grafic(int x, int y, char s[], bool ng)
{
int		i = 0;
char	l[2];

	rectfill(screen, (x-5), (y-GRAFIC_H-2), (x+GRAFIC_W), (y+5), 7);

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
//WRITE THE INSTRUCTIONS
//--------------------------------------------------------------------------

void write_instruction(void)
{
char s[60];

	rectfill(screen, INSTR_X, INSTR_Y, (INSTR_X+INSTR_W), (INSTR_Y+INSTR_H), 7);

	textout_ex(screen, font, "INSTRUCTIONS", (INSTR_X+7), (INSTR_Y-5), 0, -1);
	line(screen, INSTR_X, INSTR_Y, (INSTR_X+5), INSTR_Y, 0);
	line(screen, (INSTR_X+5+INSTR_L), INSTR_Y, (INSTR_X+INSTR_W), INSTR_Y, 0);
	line(screen, INSTR_X, INSTR_Y, INSTR_X, (INSTR_Y+INSTR_H), 0);
	line(screen, INSTR_X, (INSTR_Y+INSTR_H), (INSTR_X+INSTR_W), (INSTR_Y+INSTR_H), 0);
	line(screen, (INSTR_X+INSTR_W), (INSTR_Y+INSTR_H), (INSTR_X+INSTR_W), 10, 0);

	textout_ex(screen, font, "PRESS KEY ESC TO EXIT", (INSTR_X+10), (INSTR_Y+10), 0, -1);

	sprintf(s, "UNITA' MISURA -> %i ms = ", time_scale[pox_ts]);
	textout_ex(screen, font, s, (INSTR_X+10), (INSTR_Y+20), 0, -1);
	line(screen, (INSTR_X+210), (INSTR_Y+22.5), (INSTR_X+215), (INSTR_Y+22.5), 0);
	textout_ex(screen, font,"PRESS UP ARROW TO CHANGE (50 - 75 - 100 ms)" , (INSTR_X+220), (INSTR_Y+20), 0, -1);

	sprintf(s, "possesso risorsa a: ");
	textout_ex(screen, font, s, (INSTR_X+10), (INSTR_Y+30), 0, -1);
	rectfill(screen, (INSTR_X+165), (INSTR_Y+32.5), (INSTR_X+170), (INSTR_Y+37.5), 1);
	sprintf(s, "possesso risorsa b: ");
	textout_ex(screen, font, s, (INSTR_X+180), (INSTR_Y+30), 0, -1);
	rectfill(screen, (INSTR_X+335), (INSTR_Y+32.5), (INSTR_X+340), (INSTR_Y+37.5), 9);
	sprintf(s, "possesso risorsa a e b: ");
	textout_ex(screen, font, s, (INSTR_X+350), (INSTR_Y+30), 0, -1);
	rectfill(screen, (INSTR_X+540), (INSTR_Y+32.5), (INSTR_X+545), (INSTR_Y+37.5), 13);

	sprintf(s, "PRESS RIGHT ARROW TO CHANGE FROM PCP TO PIP AND VICEVERSA");
	textout_ex(screen, font, s, (INSTR_X+10), (INSTR_Y+40), 0, -1);
}

//--------------------------------------------------------------------------
//SETUP
//--------------------------------------------------------------------------

void setup(void)
{
cpu_set_t	cpuset;
FILE		*f_sched_budget;

	allegro_init();
	install_keyboard();

	set_color_depth(8);
	set_gfx_mode(GFX_AUTODETECT_WINDOWED, WINDOW_W, WINDOW_H, 0, 0);
	clear_to_color(screen, 7);

	//clock_gettime(CLOCK_MONOTONIC, &zero_time);
	run_task=0;

	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);
	f_sched_budget=fopen("/proc/sys/kernel/sched_rt_runtime_us","w");
	fprintf(f_sched_budget,"1000000");
	fclose(f_sched_budget);


	write_instruction();

	//grafici
	H_TASK=(GRAFIC_H-(N_TASK*10))/(N_TASK+1);
	ORIGIN_Y[0]=INSTR_Y+INSTR_H+SPACE+GRAFIC_H;
	ORIGIN_WL_Y[0]=ORIGIN_Y[0]+SPACE+GRAFIC_H;
	ORIGIN_Y[1]=ORIGIN_WL_Y[0]+SPACE+GRAFIC_H;
	ORIGIN_WL_Y[1]=ORIGIN_Y[1]+SPACE+GRAFIC_H;
	setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_Y[0], "PIP", true);
	setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_WL_Y[0], "PIP workload", false);
	setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_Y[1], "PCP", true);
	setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_WL_Y[1], "PCP workload", false);

	//create workload task
	workload_tp.arg=0;
	workload_tp.period=1;
	workload_tp.deadline=1;
	workload_tp.priority=1;
	workload_tp.dmiss=0;

	pthread_attr_init(&workload_attr);
	pthread_attr_setdetachstate(&workload_attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setaffinity_np(&workload_attr, sizeof(cpuset), &cpuset);
	pthread_attr_setinheritsched(&workload_attr, PTHREAD_EXPLICIT_SCHED);
	workload_par.__sched_priority=workload_tp.priority;
	pthread_attr_setschedparam(&workload_attr, &workload_par);
	pthread_create(&grafic_tid, &workload_attr, workload_task, &workload_tp);

	//create grafic task
	grafic_tp.arg=0;
	grafic_tp.period=time_scale[pox_ts];
	grafic_tp.deadline=10;
	grafic_tp.priority=99;
	grafic_tp.dmiss=0;

	pthread_attr_init(&grafic_attr);
	pthread_attr_setdetachstate(&grafic_attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&grafic_tid, &grafic_attr, grafic_task, &grafic_tp);

	create_task();
}

//--------------------------------------------------------------------------
//CREATE TASK
//--------------------------------------------------------------------------

void create_task(void)
{
struct		timespec t,at;
int			delta = 0;

	stop=0;

	if(pip){
		create_mux_pip();
	}
	else{
		create_mux_pcp();
	}

	create_task_1();

	/*delta = 150;
	t.tv_sec = 0;
	t.tv_nsec = delta*1000000;
	clock_nanosleep(CLOCK_MONOTONIC, 0, &t, NULL);*/
	
	clock_gettime(CLOCK_MONOTONIC, &t);
	time_add_ms(&t, 150);
	do{
		clock_gettime(CLOCK_MONOTONIC, &at);
	}while(time_cmp(at, t)<=0);

	create_task_2();

	/*delta = 50;
	t.tv_sec = 0;
	t.tv_nsec = delta*1000000;
	clock_nanosleep(CLOCK_MONOTONIC, 0, &t, NULL);*/
	
	clock_gettime(CLOCK_MONOTONIC, &t);
	time_add_ms(&t, 50);
	do{
		clock_gettime(CLOCK_MONOTONIC, &at);
	}while(time_cmp(at, t)<=0);

	create_task_3();

	/*delta = 50;
	t.tv_sec = 0;
	t.tv_nsec = delta*1000000;
	clock_nanosleep(CLOCK_MONOTONIC, 0, &t, NULL);*/
	
	clock_gettime(CLOCK_MONOTONIC, &t);
	time_add_ms(&t, 50);
	do{
		clock_gettime(CLOCK_MONOTONIC, &at);
	}while(time_cmp(at, t)<=0);

	create_task_4();
}

void create_task_1(void)
{
cpu_set_t	cpuset;
int			mod;

	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);

	//create task 1
	t1_tp.arg=0;
	t1_tp.period=1200;
	t1_tp.deadline=1000;
	t1_tp.priority=60;
	t1_tp.dmiss=0;
	pthread_attr_init(&t1_attr);
	pthread_attr_setdetachstate(&t1_attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setaffinity_np(&t1_attr, sizeof(cpuset), &cpuset);
	pthread_attr_setinheritsched(&t1_attr, PTHREAD_EXPLICIT_SCHED);
	t1_par.__sched_priority=t1_tp.priority;
	pthread_attr_setschedparam(&t1_attr, &t1_par);
	pthread_create(&t1_tid, &t1_attr, t_task_1, &t1_tp);

	if(pip)
		mod=0;
	else
		mod=1;
	draw_deadline(t1_tp, 1, mod);
	draw_activation(t1_tp, 1, mod);
}

void create_task_2(void)
{
cpu_set_t	cpuset;
int			mod;

	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);

	//create task 2
	t2_tp.arg=0;
	t2_tp.period=1200;
	t2_tp.deadline=1000;
	t2_tp.priority=70;
	t2_tp.dmiss=0;
	pthread_attr_init(&t2_attr);
	pthread_attr_setdetachstate(&t2_attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setaffinity_np(&t2_attr, sizeof(cpuset), &cpuset);
	pthread_attr_setinheritsched(&t2_attr, PTHREAD_EXPLICIT_SCHED);
	t2_par.__sched_priority=t2_tp.priority;
	pthread_attr_setschedparam(&t2_attr, &t2_par);
	pthread_create(&t2_tid, &t2_attr, t_task_2, &t2_tp);

	if(pip)
		mod=0;
	else
		mod=1;
	draw_deadline(t2_tp, 2, mod);
	draw_activation(t2_tp, 2, mod);
}

void create_task_3(void)
{
cpu_set_t	cpuset;
int			mod;

	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);

	//create task 3
	t3_tp.arg=0;
	t3_tp.period=1200;
	t3_tp.deadline=1000;
	t3_tp.priority=80;
	t3_tp.dmiss=0;
	pthread_attr_init(&t3_attr);
	pthread_attr_setdetachstate(&t3_attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setaffinity_np(&t3_attr, sizeof(cpuset), &cpuset);
	pthread_attr_setinheritsched(&t3_attr, PTHREAD_EXPLICIT_SCHED);
	t3_par.__sched_priority=t3_tp.priority;
	pthread_attr_setschedparam(&t3_attr, &t3_par);
	pthread_create(&t3_tid, &t3_attr, t_task_3, &t3_tp);

	if(pip)
		mod=0;
	else
		mod=1;
	draw_deadline(t3_tp, 3, mod);
	draw_activation(t3_tp, 3, mod);	
}

void create_task_4(void)
{
cpu_set_t	cpuset;
int			mod;

	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);

	//create task 4
	t4_tp.arg=0;
	t4_tp.period=1200;
	t4_tp.deadline=1000;
	t4_tp.priority=90;
	t4_tp.dmiss=0;
	pthread_attr_init(&t4_attr);
	pthread_attr_setdetachstate(&t4_attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setaffinity_np(&t4_attr, sizeof(cpuset), &cpuset);
	pthread_attr_setinheritsched(&t4_attr, PTHREAD_EXPLICIT_SCHED);
	t4_par.__sched_priority=t4_tp.priority;
	pthread_attr_setschedparam(&t4_attr, &t4_par);
	pthread_create(&t4_tid, &t4_attr, t_task_4, &t4_tp);

	if(pip)
		mod=0;
	else
		mod=1;
	draw_deadline(t4_tp, 4, mod);
	draw_activation(t4_tp, 4, mod);
}

//--------------------------------------------------------------------------
//STOP TASK
//--------------------------------------------------------------------------

void stop_task(void)
{
	stop=1;
	pthread_cancel(t1_tid);
	pthread_cancel(t2_tid);
	pthread_cancel(t3_tid);
	pthread_cancel(t4_tid);

	pthread_mutex_destroy(&mux_a_pip);
	pthread_mutex_destroy(&mux_b_pip);
	pthread_mutex_destroy(&mux_a_pcp);
	pthread_mutex_destroy(&mux_b_pcp);
}

//--------------------------------------------------------------------------
//CHANGE TIME SCALE
//--------------------------------------------------------------------------

void change_time_scale(void)
{
	pox_ts++;
	if(pox_ts==3){
		pox_ts = 0;
	}

	pthread_cancel(grafic_tid);
	write_instruction();

	grafic_tp.arg=0;
	grafic_tp.period=time_scale[pox_ts];
	grafic_tp.deadline=10;
	grafic_tp.priority=99;
	grafic_tp.dmiss=0;

	pthread_attr_init(&grafic_attr);
	pthread_attr_setdetachstate(&grafic_attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&grafic_tid, &grafic_attr, grafic_task, &grafic_tp);
}

//--------------------------------------------------------------------------
//CREATE A PIP MUTEX
//--------------------------------------------------------------------------

void create_mux_pip(void)
{
	pthread_mutexattr_init(&mattr_pip);
	pthread_mutexattr_setprotocol(&mattr_pip, PTHREAD_PRIO_INHERIT);
	pthread_mutex_init(&mux_a_pip, &mattr_pip);
	pthread_mutex_init(&mux_b_pip, &mattr_pip);
	pthread_mutexattr_destroy(&mattr_pip);
}

//--------------------------------------------------------------------------
//CREATE A PCP MUTEX
//--------------------------------------------------------------------------

void create_mux_pcp(void)
{
	pthread_mutexattr_init(&mattr_pcp);
	pthread_mutexattr_setprotocol(&mattr_pcp, PTHREAD_PRIO_PROTECT);
	pthread_mutexattr_setprioceiling(&mattr_pcp, max_prio_a);
	pthread_mutex_init(&mux_a_pcp, &mattr_pcp);
	pthread_mutexattr_setprioceiling(&mattr_pcp, max_prio_b);
	pthread_mutex_init(&mux_b_pcp, &mattr_pcp);
	pthread_mutexattr_destroy(&mattr_pip);
}

//--------------------------------------------------------------------------
//DRAW PARAMETER TASK IN GRAFIC PIP OR PCP
//--------------------------------------------------------------------------

void draw_task_parameter(int mod)
{
	draw_activation(t1_tp, 1, mod);
	draw_activation(t2_tp, 2, mod);
	draw_activation(t3_tp, 3, mod);
	draw_activation(t4_tp, 4, mod);
	draw_deadline(t1_tp, 1, mod);
	draw_deadline(t2_tp, 2, mod);
	draw_deadline(t3_tp, 3, mod);
	draw_deadline(t4_tp, 4, mod);
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
			{
				run=FALSE;
				break;
			}
			case KEY_SPACE:
			{	if(stop){
					if(pip){
						setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_Y[0], "PIP", true);
						setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_WL_Y[0], "PIP workload", false);
					}
					else{
						setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_Y[1], "PCP", true);
						setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_WL_Y[1], "PCP workload", false);
					}
					pwl = 0;
					wl = 0;
					free_ms = 0;
					x=0;
					a=0;
					b=0;
					create_task();
				}
				else
					stop_task();
				break;
			}
			case KEY_RIGHT:
			{
				if(pip){
					stop_task();
					setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_Y[1], "PCP", true);
					setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_WL_Y[1], "PCP workload", false);
					pwl = 0;
					wl = 0;
					free_ms = 0;
					x=0;
					a=0;
					b=0;
					pip=false;
					create_task();
				}
				else{
					stop_task();
					setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_Y[0], "PIP", true);
					setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_WL_Y[0], "PIP workload", false);
					pwl = 0;
					wl = 0;
					free_ms = 0;
					x=0;
					a=0;
					b=0;
					pip=true;
					create_task();
				}
				break;
			}
			case KEY_UP:
			{
				change_time_scale();
				break;
			}
			default:
				break;
		}
	}
}

//--------------------------------------------------------------------------
//DRAW DEADLINE OF A TASK IN PIP(MOD #0) OR PCP(MOD #1) GRAFIC
//--------------------------------------------------------------------------

void draw_deadline(struct task_par tp, int i, int mod)
{
int				j=0;
double			xd,nat;
struct timespec	at;

	clock_gettime(CLOCK_MONOTONIC, &at);

	xd=0;
	time_sub_ms(tp.dl, at, &xd);
	time_sub_ms(tp.at, at, &nat);
	xd=x+((xd/time_scale[pox_ts])*5);
	for(j=0; xd<GRAFIC_W; j++){
		line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_Y[mod]-(i*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_Y[mod]-(i*(H_TASK+10))-H_TASK),12);
		xd=x+(((nat+tp.deadline+(j*tp.period))/time_scale[pox_ts])*5);
	}
}

//--------------------------------------------------------------------------
//DRAW ACTIVATION OF A TASK IN PIP(MOD #0) OR PCP(MOD #1) GRAFIC
//--------------------------------------------------------------------------

void draw_activation(struct task_par tp, int i, int mod)
{
int				j=0;
double			xd,nat;
struct timespec	at;

	clock_gettime(CLOCK_MONOTONIC, &at);

	//disegna le varie deadline per ogni task nel grafico pip
	xd=0;
	time_sub_ms(tp.at, at, &nat);
	xd=x+((nat/time_scale[pox_ts])*5);
	for(j=0; xd<GRAFIC_W; j++){
		line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_Y[mod]-(i*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_Y[mod]-(i*(H_TASK+10))-H_TASK),14);
		xd=x+(((nat+(j*tp.period))/time_scale[pox_ts])*5);
	}
}

//--------------------------------------------------------------------------
//HANDLING MULTI EXEC TASK
//--------------------------------------------------------------------------

void multi_exec(int n, int gx, int gy)
{
	switch(n)
	{
		case 1:
		{
			rectfill(screen, (gx+(x*5)), (gy-(task[0]*(H_TASK+10))), (gx+(x*5)+5), (gy-(H_TASK/2)-(task[0]*(H_TASK+10))), 10);
			break;
		}
		case 2:
		{
			rectfill(screen, (gx+(x*5)), (gy-(task[0]*(H_TASK+10))), (gx+(x*5)+2), (gy-(H_TASK/2)-(task[0]*(H_TASK+10))), 10);
			rectfill(screen, (gx+(x*5)+2), (gy-(task[1]*(H_TASK+10))), (gx+(x*5)+5), (gy-(H_TASK/2)-(task[1]*(H_TASK+10))), 10);
			break;
		}
		case 3:
		{
			rectfill(screen, (gx+(x*5)), (gy-(task[0]*(H_TASK+10))), (gx+(x*5)+1), (gy-(H_TASK/2)-(task[0]*(H_TASK+10))), 10);
			rectfill(screen, (gx+(x*5)+1), (gy-(task[1]*(H_TASK+10))), (gx+(x*5)+3), (gy-(H_TASK/2)-(task[1]*(H_TASK+10))), 10);
			rectfill(screen, (gx+(x*5)+3), (gy-(task[2]*(H_TASK+10))), (gx+(x*5)+5), (gy-(H_TASK/2)-(task[2]*(H_TASK+10))), 10);
			break;
		}
		case 4:
		{
			rectfill(screen, (gx+(x*5)), (gy-(task[0]*(H_TASK+10))), (gx+(x*5)+1), (gy-(H_TASK/2)-(task[0]*(H_TASK+10))), 10);
			rectfill(screen, (gx+(x*5)+1), (gy-(task[1]*(H_TASK+10))), (gx+(x*5)+2), (gy-(H_TASK/2)-(task[1]*(H_TASK+10))), 10);
			rectfill(screen, (gx+(x*5)+2), (gy-(task[2]*(H_TASK+10))), (gx+(x*5)+3), (gy-(H_TASK/2)-(task[2]*(H_TASK+10))), 10);
			rectfill(screen, (gx+(x*5)+3), (gy-(task[3]*(H_TASK+10))), (gx+(x*5)+5), (gy-(H_TASK/2)-(task[3]*(H_TASK+10))), 10);
			break;
		}
		case 5:
		{
			rectfill(screen, (gx+(x*5)), (gy-(task[0]*(H_TASK+10))), (gx+(x*5)+1), (gy-(H_TASK/2)-(task[0]*(H_TASK+10))), 10);
			rectfill(screen, (gx+(x*5)+1), (gy-(task[1]*(H_TASK+10))), (gx+(x*5)+2), (gy-(H_TASK/2)-(task[1]*(H_TASK+10))), 10);
			rectfill(screen, (gx+(x*5)+2), (gy-(task[2]*(H_TASK+10))), (gx+(x*5)+3), (gy-(H_TASK/2)-(task[2]*(H_TASK+10))), 10);
			rectfill(screen, (gx+(x*5)+3), (gy-(task[3]*(H_TASK+10))), (gx+(x*5)+4), (gy-(H_TASK/2)-(task[3]*(H_TASK+10))), 10);
			rectfill(screen, (gx+(x*5)+4), (gy-(task[4]*(H_TASK+10))), (gx+(x*5)+5), (gy-(H_TASK/2)-(task[4]*(H_TASK+10))), 10);
			break;
		}
	}
}

//--------------------------------------------------------------------------
//DRAW RESOURCE OWNER
//--------------------------------------------------------------------------

void draw_resource(int gx, int gy)
{
int col = 10;

	//se entrambi dello stesso processo
	if((a!=0)&(b==a)){
		col = 13;
		rectfill(screen, (gx+(x*5)), (gy-(a*(H_TASK+10))), (gx+(x*5)+5), (gy-(H_TASK/4)-(a*(H_TASK+10))), col);
	}
	//se a di un processo
	if((a!=0)&(b!=a)){
		col = 1;
		rectfill(screen, (gx+(x*5)), (gy-(a*(H_TASK+10))), (gx+(x*5)+5), (gy-(H_TASK/4)-(a*(H_TASK+10))), col);
	}
	//se b di un processo
	if((b!=0)&(b!=a)){
		col = 9;
		rectfill(screen, (gx+(x*5)), (gy-(b*(H_TASK+10))), (gx+(x*5)+5), (gy-(H_TASK/4)-(b*(H_TASK+10))), col);
	}
}

//--------------------------------------------------------------------------
//CURRENT TASK
//--------------------------------------------------------------------------

void current_task(int tsk)
{
bool	task_r = false;
int		i;

	use=true;
	run_task=tsk;
	for(i= 0; i < nu; i++)
	{
		if(task[i]==tsk)
		{
			task_r = true;
		}
	}
	if(task_r == false)
	{
		task[nu] = tsk;
		nu++;
	}
}

//--------------------------------------------------------------------------
//GRAFIC TASK
//--------------------------------------------------------------------------

void * grafic_task(void * arg)
{
struct timespec	at;
int				i=0;
int				mod;

	set_period(&grafic_tp);

	while(1){
		if(!stop){
			//calcola workload e fai grafico
			pwl = wl;
			wl = 1 - ((double)free_ms/(double)time_scale[pox_ts]);
			//printf("freems=%d wl=%f nu=%d+ ", free_ms, wl, nu);
			free_ms = 0;
			
			if(pip)
				mod=0;
			else
				mod=1;
				
			clock_gettime(CLOCK_MONOTONIC, &at);

			if(run_task!=7)
				multi_exec(nu, ORIGIN_GRAFIC_X, ORIGIN_Y[mod]);

			draw_resource(ORIGIN_GRAFIC_X, ORIGIN_Y[mod]);

			line(screen, (ORIGIN_GRAFIC_X+((x-1)*5)), (ORIGIN_WL_Y[mod]-(GRAFIC_H*pwl)), (ORIGIN_GRAFIC_X+(x*5)), (ORIGIN_WL_Y[mod]-(GRAFIC_H*wl)), 4);

			run_task = 7;
			x++;
			if((x*5)>=GRAFIC_W){
				x=0;
				if(mod==0){					
					setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_Y[0], "PIP", true);
					setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_WL_Y[0], "PIP workload", false);
					draw_task_parameter(mod);
				}
				else{
					setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_Y[1], "PCP", true);
					setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_WL_Y[1], "PCP workload", false);
					draw_task_parameter(mod);
				}			
			}
		}
		//printf("nu=%d - task %d %d %d %d %d +++",nu,task[0],task[1],task[2],task[3],task[4]);
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
struct 	task_par *tp;
struct	timespec t;
struct	timespec at;

	tp= (struct task_par*)arg;
	set_period(tp);

	while(1){
		use = true;
		run_task=1;
		task[nu]=1;
		nu++;
		control_CPU("TASK 1",pthread_self());
		if(pip){
			pthread_mutex_lock(&mux_a_pip);
			clock_gettime(CLOCK_MONOTONIC, &t);
			time_add_ms(&t, 100);
			do{
				clock_gettime(CLOCK_MONOTONIC, &at);
				a = 1;
				current_task(1);
			}while(time_cmp(at, t)<=0);

			pthread_mutex_lock(&mux_b_pip);
			clock_gettime(CLOCK_MONOTONIC, &t);
			time_add_ms(&t, 400);
			do{
				clock_gettime(CLOCK_MONOTONIC, &at);
				b = 1;
				current_task(1);
			}while(time_cmp(at, t)<=0);

			b=0;
			pthread_mutex_unlock(&mux_b_pip);

			clock_gettime(CLOCK_MONOTONIC, &t);
			time_add_ms(&t, 100);
			do{
				clock_gettime(CLOCK_MONOTONIC, &at);
				current_task(1);
			}while(time_cmp(at, t)<=0);

			a = 0;
			pthread_mutex_unlock(&mux_a_pip);
		}
		else{
			pthread_mutex_lock(&mux_a_pcp);
			clock_gettime(CLOCK_MONOTONIC, &t);
			time_add_ms(&t, 100);
			do{
				clock_gettime(CLOCK_MONOTONIC, &at);
				a = 1;
				current_task(1);
			}while(time_cmp(at, t)<=0);

			pthread_mutex_lock(&mux_b_pcp);
			clock_gettime(CLOCK_MONOTONIC, &t);
			time_add_ms(&t, 400);
			do{
				clock_gettime(CLOCK_MONOTONIC, &at);
				b = 1;
				current_task(1);
			}while(time_cmp(at, t)<=0);

			b=0;
			pthread_mutex_unlock(&mux_b_pcp);

			clock_gettime(CLOCK_MONOTONIC, &t);
			time_add_ms(&t, 100);
			do{
				clock_gettime(CLOCK_MONOTONIC, &at);
				current_task(1);
			}while(time_cmp(at, t)<=0);

			a = 0;
			pthread_mutex_unlock(&mux_a_pcp);
		}
		wait_for_period(tp);
	}
}

//--------------------------------------------------------------------------
//GENERIC TASK 2
//--------------------------------------------------------------------------

void * t_task_2(void * arg)
{
struct	task_par	*tp;
struct	timespec t;
struct	timespec at;

	tp= (struct task_par*)arg;
	set_period(tp);

	while(1){
		use = true;
		run_task=2;
		task[nu]=2;
		nu++;
		control_CPU("TASK 2",pthread_self());
		if(pip){
			pthread_mutex_lock(&mux_b_pip);

			clock_gettime(CLOCK_MONOTONIC, &t);
			time_add_ms(&t, 300);
			do{
				clock_gettime(CLOCK_MONOTONIC, &at);
				b = 2;
				current_task(2);
			}while(time_cmp(at, t)<=0);
			b = 0;

			pthread_mutex_unlock(&mux_b_pip);
		}
		else{
			pthread_mutex_lock(&mux_b_pcp);

			clock_gettime(CLOCK_MONOTONIC, &t);
			time_add_ms(&t, 300);
			do{
				clock_gettime(CLOCK_MONOTONIC, &at);
				b = 2;
				current_task(2);
			}while(time_cmp(at, t)<=0);
			b = 0;

			pthread_mutex_unlock(&mux_b_pcp);
		}
		wait_for_period(tp);
	}
}

//--------------------------------------------------------------------------
//GENERIC TASK 3
//--------------------------------------------------------------------------

void * t_task_3(void * arg)
{
struct	task_par	*tp;
struct	timespec t;
struct	timespec at;

	tp= (struct task_par*)arg;
	set_period(tp);

	while(1){
		use = true;
		run_task=3;
		task[nu]=3;
		nu++;

		clock_gettime(CLOCK_MONOTONIC, &t);
		time_add_ms(&t, 300);
		do{
			clock_gettime(CLOCK_MONOTONIC, &at);
			current_task(3);
		}while(time_cmp(at, t)<=0);

		control_CPU("TASK 3",pthread_self());
		wait_for_period(tp);
	}
}

//--------------------------------------------------------------------------
//GENERIC TASK 4
//--------------------------------------------------------------------------

void * t_task_4(void * arg)
{
struct	task_par	*tp;
struct	timespec t;
struct	timespec at;

	tp= (struct task_par*)arg;
	set_period(tp);

	while(1){
		use = true;
		run_task=4;
		task[nu]=4;
		control_CPU("TASK 4",pthread_self());
		nu++;
		if(pip){
			pthread_mutex_lock(&mux_a_pip);

			clock_gettime(CLOCK_MONOTONIC, &t);
			time_add_ms(&t, 300);
			do{
				clock_gettime(CLOCK_MONOTONIC, &at);
				a = 4;
				current_task(4);
			}while(time_cmp(at, t)<=0);
			a = 0;

			pthread_mutex_unlock(&mux_a_pip);
		}
		else{
			pthread_mutex_lock(&mux_a_pcp);

			clock_gettime(CLOCK_MONOTONIC, &t);
			time_add_ms(&t, 300);
			do{
				clock_gettime(CLOCK_MONOTONIC, &at);
				a = 4;
				current_task(4);
			}while(time_cmp(at, t)<=0);
			a = 0;

			pthread_mutex_unlock(&mux_a_pcp);
		}
		wait_for_period(tp);
	}
}

//--------------------------------------------------------------------------
//WORKLOAD TASK
//--------------------------------------------------------------------------

void * workload_task(void * arg)
{
struct	task_par	*tp;
int		cpu;

		tp= (struct task_par*)arg;
		set_period(tp);

		while(1){
			if((!use)&(free_ms<time_scale[pox_ts]))
				free_ms++;
			use = false;
			cpu = sched_getcpu();
			if(cpu!=0)
				printf("ERRORE CPU ");
			wait_for_period(tp);
		}
}
