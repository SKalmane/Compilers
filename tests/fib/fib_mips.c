void print_int(int i);
void print_string(char *s);

int fib(int n) {
    return (n < (int)2 ? n : (fib(n-1) + fib(n-2)));
}

int main(int argc, char *argv[]) {
  int x;
  x = 10;
  print_string("Hello! The fibonacci number for ");
  print_int(x);
  print_string(" is: ");
  print_int(fib(x));
  print_string("\n");
  return 0;
}
