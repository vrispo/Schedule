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

#define WINDOW_H			820
#define WINDOW_W			900

#define GRAFIC_H			150
#define GRAFIC_W			800
#define ORIGIN_GRAFIC_X		15
#define SPACE				20
#define INSTR_H				110
#define INSTR_W				800
#define INSTR_L				100
#define INSTR_X				5
#define INSTR_Y				10

#define	UNIT				10
#define N_TASK				4

#define MAX_MS_SCALE		100

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

void create_grafic_task(void);
void create_task(void);
void create_task_1(void);
void create_task_2(void);
void create_task_3(void);
void create_task_4(void);
void stop_task(void);

void change_time_scale(void);

void multi_exec(int gx, int gy);
//void current_task(int tsk);

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

int pox_max_array(int a[], int dim);
int freq_tsk(int a[], int s, int e, int n);
void analysis_time(void);

//--------------------------------------------------------------------------
//GLOBAL VARIABLES
//--------------------------------------------------------------------------

int					time_scale[3] = {100, 70, 50};
int					pox_ts = 0;

char				phgrafic[2][4]= {"PIP","PCP"};
char				phgraficwl[2][13]= {"PIP workload","PCP workload"};

int					x = 0;
int					task[UNIT];
int					nu = 0;
int					r_task[MAX_MS_SCALE];

bool				use = false;
int					free_ms = 0;		//number of ms that the CPU is free
double				wl = 0;				//actual workload
double				pwl = 0;			//previous workload

bool				run = TRUE;
int					run_task;
int					stop=0;
int					free_r=0;

bool				pip = TRUE;

int					ORIGIN_Y[2];		//#0:PIP #1:PCP
int					ORIGIN_WL_Y[2];		//#0:PIP #1:PCP
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
int					a_occupation[MAX_MS_SCALE];
int					b = 0;
int					b_occupation[MAX_MS_SCALE];

pthread_mutex_t		mux_a_pip;
pthread_mutex_t 	mux_b_pip;
pthread_mutexattr_t	mattr_pip;

pthread_mutex_t		mux_a_pcp;
pthread_mutex_t 	mux_b_pcp;
pthread_mutexattr_t	mattr_pcp;

//--------------------------------------------------------------------------
//						FUNCTION DEFINITIONS
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
//MAIN
//--------------------------------------------------------------------------

int main(int argc, char * argv[])
{
struct		timespec t, at;
cpu_set_t	cpuset;
pthread_t	thread;

    thread = pthread_self();
	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);
	if (pthread_setaffinity_np(thread, sizeof(cpuset), &cpuset) != 0)
		perror("pthread_setaffinity_np");

	setup();

	while(run)
	{
		use = true;
		run_task=0;
		analysis_key();
		
		clock_gettime(CLOCK_MONOTONIC, &t);
		time_add_ms(&t, 50);
		
		control_CPU("MAIN",thread);
		do{
			clock_gettime(CLOCK_MONOTONIC, &at);
			use = true;
			run_task=0;
		}while(time_cmp(at, t)<=0);
		clock_gettime(CLOCK_MONOTONIC, &t);
		time_add_ms(&t,400);		
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);
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

	rectfill(screen, (x-5), (y-GRAFIC_H-2), (x+GRAFIC_W), (y+5), 0);

	//asse y
	line(screen, x, (y-GRAFIC_H), x, y, 7);
	line(screen, x, (y-GRAFIC_H), (x-5), (y-GRAFIC_H+5), 7);
	line(screen, x, (y-GRAFIC_H), (x+5), (y-GRAFIC_H+5), 7);
	textout_ex(screen, font, s, (x-5), (y-GRAFIC_H-10), 7, -1);
	//asse x
	line(screen, x, y, (x+GRAFIC_W), y, 7);
	line(screen, (x+GRAFIC_W-5), (y-5), (x+GRAFIC_W), y, 7);
	line(screen, (x+GRAFIC_W-5), (y+5), (x+GRAFIC_W), y, 7);
	textout_ex(screen, font, "t", (x+GRAFIC_W+5), (y+5), 7, -1);

	if(ng){
		for(i=0; i<=N_TASK; i++){
			sprintf(l,"%i",i);
			textout_ex(screen, font, l, (x-10), (y-(i*(H_TASK+10))),0, -1);
			line(screen, x,(y-(i*(H_TASK+10))),(x+GRAFIC_W),(y-(i*(H_TASK+10))),7);
		}
	}
}

//--------------------------------------------------------------------------
//WRITE THE INSTRUCTIONS
//--------------------------------------------------------------------------

void write_instruction(void)
{
char s[60];

	rectfill(screen, INSTR_X, INSTR_Y, (INSTR_X+INSTR_W), (INSTR_Y+INSTR_H), 0);

	textout_ex(screen, font, "INSTRUCTIONS", (INSTR_X+7), (INSTR_Y-5), 7, -1);
	line(screen, INSTR_X, INSTR_Y, (INSTR_X+5), INSTR_Y, 7);
	line(screen, (INSTR_X+5+INSTR_L), INSTR_Y, (INSTR_X+INSTR_W), INSTR_Y, 7);
	line(screen, INSTR_X, INSTR_Y, INSTR_X, (INSTR_Y+INSTR_H), 7);
	line(screen, INSTR_X, (INSTR_Y+INSTR_H), (INSTR_X+INSTR_W), (INSTR_Y+INSTR_H), 7);
	line(screen, (INSTR_X+INSTR_W), (INSTR_Y+INSTR_H), (INSTR_X+INSTR_W), 10, 7);

	textout_ex(screen, font, "PRESS KEY ESC TO EXIT", (INSTR_X+10), (INSTR_Y+10), 7, -1);

	sprintf(s, "UNITA' MISURA -> %i ms = ", time_scale[pox_ts]);
	textout_ex(screen, font, s, (INSTR_X+10), (INSTR_Y+20), 7, -1);
	line(screen, (INSTR_X+210), (INSTR_Y+22.5), (INSTR_X+215), (INSTR_Y+22.5), 4);
	sprintf(s, "PRESS UP ARROW TO CHANGE (%i - %i - %i ms)", time_scale[0], time_scale[1], time_scale[2]);
	textout_ex(screen, font,s , (INSTR_X+220), (INSTR_Y+20), 7, -1);

	sprintf(s, "possesso risorsa a: ");
	textout_ex(screen, font, s, (INSTR_X+10), (INSTR_Y+30), 7, -1);
	rectfill(screen, (INSTR_X+165), (INSTR_Y+32.5), (INSTR_X+170), (INSTR_Y+37.5), 1);
	sprintf(s, "possesso risorsa b: ");
	textout_ex(screen, font, s, (INSTR_X+180), (INSTR_Y+30), 7, -1);
	rectfill(screen, (INSTR_X+335), (INSTR_Y+32.5), (INSTR_X+340), (INSTR_Y+37.5), 9);
	sprintf(s, "possesso risorsa a e b: ");
	textout_ex(screen, font, s, (INSTR_X+350), (INSTR_Y+30), 7, -1);
	rectfill(screen, (INSTR_X+540), (INSTR_Y+32.5), (INSTR_X+545), (INSTR_Y+37.5), 13);

	sprintf(s, "PRESS RIGHT ARROW TO CHANGE FROM PCP TO PIP AND VICEVERSA");
	textout_ex(screen, font, s, (INSTR_X+10), (INSTR_Y+40), 7, -1);
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
	clear_to_color(screen, 0);

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
	setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_Y[0], phgrafic[0] , true);
	setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_WL_Y[0], "PIP workload", false);
	setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_Y[1], phgrafic[1], true);
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
	create_grafic_task();

	//create generic tasks
	create_task();
}

//--------------------------------------------------------------------------
//CREATE TASK
//--------------------------------------------------------------------------
void create_grafic_task(void)
{
	grafic_tp.arg=0;
	grafic_tp.period=time_scale[pox_ts];
	grafic_tp.deadline=8;
	grafic_tp.priority=99;
	grafic_tp.dmiss=0;
	

	pthread_attr_init(&grafic_attr);
	pthread_attr_setdetachstate(&grafic_attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&grafic_tid, &grafic_attr, grafic_task, &grafic_tp);
}

void create_task(void)
{
struct		timespec t,at;

	stop=0;

	if(pip){
		create_mux_pip();
	}
	else{
		create_mux_pcp();
	}

	create_task_1();
	
	clock_gettime(CLOCK_MONOTONIC, &t);
	time_add_ms(&t, 150);
	do{
		clock_gettime(CLOCK_MONOTONIC, &at);
	}while(time_cmp(at, t)<=0);

	create_task_2();
	
	clock_gettime(CLOCK_MONOTONIC, &t);
	time_add_ms(&t, 50);
	do{
		clock_gettime(CLOCK_MONOTONIC, &at);
	}while(time_cmp(at, t)<=0);

	create_task_3();
	
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
}

void create_task_2(void)
{
cpu_set_t	cpuset;

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
}

void create_task_3(void)
{
cpu_set_t	cpuset;

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
}

void create_task_4(void)
{
cpu_set_t	cpuset;

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
}

//--------------------------------------------------------------------------
//STOP TASK
//--------------------------------------------------------------------------

void stop_task(void)
{
	stop=1;
	
	do{
		
	}while(free_r<3);

	if(pip)
	{
		if(pthread_mutex_destroy(&mux_a_pip)!=0)
			perror("Error in destroy mutex a pip");
		if(pthread_mutex_destroy(&mux_b_pip)!=0)
			perror("Error in destroy mutex b pip");
	}
	else
	{
		pthread_mutex_destroy(&mux_a_pcp);
		pthread_mutex_destroy(&mux_b_pcp);		
/*		if(pthread_mutex_destroy(&mux_a_pcp)!=0)
			perror("Error in destroy mutex a pcp");
		if(pthread_mutex_destroy(&mux_b_pcp)!=0)
			perror("Error in destroy mutex b pcp");*/
	}
	
	pthread_cancel(t3_tid);
	pthread_cancel(grafic_tid);	
	
	free_r=0;
}

//--------------------------------------------------------------------------
//CHANGE TIME SCALE
//--------------------------------------------------------------------------

void change_time_scale(void)
{
int	mod, i=0;
	
	pox_ts++;
	if(pox_ts==3){
		pox_ts = 0;
	}

	pthread_cancel(grafic_tid);
	write_instruction();

	create_grafic_task();
	
	if(pip)
		mod=0;
	else
		mod=1;
	
	//ridisegno la parte dx del grafico
	rectfill(screen, ORIGIN_GRAFIC_X+(x*UNIT), (ORIGIN_Y[mod]-GRAFIC_H), (ORIGIN_GRAFIC_X+((x*UNIT)+GRAFIC_W)), (ORIGIN_Y[mod]), 0);
	//asse x
	line(screen, (ORIGIN_GRAFIC_X+(x*UNIT)), ORIGIN_Y[mod], (ORIGIN_GRAFIC_X+((x*UNIT)+(GRAFIC_W-(x*UNIT)))), ORIGIN_Y[mod], 7);
	line(screen, (ORIGIN_GRAFIC_X+((x*UNIT)+(GRAFIC_W-(x*UNIT))-5)), (ORIGIN_Y[mod]-5), (ORIGIN_GRAFIC_X+((x*UNIT)+(GRAFIC_W-(x*UNIT)))), ORIGIN_Y[mod], 7);
	line(screen, (ORIGIN_GRAFIC_X+((x*UNIT)+(GRAFIC_W-(x*UNIT))-5)), (ORIGIN_Y[mod]+5), (ORIGIN_GRAFIC_X+((x*UNIT)+(GRAFIC_W-(x*UNIT)))), ORIGIN_Y[mod], 7);
	textout_ex(screen, font, "t", (ORIGIN_GRAFIC_X+GRAFIC_W+5), (ORIGIN_Y[mod]+5), 0, -1);

	for(i=0; i<=N_TASK; i++){
		line(screen, (ORIGIN_GRAFIC_X+(x*UNIT)),(ORIGIN_Y[mod]-(i*(H_TASK+10))),(ORIGIN_GRAFIC_X+((x*UNIT)+(GRAFIC_W-(x*UNIT)))),(ORIGIN_Y[mod]-(i*(H_TASK+10))),7);
	}
	
	draw_task_parameter(mod);
}

//--------------------------------------------------------------------------
//CREATE A PIP MUTEX
//--------------------------------------------------------------------------

void create_mux_pip(void)
{
	pthread_mutexattr_init(&mattr_pip);
	pthread_mutexattr_setprotocol(&mattr_pip, PTHREAD_PRIO_INHERIT);
	if(pthread_mutex_init(&mux_a_pip, &mattr_pip)!=0)
		perror("Error in create mux a pip");
	if(pthread_mutex_init(&mux_b_pip, &mattr_pip)!=0)
		perror("Error in create mux b pip");
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
	if(pthread_mutex_init(&mux_a_pcp, &mattr_pcp)!=0)
		perror("Error in create mux a pcp");
	pthread_mutexattr_setprioceiling(&mattr_pcp, max_prio_b);
	if(pthread_mutex_init(&mux_b_pcp, &mattr_pcp)!=0)
		perror("Error in create mux b pcp");
	pthread_mutexattr_destroy(&mattr_pip);
}

//--------------------------------------------------------------------------
//DRAW PARAMETER TASK IN GRAFIC PIP OR PCP
//--------------------------------------------------------------------------

void draw_task_parameter(int mod)
{
	draw_deadline(t1_tp, 1, mod);
	draw_deadline(t2_tp, 2, mod);
	draw_deadline(t3_tp, 3, mod);
	draw_deadline(t4_tp, 4, mod);
	draw_activation(t1_tp, 1, mod);
	draw_activation(t2_tp, 2, mod);
	draw_activation(t3_tp, 3, mod);
	draw_activation(t4_tp, 4, mod);
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
int		mod, i;

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
			{	
				if(stop){
					if(pip)
						mod=0;
					else
						mod=1;
					setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_Y[mod], phgrafic[mod], true);
					setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_WL_Y[mod], phgraficwl[mod], false);
					pwl = 0;
					wl = 0;
					free_ms = 0;
					x=0;
					a=0;
					b=0;
					nu=0;
					for(i=0;i<UNIT;i++)
						task[i]=7;
					for(i=0;i<MAX_MS_SCALE;i++)
						r_task[i]=7;
					for(i=0; i<MAX_MS_SCALE; i++){
						a_occupation[i] = 0;
						b_occupation[i] = 0;
					}
					create_grafic_task();
					create_task();
				}
				else
					stop_task();
				break;
			}
			case KEY_RIGHT:
			{
				int i;
				if(pip)
					mod=1;
				else
					mod=0;
				if(!stop)
					stop_task();
				setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_Y[mod], phgrafic[mod], true);
				setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_WL_Y[mod], phgraficwl[mod], false);
				pwl = 0;
				wl = 0;
				free_ms = 0;
				x=0;
				a=0;
				b=0;
				nu=0;
				for(i=0;i<UNIT;i++)
					task[i]=7;
				for(i=0;i<MAX_MS_SCALE;i++)
					r_task[i]=7;
				for(i=0; i<MAX_MS_SCALE; i++){
					a_occupation[i] = 0;
					b_occupation[i] = 0;
				}
				pip=!pip;
				create_grafic_task();
				create_task();
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
	
	xd=((x+(xd/time_scale[pox_ts]))*UNIT);
	
	for(j=0; xd<GRAFIC_W; j++){
		if(xd>=0)
			line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_Y[mod]-(i*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_Y[mod]-(i*(H_TASK+10))-H_TASK),12);
		xd=((x+((nat+tp.deadline+(j*tp.period))/time_scale[pox_ts]))*UNIT);
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

	xd=0;
	time_sub_ms(tp.at, at, &nat);
	nat=nat-tp.period;
	xd=(int)((x+(nat/time_scale[pox_ts]))*UNIT);
	for(j=0; xd<GRAFIC_W; j++){
		if(xd>=0)
			line(screen, (ORIGIN_GRAFIC_X+xd),(ORIGIN_Y[mod]-(i*(H_TASK+10))),(ORIGIN_GRAFIC_X+xd),(ORIGIN_Y[mod]-(i*(H_TASK+10))-H_TASK),14);
		xd=(int)((x+((nat+(j*tp.period))/time_scale[pox_ts]))*UNIT);
	}
}

//--------------------------------------------------------------------------
//HANDLING MULTI EXEC TASK
//--------------------------------------------------------------------------

void multi_exec(int gx, int gy)
{
int	i;	
	
	analysis_time();
	
	for(i=0; i<UNIT; i++)
		if(task[i]!=7)
			line(screen, (gx+(x*UNIT)+i), (gy-(task[i]*(H_TASK+10))),(gx+(x*UNIT)+i), (gy-(H_TASK/2)-(task[i]*(H_TASK+10))), 10);
}

//--------------------------------------------------------------------------
//DRAW RESOURCE OWNER
//--------------------------------------------------------------------------

void draw_resource(int gx, int gy)
{
int col = 10;
int i, n, ax, bx;

	n=time_scale[pox_ts]/UNIT;
	
	for(i=0; i<UNIT; i++)
	{
		ax=freq_tsk(a_occupation, (n*i), ((n*i)+n), 6);
		bx=freq_tsk(b_occupation, (n*i), ((n*i)+n), 6);
		
		if((ax!=0)&(bx==ax))
		{
			col = 13;
			line(screen, (gx+(x*UNIT)+i), (gy-(ax*(H_TASK+10))), (gx+(x*UNIT)+i), (gy-(H_TASK/4)-(ax*(H_TASK+10))), col);
		}
		
		if((ax!=0)&(bx!=a))
		{
			col = 1;
			line(screen, (gx+(x*UNIT)+i), (gy-(ax*(H_TASK+10))), (gx+(x*UNIT)+i), (gy-(H_TASK/4)-(ax*(H_TASK+10))), col);
		}
		
		if((bx!=0)&(bx!=ax))
		{
			col = 9;
			line(screen, (gx+(x*UNIT)+i), (gy-(bx*(H_TASK+10))), (gx+(x*UNIT)+i), (gy-(H_TASK/4)-(bx*(H_TASK+10))), col);
		}		
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
				multi_exec(ORIGIN_GRAFIC_X, ORIGIN_Y[mod]);

			draw_resource(ORIGIN_GRAFIC_X, ORIGIN_Y[mod]);

			if(x>0)
				line(screen, (ORIGIN_GRAFIC_X+((x-1)*UNIT)), (ORIGIN_WL_Y[mod]-(GRAFIC_H*pwl)), (ORIGIN_GRAFIC_X+(x*UNIT)), (ORIGIN_WL_Y[mod]-(GRAFIC_H*wl)), 4);

			run_task = 7;
			x++;
			if((x*UNIT)>=GRAFIC_W){
				x=0;					
				setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_Y[mod], phgrafic[mod], true);
				setup_grafic(ORIGIN_GRAFIC_X, ORIGIN_WL_Y[mod], phgraficwl[mod], false);
				draw_task_parameter(mod);			
			}
		}
		
		for(i=0; i<MAX_MS_SCALE; i++)
			r_task[i] = 7;
		for(i=0; i<MAX_MS_SCALE; i++){
			a_occupation[i] = 0;
			b_occupation[i] = 0;
		}		
		nu=0;
		for(i=0; i<UNIT; i++)
			task[i] = 7;
		
		wait_for_period(&grafic_tp);
	}
}

//--------------------------------------------------------------------------
//GENERIC TASK 1
//--------------------------------------------------------------------------

void * t_task_1(void * arg)
{
struct 	task_par	*tp;
struct	timespec	t;
struct	timespec	at;
int					mod;

	tp= (struct task_par*)arg;
	set_period(tp);
	
	//draw parameters
	if(pip)
		mod=0;
	else
		mod=1;
	draw_deadline(t1_tp, 1, mod);
	draw_activation(t1_tp, 1, mod);

	while(1){
		use = true;
		run_task=1;
		control_CPU("TASK 1",pthread_self());
		
		if(pip)
			pthread_mutex_lock(&mux_a_pip);
		else
			pthread_mutex_lock(&mux_a_pcp);

		a = 1;
		
		clock_gettime(CLOCK_MONOTONIC, &t);
		time_add_ms(&t, 100);
		
		if(stop==1)
		{
			if(pip)
			{
				if(pthread_mutex_unlock(&mux_a_pip)!=0)
					perror("Error unlock a pip task 1");
			}
			else
			{
				if(pthread_mutex_unlock(&mux_a_pcp)!=0)
					perror("Error unlock a pcp task 1");
			}
			free_r++;
			pthread_exit(0);
		}
		
		do{
			clock_gettime(CLOCK_MONOTONIC, &at);
			a = 1;
			use=true;
			run_task=1;
		}while(time_cmp(at, t)<=0);

		if(pip)	
			pthread_mutex_lock(&mux_b_pip);
		else
			pthread_mutex_lock(&mux_b_pcp);

		b = 1;
			
		clock_gettime(CLOCK_MONOTONIC, &t);
		time_add_ms(&t, 400);
		
		if(stop==1)
		{
			if(pip)
			{
				if(pthread_mutex_unlock(&mux_a_pip)!=0)
					perror("Error unlock a pip task 1");
			}
			else
			{
				if(pthread_mutex_unlock(&mux_a_pcp)!=0)
					perror("Error unlock a pcp task 1");
			}
			free_r++;
			pthread_exit(0);
		}
		
		do{
			clock_gettime(CLOCK_MONOTONIC, &at);
			b = 1;
			use=true;
			run_task=1;
		}while(time_cmp(at, t)<=0);

		b=0;
		
		if(pip)
			pthread_mutex_unlock(&mux_b_pip);
		else
			pthread_mutex_unlock(&mux_b_pcp);

		clock_gettime(CLOCK_MONOTONIC, &t);
		
		if(stop==1)
		{
			if(pip)
			{
				if(pthread_mutex_unlock(&mux_a_pip)!=0)
					perror("Error unlock a pip task 1");
			}
			else
			{
				if(pthread_mutex_unlock(&mux_a_pcp)!=0)
					perror("Error unlock a pcp task 1");
			}
			free_r++;
			pthread_exit(0);
		}		
		
		time_add_ms(&t, 100);
		do{
			clock_gettime(CLOCK_MONOTONIC, &at);
			use=true;
			run_task=1;
		}while(time_cmp(at, t)<=0);

		a = 0;

		if(pip)
			pthread_mutex_unlock(&mux_a_pip);
		else
			pthread_mutex_unlock(&mux_a_pcp);
		
		if(stop==1)
		{
			free_r++;
			pthread_exit(0);			
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
struct	timespec 	t;
struct	timespec 	at;
int					mod;

	tp= (struct task_par*)arg;
	set_period(tp);
	
	//draw parameters
	if(pip)
		mod=0;
	else
		mod=1;
	draw_deadline(t2_tp, 2, mod);
	draw_activation(t2_tp, 2, mod);

	while(1){
		use = true;
		run_task=2;
		
		control_CPU("TASK 2",pthread_self());
		
		if(pip)
			pthread_mutex_lock(&mux_b_pip);
		else
			pthread_mutex_lock(&mux_b_pcp);

		clock_gettime(CLOCK_MONOTONIC, &t);
		time_add_ms(&t, 300);
		
		if(stop==1)
		{
			if(pip)
			{
				if(pthread_mutex_unlock(&mux_b_pip)!=0)
					perror("Error in unlock mux b pip task 2");
			}
			else
			{	
				if(pthread_mutex_unlock(&mux_b_pcp)!=0)
					perror("Error in unlock mux b pcp task 2");
			}
			free_r++;
			pthread_exit(0);
		}
		
		do{
			clock_gettime(CLOCK_MONOTONIC, &at);
			b = 2;
			use=true;
			run_task=2;
		}while(time_cmp(at, t)<=0);

		b = 0;
		
		if(pip)
			pthread_mutex_unlock(&mux_b_pip);
		else
			pthread_mutex_unlock(&mux_b_pcp);
		
		if(stop==1)
		{
			free_r++;
			pthread_exit(0);			
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
struct	timespec 	t;
struct	timespec 	at;
int					mod;

	tp= (struct task_par*)arg;
	set_period(tp);
	
	//draw parameters
	if(pip)
		mod=0;
	else
		mod=1;
	draw_deadline(t3_tp, 3, mod);
	draw_activation(t3_tp, 3, mod);

	while(1){
		use = true;
		run_task=3;

		clock_gettime(CLOCK_MONOTONIC, &t);
		time_add_ms(&t, 300);
		do{
			clock_gettime(CLOCK_MONOTONIC, &at);
			use=true;
			run_task=3;
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
struct	timespec 	t;
struct	timespec 	at;
int					mod;

	tp= (struct task_par*)arg;
	set_period(tp);
	
	//draw parameters
	if(pip)
		mod=0;
	else
		mod=1;
	draw_deadline(t4_tp, 4, mod);
	draw_activation(t4_tp, 4, mod);

	while(1){
		use = true;
		run_task=4;
		control_CPU("TASK 4",pthread_self());

		if(pip)
			pthread_mutex_lock(&mux_a_pip);
		else
			pthread_mutex_lock(&mux_a_pcp);

		clock_gettime(CLOCK_MONOTONIC, &t);
		time_add_ms(&t, 300);
		
		if(stop==1)
		{
			if(pip)
			{	
				if(pthread_mutex_unlock(&mux_a_pip)!=0)
					perror("Error unlock mux a pip");
			}
			else
			{
				if(pthread_mutex_unlock(&mux_a_pcp)!=0)
					perror("Error unlock mux a pcp");
			}
			free_r++;
			pthread_exit(0);
		}
		
		do{
			clock_gettime(CLOCK_MONOTONIC, &at);
			a = 4;
			use=true;
			run_task=4;				
		}while(time_cmp(at, t)<=0);

		a = 0;
			
		if(pip)
			pthread_mutex_unlock(&mux_a_pip);
		else
			pthread_mutex_unlock(&mux_a_pcp);

		if(stop==1)
		{
			free_r++;
			pthread_exit(0);
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
			
			if((use==false)&(nu<time_scale[pox_ts]))
				r_task[nu]=7;
			if((use==true)&(nu<time_scale[pox_ts]))
				r_task[nu]=run_task;
			
			if(nu<time_scale[pox_ts])
			{
				a_occupation[nu] = a;
				b_occupation[nu] = b;
			}
			
			use = false;
			nu++;
			
			cpu = sched_getcpu();
			if(cpu!=0)
				printf("ERRORE CPU ");
			wait_for_period(tp);
		}
}

//--------------------------------------------------------------------------
//TIME ANALYSIS
//--------------------------------------------------------------------------

int pox_max_array(int a[], int dim)
{
int	pox,max,i;

	pox=0;
	max=a[pox];
	
	for(i=0; i<dim; i++)
		if(max<a[i])
		{
			max = a[i];
			pox = i;
		}
	
	return pox;
}

int freq_tsk(int a[], int s, int e, int n)
{
int pox, i;
int k[n];
	
	for(i=0; i<n; i++)
		k[i]=0;
	for(i=s; i<e; i++)
		k[a[i]]++;
	
	pox=pox_max_array(k,n);
	return pox;
}

void analysis_time(void)
{
int n, i;
	
	n=time_scale[pox_ts]/UNIT;
	
	for(i=0; i<UNIT; i++)
		task[i]=freq_tsk(r_task, (n*i), ((n*i)+n), 8);
}
