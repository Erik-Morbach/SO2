#include <unistd.h>
#include <stdio.h>
int main() {
 printf ("Entrando em um laco infinito...\n");
 for(;;) { sleep (2); }
 return 0;
}