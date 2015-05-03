void print_int(int i);
void print_string(char *s);
int read_int(void);

int fib(int n) {
    return (n < (int)2 ? 1 : (fib(n-1) + fib(n-2)));
}

int main(int argc, char *argv[]) {
  int x;
  print_string("Hi! Please enter a number:");
  x = read_int();
  print_string("The fibonacci number for ");
  print_int(x);
  print_string(" is: ");
  print_int(fib(x));
  print_string("\n");
  return 0;
}
