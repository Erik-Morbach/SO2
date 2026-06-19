#include <unistd.h>
#include <stdlib.h>
int main() {
 size_t fatia = 104857600; /* 100 MB */
 for(;;) {
 malloc(fatia);
 sleep(2);
 }
 return 0;
}