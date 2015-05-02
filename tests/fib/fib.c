#include <stdio.h>
#include <stdlib.h>

void print_int(int i);
void print_string(char *s);
int atoi_sample(char *s);

int fib(int n) {
    return (n < (int)2 ? n : (fib(n-1) + fib(n-2)));
}

int main(int argc, char *argv[]) {
  print_int(fib(atoi_sample(argv[1])));
  print_string("Hello");
  print_string("\n");
  return 0;
}

void print_int(int i) {
    printf("%d\n", i);
}

void print_string(char *s) {
    printf("%s\n", s);
}

int atoi_sample(char *s) {
    return atoi(s); 
}
