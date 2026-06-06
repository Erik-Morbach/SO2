#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

//#define DEBUG
bool running;

int idxVector;
int vectorLen;
int *vector;
pthread_mutex_t vectorRead = PTHREAD_MUTEX_INITIALIZER;

int idxLMedia;
int idxRMedia;
int somaMediaLen;
int *somaMedia;
pthread_mutex_t somaMediaRead = PTHREAD_MUTEX_INITIALIZER;

int delta;


void sleep_ms(long milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void* temperaturaFazenda(void *args) {
  idxVector = 0;
  while(running) {
    pthread_mutex_lock(&vectorRead);
    const int temp = 1 + (rand()%1024);
    vector[idxVector] = temp;
    idxVector = (idxVector+1)%vectorLen;
    pthread_mutex_unlock(&vectorRead);
#ifdef DEBUG
    printf("Adding %d\n", temp);
#endif
    sleep_ms(delta);
  }
  return NULL;
}

void* temperaturaMedia(void *args) {
  // Esse valor poderia estar sendo calculado na inserção do temp no vector mantendo uma soma total
  const int tempo = *(int*)args;
  while(running) {
    const int totalQnt = tempo*1000 / delta;
    int med = 0;
    int curQnt = totalQnt;
    pthread_mutex_lock(&vectorRead);
    int cur = idxVector-1;
    while(curQnt-- > 0) {
      med += vector[(cur+vectorLen)%vectorLen];
      cur = (cur-1 + vectorLen)%vectorLen;
    }
    pthread_mutex_unlock(&vectorRead);
#ifdef DEBUG
    printf("Soma %d ->  %d\n", med, med/totalQnt);
#endif
    med/=totalQnt;
    pthread_mutex_lock(&somaMediaRead);
    somaMedia[idxRMedia] = med;
    idxRMedia=(idxRMedia+1)%somaMediaLen;
    pthread_mutex_unlock(&somaMediaRead);
    sleep_ms(tempo*1000);
  }
  return NULL;
}

void* verificacaoTemperaturaMedia(void *args) {
  const int limite = *(int*)args;
  while(running) {
    if(idxLMedia == idxRMedia) {
#ifdef DEBUG
      printf(".");
      fflush(stdout);
#endif
      sleep_ms(100);
      continue;
    }
#ifdef DEBUG
    printf("Verificando temperatura Media %d\n", somaMedia[idxLMedia]);
#endif
    pthread_mutex_lock(&somaMediaRead);
    if(somaMedia[idxLMedia] < limite) {
      printf("Erro de temperatura abaixo do limite:\n\tTemperatura:%d\n\tLimite:%d\nAtive a irrigação\n", somaMedia[idxLMedia], limite);
    }
    idxLMedia = (idxLMedia+1)%somaMediaLen;
    pthread_mutex_unlock(&somaMediaRead);
  }
  return NULL;
}

int main() { 
  srand(time(NULL));
  pthread_t leituraFazenda, leituraMedia, verificacao;
  delta = 100;
  int tempo = 10;
  int limite = 512;
  running = true;
  idxVector = 0;
  idxLMedia = 0;
  idxRMedia = 0;
  vectorLen = (int)(tempo*delta*1.5);
  vector = malloc(sizeof(int) * vectorLen);
  somaMediaLen = 1000;
  somaMedia = malloc(sizeof(int) * somaMediaLen);

  pthread_create(&leituraFazenda, NULL, &temperaturaFazenda, NULL);
  pthread_create(&leituraMedia, NULL, &temperaturaMedia, &tempo);
  pthread_create(&verificacao, NULL, &verificacaoTemperaturaMedia, &limite);

  sleep_ms(tempo*2*1000);
  running = false;
 
  void* status = malloc(1);
  pthread_join(leituraFazenda, &status);
  pthread_join(leituraMedia, &status);
  pthread_join(verificacao, &status);
  free(vector);
  free(somaMedia);
  return 0;
}
