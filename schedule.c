#define _GNU_SOURCE

#include <features.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include <semaphore.h>
#include <allegro.h>
#include <stdbool.h>
#include "taskRT.h"
#include "timeplus.h"

//--------------------------------------------------------------------------------------------
//			GLOBAL CONSTANTS
//--------------------------------------------------------------------------------------------

#define WINDOW_H			820
#define WINDOW_W			940

#define GRAPHIC_H			150
#define GRAPHIC_W			800
#define ORIGIN_GRAPHIC_X	55
#define SPACE				20

#define INSTR_H				110
#define INSTR_W				500
#define INSTR_L				100
#define INSTR_X				5
#define INSTR_Y				10

#define BOX_H				110
#define BOX_W				390
#define BOX_L				70
#define BOX_X				535
#define BOX_COLUMNS			6
#define BOX_Y				10
#define BOX_ROWS			5

#define	UNIT				10
#define N_TASK				4

#define MAX_MS_SCALE		100

//--------------------------------------------------------------------------------------------
//			FUNCTION DECLARATIONS
//--------------------------------------------------------------------------------------------

void setup (void);
void setup_graphic(int x, int y, char s[], bool ng);
void draw_task_parameter(int mod);
void write_instruction(void);
void draw_taskset_box(void);
void reset(void);

void analysis_key(void);
void get_keycodes(char * scan, char * ascii);

void control_CPU(char *task_name, pthread_t	thread, int cpn);

void set_parameter(char *nfile);

void create_workload_task(void);
void create_main_task(void);
void create_graphic_task(void);
void create_task(void);
void create_task_1(void);
void create_task_2(void);
void create_task_3(void);
void create_task_4(void);

void change_time_scale(void);

void multi_exec(int gx, int gy);

void draw_resource(int gx, int gy);

void * graphic_task(void * arg);
void * t_task(void * arg);
void * workload_task(void * arg);
void * m_task(void * arg);

void draw_deadline(struct task_par tp, int i, int mod);
void draw_activation(struct task_par tp,int i, int mod);

void create_mux_pip(void);
void create_mux_pcp(void);

int pox_max_array(int a[], int dim);
int freq_tsk(int a[], int s, int e, int n);
void analysis_time(void);

//--------------------------------------------------------------------------------------------
//			GLOBAL VARIABLES
//--------------------------------------------------------------------------------------------

int			time_scale[3] = {100, 70, 50};
int			pox_ts = 0;

char			phgraphic[2][4]= {"PIP","PCP"};
char			phgraphicwl[2][13]= {"PIP workload","PCP workload"};

int			x = 0;
int			task[UNIT];
int			nu = 0;
int			r_task[MAX_MS_SCALE];

bool			use = false;
int			free_ms = 0;		//number of ms that the CPU is free
double			wl = 0;			//actual workload
double			pwl = 0;		//previous workload

bool			run = TRUE;
int			run_task;
int			stop=0;

int			mod = 0;

int			ORIGIN_Y[2];		//#0:PIP #1:PCP
int			ORIGIN_WL_Y[2];		//#0:PIP #1:PCP
int			H_TASK;

int			period[N_TASK];
int			deadline[N_TASK];
int			priority[N_TASK];
int			comp_time[N_TASK];
int			sect_a_time[N_TASK];
int			sect_b_time[N_TASK];
bool			nested[N_TASK];
int			d_miss[N_TASK];

pthread_t		graphic_tid;
struct task_par		graphic_tp;
pthread_attr_t		graphic_attr;
struct sched_param	graphic_par;

pthread_t		t1_tid;
struct task_par		t1_tp;
pthread_attr_t		t1_attr;
struct sched_param	t1_par;

pthread_t		t2_tid;
struct task_par		t2_tp;
pthread_attr_t		t2_attr;
struct sched_param	t2_par;

pthread_t		t3_tid;
struct task_par		t3_tp;
pthread_attr_t		t3_attr;
struct sched_param	t3_par;

pthread_t		t4_tid;
struct task_par		t4_tp;
pthread_attr_t		t4_attr;
struct sched_param	t4_par;

pthread_t		workload_tid;
struct task_par		workload_tp;
pthread_attr_t		workload_attr;
struct sched_param	workload_par;

pthread_t		m_tid;
struct task_par		m_tp;
pthread_attr_t		m_attr;
struct sched_param	m_par;

int			max_prio_a = 90;
int			max_prio_b = 70;
int			a = 0;
int			a_occupation[MAX_MS_SCALE];
int			b = 0;
int			b_occupation[MAX_MS_SCALE];

pthread_barrier_t	stop_barrier;

pthread_mutex_t		mux_a_pip;
pthread_mutex_t 	mux_b_pip;
pthread_mutexattr_t	mattr_pip;

pthread_mutex_t		mux_a_pcp;
pthread_mutex_t 	mux_b_pcp;
pthread_mutexattr_t	mattr_pcp;

//--------------------------------------------------------------------------------------------
//			FUNCTION DEFINITIONS
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
//			MAIN
//--------------------------------------------------------------------------------------------

int main(int argc, char * argv[])
{
cpu_set_t	cpuset;
pthread_t	thread;

    thread = pthread_self();
	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);
	if (pthread_setaffinity_np(thread, sizeof(cpuset), &cpuset) != 0)
		perror("pthread_setaffinity_np");

	if(argc!=2){
		printf("Error: Must pass the name of the task set file by command line\n");
		exit(-1);
	}
	set_parameter(argv[1]);
	setup();
	//create main task
	create_main_task();

	while(run);

	allegro_exit();
	return 0;
}

//--------------------------------------------------------------------------------------------
//			CONTROL IN THE TASK IS RUNNING ON CORRECT CPU
//--------------------------------------------------------------------------------------------

void control_CPU(char *task_name, pthread_t	thread, int cpn)
{
cpu_set_t	cpuget;
int		ncpu;
int		cpu;

	cpu = sched_getcpu();
	if(cpu!=cpn)
	{
		printf("Error current cpu %s is not %i \n", task_name, cpn);
		exit(-1);
	}
	pthread_getaffinity_np(thread,sizeof(cpuget),&cpuget);
	ncpu=CPU_COUNT(&cpuget);
	if(ncpu!=1){
		printf("Error more than one elegible CPU for: %s \n", task_name);
		exit(-1);
	}
	if(CPU_ISSET(cpn, &cpuget)==0){
		printf("Error cpu %i not in the elegible set of: %s \n", cpn, task_name);
		exit(-1);
	}
}

//--------------------------------------------------------------------------------------------
//			DRAW EMPTY GRAPHIC
//--------------------------------------------------------------------------------------------

void setup_graphic(int x, int y, char s[], bool ng)
{
int	i = 0;
char	l[10];

	rectfill(screen, (x-5), (y-GRAPHIC_H-2), (x+GRAPHIC_W), (y+5), 0);

	//asse y
	line(screen, x, (y-GRAPHIC_H), x, y, 7);
	line(screen, x, (y-GRAPHIC_H), (x-5), (y-GRAPHIC_H+5), 7);
	line(screen, x, (y-GRAPHIC_H), (x+5), (y-GRAPHIC_H+5), 7);
	textout_ex(screen, font, s, (x-5), (y-GRAPHIC_H-10), 7, -1);
	//asse x
	line(screen, x, y, (x+GRAPHIC_W), y, 7);
	line(screen, (x+GRAPHIC_W-5), (y-5), (x+GRAPHIC_W), y, 7);
	line(screen, (x+GRAPHIC_W-5), (y+5), (x+GRAPHIC_W), y, 7);
	textout_ex(screen, font, "t", (x+GRAPHIC_W+5), (y+5), 7, -1);

	if(ng){
		for(i=0; i<=N_TASK; i++){
			if(i==0)
				sprintf(l,"MAIN");
			else
				sprintf(l,"TASK %i",i);
			textout_ex(screen, font, l, (x-50), (y-(i*(H_TASK+10))-10), 7, -1);
			line(screen, x,(y-(i*(H_TASK+10))),(x+GRAPHIC_W),(y-(i*(H_TASK+10))),7);
		}
		textout_ex(screen, font, "#D_Miss", (ORIGIN_GRAPHIC_X+GRAPHIC_W+10), (y-(5*(H_TASK+10))+5), 7, 4);
	}
}

//--------------------------------------------------------------------------------------------
//			DRAW THE BOX OF THE TASK SET
//--------------------------------------------------------------------------------------------
void draw_taskset_box(void)
{
char	s[10];
int		lcol,lrow,i;

	lcol = BOX_W/BOX_COLUMNS;
	lrow = BOX_H/BOX_ROWS;
	rectfill(screen, BOX_X, BOX_Y, (BOX_X+BOX_W), (BOX_Y+BOX_H), 0);

	textout_ex(screen, font, "TASK SET", (BOX_X+7), (BOX_Y-5), 7, -1);
	line(screen, BOX_X, BOX_Y, (BOX_X+5), BOX_Y, 7);
	line(screen, (BOX_X+5+BOX_L), BOX_Y, (BOX_X+BOX_W), BOX_Y, 7);
	line(screen, BOX_X, BOX_Y, BOX_X, (BOX_Y+BOX_H), 7);
	line(screen, BOX_X, (BOX_Y+BOX_H), (BOX_X+BOX_W), (BOX_Y+BOX_H), 7);
	line(screen, (BOX_X+BOX_W), (BOX_Y+BOX_H), (BOX_X+BOX_W), 10, 7);
	
	for(i=1; i<BOX_COLUMNS; i++)
		line(screen, (BOX_X+(lcol*i)), (BOX_Y+10), (BOX_X+(lcol*i)), (BOX_Y+BOX_H-5), 7);
	
	textout_ex(screen, font, "T", (BOX_X+(lcol*1)+10), (BOX_Y+10), 7, -1);
	textout_ex(screen, font, "D", (BOX_X+(lcol*2)+10), (BOX_Y+10), 7, -1);
	textout_ex(screen, font, "PRIO", (BOX_X+(lcol*3)+10), (BOX_Y+10), 7, -1);
	textout_ex(screen, font, "CS A", (BOX_X+(lcol*4)+10), (BOX_Y+10), 7, -1);
	textout_ex(screen, font, "CS B", (BOX_X+(lcol*5)+10), (BOX_Y+10), 7, -1);
	
	for(i=1; i<BOX_ROWS; i++)
		line(screen, (BOX_X+10), (BOX_Y+(lrow*i)), (BOX_X+BOX_W-10), (BOX_Y+(lrow*i)), 7);
	
	//#task
	textout_ex(screen, font, "TASK 1", (BOX_X+10), (BOX_Y+(lrow*1)+10), 7, -1);
	textout_ex(screen, font, "TASK 2", (BOX_X+10), (BOX_Y+(lrow*2)+10), 7, -1);
	textout_ex(screen, font, "TASK 3", (BOX_X+10), (BOX_Y+(lrow*3)+10), 7, -1);
	textout_ex(screen, font, "TASK 4", (BOX_X+10), (BOX_Y+(lrow*4)+10), 7, -1);
	
	for(i=1; i<BOX_ROWS; i++)
	{
		//Period
		sprintf(s, "%i", period[(i-1)]);
		textout_ex(screen, font, s, (BOX_X+(lcol*1)+10), (BOX_Y+(lrow*i)+10), 7, -1);
		//Deadline
		sprintf(s, "%i", deadline[(i-1)]);
		textout_ex(screen, font, s, (BOX_X+(lcol*2)+10), (BOX_Y+(lrow*i)+10), 7, -1);
		//Priority
		sprintf(s, "%i", priority[(i-1)]);
		textout_ex(screen, font, s, (BOX_X+(lcol*3)+10), (BOX_Y+(lrow*i)+10), 7, -1);
		//CS A
		sprintf(s, "%i", sect_a_time[(i-1)]);
		textout_ex(screen, font, s, (BOX_X+(lcol*4)+10), (BOX_Y+(lrow*i)+10), 7, -1);
		//CS B
		sprintf(s, "%i", sect_b_time[(i-1)]);
		textout_ex(screen, font, s, (BOX_X+(lcol*5)+10), (BOX_Y+(lrow*i)+10), 7, -1);
	}
}
//--------------------------------------------------------------------------------------------
//			WRITE THE INSTRUCTIONS
//--------------------------------------------------------------------------------------------

void write_instruction(void)
{
char s[60];

	//do the box for the instructions
	rectfill(screen, INSTR_X, INSTR_Y, (INSTR_X+INSTR_W), (INSTR_Y+INSTR_H), 0);
	textout_ex(screen, font, "INSTRUCTIONS", (INSTR_X+7), (INSTR_Y-5), 7, -1);
	line(screen, INSTR_X, INSTR_Y, (INSTR_X+5), INSTR_Y, 7);
	line(screen, (INSTR_X+5+INSTR_L), INSTR_Y, (INSTR_X+INSTR_W), INSTR_Y, 7);
	line(screen, INSTR_X, INSTR_Y, INSTR_X, (INSTR_Y+INSTR_H), 7);
	line(screen, INSTR_X, (INSTR_Y+INSTR_H), (INSTR_X+INSTR_W), (INSTR_Y+INSTR_H), 7);
	line(screen, (INSTR_X+INSTR_W), (INSTR_Y+INSTR_H), (INSTR_X+INSTR_W), 10, 7);
	
	textout_ex(screen, font, "PRESS KEY ESC TO EXIT", (INSTR_X+10), (INSTR_Y+10), 7, -1);
	textout_ex(screen, font, "PRESS RIGHT ARROW TO CHANGE FROM PCP TO PIP AND VICEVERSA", (INSTR_X+10), (INSTR_Y+20), 7, -1);
	sprintf(s, "PRESS UP ARROW TO CHANGE (%i - %i - %i ms)", time_scale[0], time_scale[1], time_scale[2]);
	textout_ex(screen, font,s , (INSTR_X+10), (INSTR_Y+30), 7, -1);

	textout_ex(screen, font, "LEGEND", (INSTR_X+10), (INSTR_Y+50), 7, -1);
	line(screen, INSTR_X, INSTR_Y+55, (INSTR_X+5), INSTR_Y+55, 7);
	line(screen, (INSTR_X+5+INSTR_L-45), (INSTR_Y+55), (INSTR_X+INSTR_W), (INSTR_Y+55), 7);
	sprintf(s, "Actual unit of time -> %i ms = ", time_scale[pox_ts]);
	textout_ex(screen, font, s, (INSTR_X+10), (INSTR_Y+65), 7, -1);
	line(screen, (INSTR_X+270), (INSTR_Y+70), (INSTR_X+270+UNIT), (INSTR_Y+70), 4);
	line(screen, (INSTR_X+270), (INSTR_Y+70), (INSTR_X+270), (INSTR_Y+67), 4);
	line(screen, (INSTR_X+270+UNIT), (INSTR_Y+70), (INSTR_X+270+UNIT), (INSTR_Y+67), 4);
	textout_ex(screen, font, "in Critical Section A: ", (INSTR_X+10), (INSTR_Y+75), 7, -1);
	rectfill(screen, (INSTR_X+200), (INSTR_Y+77.5), (INSTR_X+205), (INSTR_Y+82.5), 15);
	textout_ex(screen, font, "in Critical Section B: ", (INSTR_X+10), (INSTR_Y+85), 7, -1);
	rectfill(screen, (INSTR_X+200), (INSTR_Y+87.5), (INSTR_X+205), (INSTR_Y+92.5), 9);
	textout_ex(screen, font, "in Critical Sections A and B: ", (INSTR_X+10), (INSTR_Y+95), 7, -1);
	rectfill(screen, (INSTR_X+250), (INSTR_Y+97.5), (INSTR_X+255), (INSTR_Y+102.5), 13);
}

//--------------------------------------------------------------------------------------------
//			SETUP
//--------------------------------------------------------------------------------------------

void reset(void)
{
int i;

	for(i=0; i<MAX_MS_SCALE; i++)
	{
		r_task[i] = 7;
		a_occupation[i] = 0;
		b_occupation[i] = 0;
	}
	nu = 0;
	for(i=0; i<UNIT; i++)
		task[i] = 7;
}

//--------------------------------------------------------------------------------------------
//			SETUP
//--------------------------------------------------------------------------------------------

void set_parameter(char* nfile)
{
FILE	*ts;
char	riga[100];
char	*tok;
int		n, i = 0;

	ts=fopen(nfile, "r");
	if(ts==NULL){
		perror("Error in file open");
		exit(-1);
	}
	
	fgets(riga,99,ts);
	do
	{
		if(i>=N_TASK)
		{
			printf("Error:too much row. The number of task must be 4.\n");
			exit(-1);
		}

		tok=strtok(riga, " ");
		period[i]=atoi(tok);
		tok=strtok(NULL, " ");
		deadline[i]=atoi(tok);
		tok=strtok(NULL, " ");
		priority[i]=atoi(tok);
		tok=strtok(NULL, " ");
		comp_time[i]=atoi(tok);
		tok=strtok(NULL, " ");
		sect_a_time[i]=atoi(tok);
		tok=strtok(NULL, " ");
		sect_b_time[i]=atoi(tok);
		tok=strtok(NULL, " ");
		n=atoi(tok);
		if(n==1)
		{
			if((sect_a_time[i]==0)|(sect_b_time[i]==0))
			{
				printf("Error in taskset.txt nested can not be 1 if the use of both the resource is not different from 0 \n");
				exit(-1);
			}
			if(sect_a_time[i]<sect_b_time[i])
			{
				printf("Error in taskset.txt in nested mode the use of the second resource must be less than the first because his critical section is nested in the other\n");
				exit(-1);
			}
			if(sect_a_time[i]>comp_time[i])
			{
				printf("Error in taskset.txt in every case the total computation time must be greater or equal than the two critical sections\n");
				exit(-1);
			}
			nested[i]=true;
		}
		else
		{
			nested[i]=false;	
			if((sect_a_time[i]+sect_b_time[i])>comp_time[i])
			{
				printf("Error in taskset.txt in every case the total computation time must be greater or equal than the two critical sections\n");
				exit(-1);
			}
		}
		
		i++;
	}while(fgets(riga,99,ts)!=NULL);
	
	if(i<N_TASK)
	{
		printf("Error:too few row. The number of task must be 4.\n");
		exit(-1);
	}

	fclose(ts);
}

void setup(void)
{
cpu_set_t	cpuset;
//FILE		*f_sched_budget;

	allegro_init();
	install_keyboard();

	set_color_depth(8);
	set_gfx_mode(GFX_AUTODETECT_WINDOWED, WINDOW_W, WINDOW_H, 0, 0);
	clear_to_color(screen, 0);

	run_task=0;
	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);
/*	f_sched_budget=fopen("/proc/sys/kernel/sched_rt_runtime_us","w");
	if(f_sched_budget==NULL){
		perror("Error in file open /proc/sys/kernel/sched_rt_runtime_us you must run the program as super user ");
		exit(-1);
	}
	fprintf(f_sched_budget,"1000000");
	fclose(f_sched_budget);*/


	write_instruction();
	draw_taskset_box();

	//graphici
	H_TASK=(GRAPHIC_H-(N_TASK*10))/(N_TASK+1);
	ORIGIN_Y[0]=INSTR_Y+INSTR_H+SPACE+GRAPHIC_H;
	ORIGIN_WL_Y[0]=ORIGIN_Y[0]+SPACE+GRAPHIC_H;
	ORIGIN_Y[1]=ORIGIN_WL_Y[0]+SPACE+GRAPHIC_H;
	ORIGIN_WL_Y[1]=ORIGIN_Y[1]+SPACE+GRAPHIC_H;
	setup_graphic(ORIGIN_GRAPHIC_X, ORIGIN_Y[0], phgraphic[0] , true);
	setup_graphic(ORIGIN_GRAPHIC_X, ORIGIN_WL_Y[0], "PIP workload", false);
	setup_graphic(ORIGIN_GRAPHIC_X, ORIGIN_Y[1], phgraphic[1], true);
	setup_graphic(ORIGIN_GRAPHIC_X, ORIGIN_WL_Y[1], "PCP workload", false);
	
	//create mutexs and barrier
	pthread_barrier_init(&stop_barrier, NULL, 5);	
	create_mux_pip();
	create_mux_pcp();
	
	//create workload task
	create_workload_task();
	
	//create graphic task
	create_graphic_task();

	//create generic tasks
	create_task();
}

//--------------------------------------------------------------------------------------------
//			CREATE TASK
//--------------------------------------------------------------------------------------------

void create_main_task(void)
{
cpu_set_t	cpuset;

	CPU_ZERO(&cpuset);
	CPU_SET(0,&cpuset);
	
	m_tp.arg=0;
	m_tp.period=500;
	m_tp.deadline=200;
	m_tp.priority=99;
	m_tp.dmiss=0;
	
	pthread_attr_init(&m_attr);
	pthread_attr_setdetachstate(&m_attr, PTHREAD_CREATE_DETACHED);	
	pthread_attr_setaffinity_np(&m_attr, sizeof(cpuset), &cpuset);
	
	pthread_attr_setinheritsched(&m_attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&m_attr, SCHED_FIFO);
	m_par.sched_priority=m_tp.priority;
	pthread_attr_setschedparam(&m_attr, &m_par);
	
	if(pthread_create(&m_tid, &m_attr, m_task, &m_tp)!=0)
	{
		perror("Error in pthread_create main task ");
		exit(-1);
	}		
}

void create_workload_task(void)
{
cpu_set_t	cpuset;

	CPU_ZERO(&cpuset);
	CPU_SET(1,&cpuset);
	
	workload_tp.arg=0;
	workload_tp.period=1;
	workload_tp.deadline=1;
	workload_tp.priority=1;
	workload_tp.dmiss=0;

	pthread_attr_init(&workload_attr);
	pthread_attr_setdetachstate(&workload_attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setaffinity_np(&workload_attr, sizeof(cpuset), &cpuset);
	pthread_attr_setinheritsched(&workload_attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&workload_attr, SCHED_FIFO);
	workload_par.sched_priority=workload_tp.priority;
	pthread_attr_setschedparam(&workload_attr, &workload_par);
	pthread_create(&workload_tid, &workload_attr, workload_task, &workload_tp);
}

void create_graphic_task(void)
{
cpu_set_t	cpuset;

	CPU_ZERO(&cpuset);
	CPU_SET(1,&cpuset);
	
	graphic_tp.arg=0;
	graphic_tp.period=time_scale[pox_ts];
	graphic_tp.deadline=8;
	graphic_tp.priority=99;
	graphic_tp.dmiss=0;
	
	pthread_attr_init(&graphic_attr);
	pthread_attr_setdetachstate(&graphic_attr, PTHREAD_CREATE_DETACHED);	
	pthread_attr_setaffinity_np(&graphic_attr, sizeof(cpuset), &cpuset);
	
	pthread_attr_setinheritsched(&graphic_attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&graphic_attr, SCHED_FIFO);
	graphic_par.sched_priority=graphic_tp.priority;
	pthread_attr_setschedparam(&graphic_attr, &graphic_par);
	
	if(pthread_create(&graphic_tid, &graphic_attr, graphic_task, &graphic_tp)!=0)
	{
		perror("Error in pthread_create graphic task ");
		exit(-1);
	}		
}

void create_task(void)
{
struct		timespec t,at;
int			i;

	for(i=0; i<N_TASK; i++)
		d_miss[i]=0;

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

	t1_tp.arg=1;
	t1_tp.period=period[0];
	t1_tp.deadline=deadline[0];
	t1_tp.priority=priority[0];
	t1_tp.dmiss=0;
	pthread_attr_init(&t1_attr);
	pthread_attr_setdetachstate(&t1_attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setaffinity_np(&t1_attr, sizeof(cpuset), &cpuset);
	pthread_attr_setinheritsched(&t1_attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&t1_attr, SCHED_FIFO);
	t1_par.sched_priority=t1_tp.priority;
	pthread_attr_setschedparam(&t1_attr, &t1_par);
	if(pthread_create(&t1_tid, &t1_attr, t_task, &t1_tp)!=0)
	{
		perror("Error in pthread_create task 1 ");
		exit(-1);
	}
}

void create_task_2(void)
{
cpu_set_t	cpuset;

	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);

	t2_tp.arg=2;
	t2_tp.period=period[1];
	t2_tp.deadline=deadline[1];
	t2_tp.priority=priority[1];
	t2_tp.dmiss=0;
	pthread_attr_init(&t2_attr);
	pthread_attr_setdetachstate(&t2_attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setaffinity_np(&t2_attr, sizeof(cpuset), &cpuset);
	pthread_attr_setinheritsched(&t2_attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&t2_attr, SCHED_FIFO);
	t2_par.sched_priority=t2_tp.priority;
	pthread_attr_setschedparam(&t2_attr, &t2_par);
	if(pthread_create(&t2_tid, &t2_attr, t_task, &t2_tp)!=0)
	{
		perror("Error in pthread_create task 2 ");
		exit(-1);
	}
}

void create_task_3(void)
{
cpu_set_t	cpuset;

	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);

	t3_tp.arg=3;
	t3_tp.period=period[2];
	t3_tp.deadline=deadline[2];
	t3_tp.priority=priority[2];
	t3_tp.dmiss=0;
	pthread_attr_init(&t3_attr);
	pthread_attr_setdetachstate(&t3_attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setaffinity_np(&t3_attr, sizeof(cpuset), &cpuset);
	pthread_attr_setinheritsched(&t3_attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&t3_attr, SCHED_FIFO);
	t3_par.sched_priority=t3_tp.priority;
	pthread_attr_setschedparam(&t3_attr, &t3_par);
	if(pthread_create(&t3_tid, &t3_attr, t_task, &t3_tp)!=0)
	{
		perror("Error in pthread_create task 3 ");
		exit(-1);
	}
}

void create_task_4(void)
{
cpu_set_t	cpuset;

	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);

	t4_tp.arg=4;
	t4_tp.period=period[3];
	t4_tp.deadline=deadline[3];
	t4_tp.priority=priority[3];
	t4_tp.dmiss=0;
	pthread_attr_init(&t4_attr);
	pthread_attr_setdetachstate(&t4_attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setaffinity_np(&t4_attr, sizeof(cpuset), &cpuset);
	pthread_attr_setinheritsched(&t4_attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&t4_attr, SCHED_FIFO);
	t4_par.sched_priority=t4_tp.priority;
	pthread_attr_setschedparam(&t4_attr, &t4_par);
	if(pthread_create(&t4_tid, &t4_attr, t_task, &t4_tp)!=0)
	{
		perror("Error in pthread_create task 4 ");
		exit(-1);
	}
}

//--------------------------------------------------------------------------------------------
//			CHANGE TIME SCALE
//--------------------------------------------------------------------------------------------

void change_time_scale(void)
{
int i=0;
	
	pox_ts++;
	if(pox_ts==3){
		pox_ts = 0;
	}

	pthread_cancel(graphic_tid);
	write_instruction();

	create_graphic_task();
	
	//redraw part of the graph
	rectfill(screen, ORIGIN_GRAPHIC_X+(x*UNIT), (ORIGIN_Y[mod]-GRAPHIC_H), (ORIGIN_GRAPHIC_X+((x*UNIT)+GRAPHIC_W)), (ORIGIN_Y[mod]), 0);
	//axes x
	line(screen, (ORIGIN_GRAPHIC_X+(x*UNIT)), ORIGIN_Y[mod], (ORIGIN_GRAPHIC_X+((x*UNIT)+(GRAPHIC_W-(x*UNIT)))), ORIGIN_Y[mod], 7);
	line(screen, (ORIGIN_GRAPHIC_X+((x*UNIT)+(GRAPHIC_W-(x*UNIT))-5)), (ORIGIN_Y[mod]-5), (ORIGIN_GRAPHIC_X+((x*UNIT)+(GRAPHIC_W-(x*UNIT)))), ORIGIN_Y[mod], 7);
	line(screen, (ORIGIN_GRAPHIC_X+((x*UNIT)+(GRAPHIC_W-(x*UNIT))-5)), (ORIGIN_Y[mod]+5), (ORIGIN_GRAPHIC_X+((x*UNIT)+(GRAPHIC_W-(x*UNIT)))), ORIGIN_Y[mod], 7);
	textout_ex(screen, font, "t", (ORIGIN_GRAPHIC_X+GRAPHIC_W+5), (ORIGIN_Y[mod]+5), 0, -1);

	for(i=0; i<=N_TASK; i++){
		line(screen, (ORIGIN_GRAPHIC_X+(x*UNIT)),(ORIGIN_Y[mod]-(i*(H_TASK+10))),(ORIGIN_GRAPHIC_X+((x*UNIT)+(GRAPHIC_W-(x*UNIT)))),(ORIGIN_Y[mod]-(i*(H_TASK+10))),7);
	}
	
	draw_task_parameter(mod);
}

//--------------------------------------------------------------------------------------------
//			CREATE A PIP MUTEX
//--------------------------------------------------------------------------------------------

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

//--------------------------------------------------------------------------------------------
//			CREATE A PCP MUTEX
//--------------------------------------------------------------------------------------------

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
	pthread_mutexattr_destroy(&mattr_pcp);
}

//--------------------------------------------------------------------------------------------
//			DRAW PARAMETER TASK IN GRAPHIC PIP OR PCP
//--------------------------------------------------------------------------------------------

void draw_task_parameter(int mod)
{
	draw_deadline(m_tp, 0, mod);
	draw_deadline(t1_tp, 1, mod);
	draw_deadline(t2_tp, 2, mod);
	draw_deadline(t3_tp, 3, mod);
	draw_deadline(t4_tp, 4, mod);
	draw_activation(m_tp, 0, mod);
	draw_activation(t1_tp, 1, mod);
	draw_activation(t2_tp, 2, mod);
	draw_activation(t3_tp, 3, mod);
	draw_activation(t4_tp, 4, mod);
}

//--------------------------------------------------------------------------------------------
//			GET THE SCAN CODE AND THE ASCII CODE FROM A READ KEY
//--------------------------------------------------------------------------------------------

void get_keycodes(char * scan, char * ascii)
{
int k;

	k=readkey();
	*ascii=k;
	*scan=k>>8;
}

//--------------------------------------------------------------------------------------------
//			ANALYSIS KEYBOARD
//--------------------------------------------------------------------------------------------

void analysis_key(void)
{
char	scan, ascii;
bool	keyp=FALSE;
int		rc;

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
				if(stop)
				{
					setup_graphic(ORIGIN_GRAPHIC_X, ORIGIN_Y[mod], phgraphic[mod], true);
					setup_graphic(ORIGIN_GRAPHIC_X, ORIGIN_WL_Y[mod], phgraphicwl[mod], false);
					pwl = 0;
					wl = 0;
					free_ms = 0;
					x=0;
					a=0;
					b=0;
					reset();
					stop=!stop;
					create_task();			
				}
				else
				{
					stop=!stop;
					rc = pthread_barrier_wait(&stop_barrier);
					if((rc !=0) && (rc != PTHREAD_BARRIER_SERIAL_THREAD))
					{
						perror("Could not wait on barrier");
						exit(-1);
					}
				}				
				break;
			}
			case KEY_RIGHT:
			{
				if(!stop)
				{
					stop=!stop;
					if(mod==0)
						mod=1;
					else
						mod=0;
					rc = pthread_barrier_wait(&stop_barrier);
					if((rc !=0) && (rc != PTHREAD_BARRIER_SERIAL_THREAD))
					{
						perror("Could not wait on barrier");
						exit(-1);
					}
				}
				else
				{
					if(mod==0)
						mod=1;
					else
						mod=0;
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

//--------------------------------------------------------------------------------------------
//			DRAW DEADLINE OF A TASK IN PIP(MOD #0) OR PCP(MOD #1) GRAPHIC
//--------------------------------------------------------------------------------------------

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
	
	for(j=0; xd<GRAPHIC_W; j++){
		if(xd>=0)
			line(screen, (ORIGIN_GRAPHIC_X+xd), (ORIGIN_Y[mod]-(i*(H_TASK+10))), (ORIGIN_GRAPHIC_X+xd), (ORIGIN_Y[mod]-(i*(H_TASK+10))-H_TASK), 12);
		xd=((x+((nat+tp.deadline+(j*tp.period))/time_scale[pox_ts]))*UNIT);
	}
}

//--------------------------------------------------------------------------------------------
//			DRAW ACTIVATION OF A TASK IN PIP(MOD #0) OR PCP(MOD #1) GRAPHIC
//--------------------------------------------------------------------------------------------

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
	for(j=0; xd<GRAPHIC_W; j++){
		if(xd>=0)
			line(screen, (ORIGIN_GRAPHIC_X+xd), (ORIGIN_Y[mod]-(i*(H_TASK+10))), (ORIGIN_GRAPHIC_X+xd), (ORIGIN_Y[mod]-(i*(H_TASK+10))-H_TASK), 14);
		xd=(int)((x+((nat+(j*tp.period))/time_scale[pox_ts]))*UNIT);
	}
}

//--------------------------------------------------------------------------------------------
//			HANDLING MULTI EXEC TASK
//--------------------------------------------------------------------------------------------

void multi_exec(int gx, int gy)
{
int	i;	
	
	analysis_time();
	
	for(i=0; i<UNIT; i++)
		if(task[i]!=7)
			line(screen, (gx+(x*UNIT)+i), (gy-(task[i]*(H_TASK+10))),(gx+(x*UNIT)+i), (gy-(H_TASK/2)-(task[i]*(H_TASK+10))), 10);
}

//--------------------------------------------------------------------------------------------
//			DRAW RESOURCE OWNER
//--------------------------------------------------------------------------------------------

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
		
		if((ax!=0)&(bx!=ax))
		{
			col = 15;
			line(screen, (gx+(x*UNIT)+i), (gy-(ax*(H_TASK+10))), (gx+(x*UNIT)+i), (gy-(H_TASK/4)-(ax*(H_TASK+10))), col);
		}
		
		if((bx!=0)&(bx!=ax))
		{
			col = 9;
			line(screen, (gx+(x*UNIT)+i), (gy-(bx*(H_TASK+10))), (gx+(x*UNIT)+i), (gy-(H_TASK/4)-(bx*(H_TASK+10))), col);
		}		
	}
}

//--------------------------------------------------------------------------------------------
//			GRAPHIC TASK
//--------------------------------------------------------------------------------------------

void * graphic_task(void * arg)
{
struct timespec	at;
int				i=0;
char			l[2];

	set_period(&graphic_tp);	

	while(1){
		
		control_CPU("Graphic task", pthread_self(),1);
		if(!stop){
			//calcola workload e fai graphico
			pwl = wl;
			wl = 1 - ((double)free_ms/(double)time_scale[pox_ts]);
			free_ms = 0;
				
			clock_gettime(CLOCK_MONOTONIC, &at);

			if(run_task!=7)
				multi_exec(ORIGIN_GRAPHIC_X, ORIGIN_Y[mod]);

			draw_resource(ORIGIN_GRAPHIC_X, ORIGIN_Y[mod]);

			if(x>0)
				line(screen, (ORIGIN_GRAPHIC_X+((x-1)*UNIT)), (ORIGIN_WL_Y[mod]-(GRAPHIC_H*pwl)), (ORIGIN_GRAPHIC_X+(x*UNIT)), (ORIGIN_WL_Y[mod]-(GRAPHIC_H*wl)), 4);

			for(i=1; i<=N_TASK; i++){
				sprintf(l," %i ", d_miss[(i-1)]);
				textout_ex(screen, font, l, (ORIGIN_GRAPHIC_X+GRAPHIC_W+SPACE), (ORIGIN_Y[mod]-(i*(H_TASK+10))-10), 7, 4);
			}
			sprintf(l," %i ", m_tp.dmiss);
			textout_ex(screen, font, l, (ORIGIN_GRAPHIC_X+GRAPHIC_W+SPACE), (ORIGIN_Y[mod]-10), 7, 4);
			
			run_task = 7;
			x++;
			if((x*UNIT)>=GRAPHIC_W){
				x=0;					
				setup_graphic(ORIGIN_GRAPHIC_X, ORIGIN_Y[mod], phgraphic[mod], true);
				setup_graphic(ORIGIN_GRAPHIC_X, ORIGIN_WL_Y[mod], phgraphicwl[mod], false);
				draw_task_parameter(mod);			
			}
		}
		
		reset();
		
		wait_for_period(&graphic_tp);
	}
}

//--------------------------------------------------------------------------------------------
//			GENERIC TASK
//--------------------------------------------------------------------------------------------

void * t_task(void * arg) 
{
	struct 	task_par	*tp;
	struct	timespec	t;
	struct	timespec	at;
	int					rc;
	int					c=0,ca=0, cb=0;
	int					argument;
	char				task_name[10];

		tp= (struct task_par*)arg;
		set_period(tp);
		argument=(*tp).arg;
		
		ca=sect_a_time[(argument-1)];
		cb=sect_b_time[(argument-1)];
		
		//draw parameters
		draw_deadline(*tp, argument, mod);
		draw_activation(*tp, argument, mod);
		
		while(1){
			use = true;
			run_task=argument;
			control_CPU(task_name,pthread_self(),0);	
			
			if(nested[(argument-1)])
			{
				c=comp_time[(argument-1)]-ca;
				clock_gettime(CLOCK_MONOTONIC, &t);
				time_add_ms(&t, (c/2));
				do{
					clock_gettime(CLOCK_MONOTONIC, &at);
					use = true;
					run_task = argument;
				}while(time_cmp(at, t)<=0);
				
				if(mod==0)
					pthread_mutex_lock(&mux_a_pip);
				else
					pthread_mutex_lock(&mux_a_pcp);
				a=argument;

				clock_gettime(CLOCK_MONOTONIC, &t);
				time_add_ms(&t, ((ca-cb)/2));
				do{
					clock_gettime(CLOCK_MONOTONIC, &at);
					a = argument;
					use=true;
					run_task=argument;
				}while(time_cmp(at, t)<=0);
				
				if(mod==0)
					pthread_mutex_lock(&mux_b_pip);
				else
					pthread_mutex_lock(&mux_b_pcp);
				b=argument;
				
				clock_gettime(CLOCK_MONOTONIC, &t);
				time_add_ms(&t, cb);
				do{
					clock_gettime(CLOCK_MONOTONIC, &at);
					b=argument;
					a = argument;
					use=true;
					run_task=argument;
				}while(time_cmp(at, t)<=0);				
				
				b=0;				
				if(mod==0)
					pthread_mutex_unlock(&mux_b_pip);
				else
					pthread_mutex_unlock(&mux_b_pcp);
				
				clock_gettime(CLOCK_MONOTONIC, &t);
				time_add_ms(&t, ((ca-cb)/2));
				do{
					clock_gettime(CLOCK_MONOTONIC, &at);
					a = argument;
					use=true;
					run_task=argument;
				}while(time_cmp(at, t)<=0);
				
				a=0;				
				if(mod==0)
					pthread_mutex_unlock(&mux_a_pip);
				else
					pthread_mutex_unlock(&mux_a_pcp);	
				
				clock_gettime(CLOCK_MONOTONIC, &t);
				time_add_ms(&t, (c/2));
				do{
					clock_gettime(CLOCK_MONOTONIC, &at);
					use = true;
					run_task = argument;
				}while(time_cmp(at, t)<=0);
			}
			else
			{
				c=comp_time[(argument-1)]-ca-cb;
				clock_gettime(CLOCK_MONOTONIC, &t);
				time_add_ms(&t, (c/2));
				do{
					clock_gettime(CLOCK_MONOTONIC, &at);
					use = true;
					run_task = argument;
				}while(time_cmp(at, t)<=0);
				
				if(ca!=0)
				{
					if(mod==0)
						pthread_mutex_lock(&mux_a_pip);
					else
						pthread_mutex_lock(&mux_a_pcp);
					a = argument;
					
					clock_gettime(CLOCK_MONOTONIC, &t);
					time_add_ms(&t, sect_a_time[(argument-1)]);
					do{
						clock_gettime(CLOCK_MONOTONIC, &at);
						a = argument;
						use = true;
						run_task = argument;
					}while(time_cmp(at, t)<=0);
					
					a = 0;
					if(mod==0)
						pthread_mutex_unlock(&mux_a_pip);
					else
						pthread_mutex_unlock(&mux_a_pcp);	
				}
				
				if(cb!=0)
				{
					if(mod==0)
						pthread_mutex_lock(&mux_b_pip);
					else
						pthread_mutex_lock(&mux_b_pcp);
					b = argument;
					
					clock_gettime(CLOCK_MONOTONIC, &t);
					time_add_ms(&t, sect_b_time[(argument-1)]);
					do{
						clock_gettime(CLOCK_MONOTONIC, &at);
						b = argument;
						use = true;
						run_task = argument;
					}while(time_cmp(at, t)<=0);
					
					b = 0;
					if(mod==0)
						pthread_mutex_unlock(&mux_b_pip);
					else
						pthread_mutex_unlock(&mux_b_pcp);					
				}
				
				clock_gettime(CLOCK_MONOTONIC, &t);
				time_add_ms(&t, (c/2));
				do{
					clock_gettime(CLOCK_MONOTONIC, &at);
					use = true;
					run_task = argument;
				}while(time_cmp(at, t)<=0);
			}		
			
			if(stop==1)
			{
				rc = pthread_barrier_wait(&stop_barrier);
				if((rc !=0) && (rc != PTHREAD_BARRIER_SERIAL_THREAD))
				{
					perror("Could not wait on barrier");
					exit(-1);
				}
				pthread_exit(0);			
			}
			
			deadline_miss(tp);
			d_miss[argument-1]=(*tp).dmiss;
			
			wait_for_period(tp);
		}
}

//--------------------------------------------------------------------------------------------
//			MAIN TASK
//--------------------------------------------------------------------------------------------

void * m_task(void * arg)
{
struct	task_par	*tp;
struct timespec	t,at;

		tp= (struct task_par*)arg;
		set_period(tp);
		
		draw_deadline(*tp, 0, mod);
		draw_activation(*tp, 0, mod);

		while(1){			
			control_CPU("m task", pthread_self(), 0);			
			analysis_key();
			clock_gettime(CLOCK_MONOTONIC, &t);
			time_add_ms(&t, 50);
			do{
				use = true;
				run_task=0;
				clock_gettime(CLOCK_MONOTONIC, &at);
			}while((time_cmp(at, t)<=0)&(stop==0));

			wait_for_period(tp);
		}
}

//--------------------------------------------------------------------------------------------
//			WORKLOAD TASK
//--------------------------------------------------------------------------------------------

void * workload_task(void * arg)
{
struct	task_par	*tp;

		tp= (struct task_par*)arg;
		set_period(tp);

		while(1){			
			control_CPU("workload task", pthread_self(), 1);
			
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
			
			wait_for_period(tp);
		}
}

//--------------------------------------------------------------------------------------------
//			TIME ANALYSIS
//--------------------------------------------------------------------------------------------

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
