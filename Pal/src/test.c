#include<stdio.h>

int main() {
unsigned long int l_entry = 0x400480, argc = 1;
const char **argv = (void *)0x7fffffffe868;
const char **pal = (void *)0x7fffffffe878;
 
int x = ((int (*) (int, const char **, const char **))
              l_entry) (argc, argv, pal);
printf("%d",x);
}
