#include <stdio.h>
#include <stdlib.h>//required for random()
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <native/task.h>
#include <native/timer.h>
#include <native/sem.h>
#include <rtdk.h>
#include <time.h>//required to use 'srand(time(NULL))'

#define N 5
#define LEFT (i + N - 1) % N
#define RIGHT (i + 1) % N
#define THINKING 0
#define HUNGRY 1
#define EATING 2

void test(int );

int statePhil[N];
int stateFork[N];
double int_rand;

//semaforos
//static RT_SEM mutex;
static RT_SEM forkk[N];
static RT_SEM mysync;
static RT_TASK phil[N];
static RT_TASK show;

//tarefa que mostra o estado dos N filosofos
void showStatus(void *arg)
{
	rt_sem_p(&mysync,TM_INFINITE); 
    int aux=0;  

    while (1) {		
		//rt_printf("\n\n\t\tTask show Status\n\n"); 
        int i;
        for (i = 1; i <= N; i++)
        {
            if (statePhil[i - 1] == THINKING)
            {
                rt_printf("Philosopher %d is Thinking!\n", i);
            }
            if (statePhil[i - 1] == HUNGRY)
            {
                rt_printf("Philosopher %d is Hungry!\n", i);
            }
            if (statePhil[i - 1] == EATING)
            {
                rt_printf("Philosopher %d is Eating!\n", i);
                stateFork[i - 1] = i;
                aux = LEFT;
                if(aux == 0) aux = N;
                stateFork[(aux)-1] = i;
                //rt_printf("\t\t\t\t\t%d !\n", (LEFT)-1);
            }
        }
        //rt_printf("\n");
        for (i = 1; i <= N; i++)
        {
            if (stateFork[i - 1] != 0)
            {
                rt_printf("Fork %d is held by philosopher %d!\n", i, stateFork[i - 1]);
            }else{
                rt_printf("Fork %d is FREE!\n", i);
            }
            
        }
        rt_printf("\n");
        for (i = 0; i < N; i++) stateFork[i] = 0;

		rt_task_wait_period(NULL);
	}
	return;
}

//acao do filosofo
void action_philosopher(void *j)
{
    int i = *(int *)j;
    
    while (1)
    {
        //pensar//
        //srand(time(NULL));//required for "randomness"
        int_rand = ((random() % (2000 + 1 - 501)) + 501);			
        rt_task_sleep(int_rand*1000000);
        /////////
        //pegar garfo//
        //rt_sem_p(&mutex,TM_INFINITE);
        statePhil[i] = HUNGRY;
       // mostrar();
        test(i);
        //rt_sem_v(&mutex);
        rt_sem_p(&forkk[i],TM_INFINITE);
        ///////////////
        //comer//
        //srand(time(NULL));//required for "randomness"
        int_rand = ((random() % (5000 + 1 - 501)) + 501);
        rt_task_sleep(int_rand*1000000);
        /////////
        //por garfo//
       // rt_sem_p(&mutex,TM_INFINITE);
        statePhil[i] = THINKING;
        //mostrar();
        test(LEFT);
        test(RIGHT);
        //rt_sem_v(&mutex);
        /////////////
    }
}



//funcao que testa se o filosofo pode comer
void test(int i)
{
    if (statePhil[i] == HUNGRY && statePhil[LEFT] != EATING && statePhil[RIGHT] != EATING)
    {
        statePhil[i] = EATING;

        //stateFork[i] = i;
        //stateFork[LEFT] = i;
        
        rt_sem_v(&forkk[i]);
    }
    
}



void main()
{
	char fk[5][10] ;
	char ph[5][15] ;
	int res,i;
	/* Perform auto-init of rt_print buffers if the task doesn't do so */
	rt_print_auto_init(1);
	/* Avoids memory swapping for this program */
	mlockall(MCL_CURRENT|MCL_FUTURE);
    for (i = 0; i < N; i++)
    {
        statePhil[i] = 0;
        stateFork[i] = 0;
    }
    //mostrar();
	//rt_task_sleep(5000000000);
	sprintf(fk[0],"Fork1");
	sprintf(fk[1],"Fork2");
	sprintf(fk[2],"Fork3");
	sprintf(fk[3],"Fork4");
	sprintf(fk[4],"Fork5");
	sprintf(ph[0],"Philosopher1");
	sprintf(ph[1],"Philosopher2");
	sprintf(ph[2],"Philosopher3");
	sprintf(ph[3],"Philosopher4");
	sprintf(ph[4],"Philosopher5"); 

    //inicia os semaforos    
	// semaphore to sync task startup on
	res = rt_sem_create(&mysync,"MySemaphore",1,S_FIFO);
    if (res != 0)
    {
        perror("Error creating the syncing semaphore!");
        exit(EXIT_FAILURE);
    }
	// create semaphores(forks) 
    for (i = 0; i < N; i++)
    {
        res = rt_sem_create(&forkk[i],fk[i],0,S_FIFO);
        if (res != 0)
        {
            perror("Error creating the fork semaphore!");
            exit(EXIT_FAILURE);
        }
    }

    /////////////// create tasks(Philosophers) ///////////////////////////
    for (i = 0; i < N; i++)
    {
        //res = pthread_create(&thread[i], NULL, acao_filosofo, &i);
		res = rt_task_create(&phil[i], ph[i], 0, 50, T_JOINABLE);
        if (res != 0){
            perror("Error creating task");
            exit(EXIT_FAILURE);
        }
    }
    res = rt_task_create(&show, NULL, 0, 60, T_JOINABLE);
    if (res != 0){
        perror("Error creating task");
        exit(EXIT_FAILURE);
    }
    //////////////////////////////////////////////////////////////////////

    rt_task_set_periodic(&show, TM_NOW, (500000000)); // 500ms

	//////////////// Start tasks ////////////////////////////////////////
	for (i = 0; i < N; i++)
    {
        //res = pthread_create(&thread[i], NULL, acao_filosofo, &i);
		res = rt_task_start(&phil[i], &action_philosopher, &i);
        if (res != 0)
        {
            perror("Erro na inicialização da thread!");
            exit(EXIT_FAILURE);
        }
    }
    rt_task_start(&show, &showStatus, 0);
    ////////////////////////////////////////////////////////////////////////

    ///////////// faz um join nas tarefas ///////////////////////////////////
    for (i = 0; i < N; i++)
    {
        res = rt_task_join(&phil[i]);
        if (res != 0)
        {
            perror("Erro ao fazer join nas threads!");
            exit(EXIT_FAILURE);
        }
    }
    res = rt_task_join(&show);
        if (res != 0)
        {
            perror("Erro ao fazer join nas threads!");
            exit(EXIT_FAILURE);
        }
    ////////////////////////////////////////////////////////////////////////
	rt_sem_broadcast(&mysync);
}
