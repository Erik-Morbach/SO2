#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#define N 5
#define LEFT (i+N-1)%N
#define RIGHT (i+1)%N
#define THINKING 0
#define HUNGRY 1
#define EATING 2

int state[N], i, int_rand;
float float_rand;

void estatistica(void);
void mostrar(void);
void pensar(int);
void pegar_garfo(int);
void por_garfo(int);
void comer(int);
void test(int);
void *acao_filosofo(void *);

sem_t* mutex;
sem_t* sem_fil[N];
int args[N];
int tentouComer[N];
int comeu[N];
bool running;

int main(){
	for(int i=0;i<N;i++) {
		state[i] = THINKING;
        tentouComer[i] = 0;
        comeu[i] = 0;
	}
	mostrar();

	int res;
	pthread_t thread[N];
	void *thread_result;
	mutex = sem_open("/mutex", O_CREAT, 0644, 1);
	for(i=0; i<N; i++){
		char name[20];
		sprintf(name, "/sem_fil_%d", i);
		sem_fil[i] = sem_open(name, O_CREAT, 0644, 0);
	}

    running = true;
	for(i=0; i<N; i++){
        args[i] = i;
		res = pthread_create(&thread[i],NULL,acao_filosofo,&args[i]);
		if(res!=0){
			perror("Erro na inicialização da thread!");
			exit(EXIT_FAILURE);
		}
	}

    sleep(10);
    running = false;

	for(i=0; i<N; i++){
        res = pthread_join(thread[i],&thread_result);
        if(res!=0){
            perror("Erro ao fazer join nas threads!");
            exit(EXIT_FAILURE);
        }
    }
    estatistica();
    return 0;
}

void estatistica() {
    printf("-----------------------------------------\n");
    printf("   Estatistica do jantar dos filosofos   \n");
    printf("-----------------------------------------\n");
    for(int i=0;i<N;i++) {
        printf("Filosofo %2d tentou comer %2d mas só comeu %2d refeicoes.\n", i, tentouComer[i], comeu[i]);
    }
}

void mostrar(){
    for(i=1; i<=N; i++){
        if(state[i-1] == THINKING){
            printf("O filosofo %d esta pensando!\n", i);
        }
        if(state[i-1] == HUNGRY){
            printf("O filosofo %d esta com fome!\n", i);
        }
        if(state[i-1] == EATING){
            printf("O filosofo %d esta comendo!\n", i);
        }
    }
    printf("\n");
}
void *acao_filosofo(void *j){
    int i = *(int*) j;
    while(running){
        pensar(i);
        pegar_garfo(i);
        comer(i);
        por_garfo(i);
    }
    return NULL;
}
void pensar(int i){
    float_rand=0.001*random();
    int_rand=float_rand;
    usleep(int_rand);
}
void pegar_garfo(int i){
    sem_wait(mutex);
    state[i]=HUNGRY;
    mostrar();
    test(i);
    sem_post(mutex);
    sem_wait(sem_fil[i]);
}
void test(int i){
    tentouComer[i]++;
    if(state[i] == HUNGRY && state[LEFT] != EATING && state[RIGHT] != EATING){
        comeu[i]++;
        state[i]=EATING;
        mostrar();
        sem_post(sem_fil[i]);
    }
}
void comer(int i){
    float_rand=0.001*random();
    int_rand=float_rand;
    usleep(int_rand);
}
void por_garfo(int i){
	sem_wait(mutex);
	mostrar();
	test(LEFT);
	test(RIGHT);
	state[i]=THINKING;
	sem_post(mutex);
}
