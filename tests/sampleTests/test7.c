/* Built-in syscalls */
void print_int(int integer);
void print_string(char *string);
int read_int(void);
void read_string(char *buffer, int length);
void exit(void);

void strcpy(char *dest, char *src);
int factorial(int n);

int main(void) {
  char prompt[80];
  int n;
  int f;

  strcpy(prompt, "Factorial of ");
  print_string(prompt);
  
  n = read_int();
  
  strcpy(prompt, " equals ");
  print_string(prompt);
  
  f = factorial(n);

  print_int(f);
  
  strcpy(prompt, "\n");
  print_string(prompt);

  return 0;
}

char *strcpy(char *dest, char *src) {
  char *initial_dest;

  initial_dest = dest;
  do {
    *dest++ = *src;
  } while(*src++);
  return initial_dest;
}

int factorial(int n) {
  if(n > 1) {
    return n * factorial(n-1);
  } else {
    return 1;
  }
}

/* casting from int to char - do an AND with FF (8 1s)
 */

/* Constant propagation - interesting
 */
