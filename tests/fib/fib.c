void print_int(int i);
void print_string(char *s);
int atoi(char *s);

int fib(int n) {
    int a;
    int b;
    char *c;
    a = fib(n-1);
    b = fib(n-2);
    return (n < (int)2 ? n : (a + b));
}

int main(int argc, char *argv[]) {
  print_int(fib(atoi(argv[1])));
  print_string("Hello");
  print_string("\n");
  return 0;
}
