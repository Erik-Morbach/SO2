#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
int main() {
 size_t fatia = 104857600; /* 100 MB */
 void *pont;
 for(;;) {
 pont = malloc(fatia);
 mlock(pont,fatia/1.1);
 sleep(2);
 }
 return 0;
}