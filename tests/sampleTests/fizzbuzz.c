void print_string(char *s);
void print_int(int i);
int read_int(void);


void print_fizz_buzz(int length) {
    int i;
    int len;
    int remainderOnDividingByThree;
    int remainderOnDividingByFive;
    len = length;
    if(len < 1) {
      print_string("Please enter a number greater than zero..\n");
    } else {
      for(i = 1; i <= len; ++i) {
        remainderOnDividingByThree = (i % 3);
        remainderOnDividingByFive = (i % 5);
        if(!(i % 3) && !(i % 5)) {
	  print_int(i);
	  print_string("     ");
	  print_string("FizzBuzz\n");
        } else if(!(i % 5)) {
	  print_int(i);
	  print_string("     ");
	  print_string("Fizz\n");
        } else if(!(i % 3)) {
	  print_int(i);
	  print_string("     ");
	  print_string("Buzz\n");	  
        }
      }
    }
}

int main(void) {
    int length;
    print_string("Please enter a number: ");
    length = read_int();
    print_fizz_buzz(length);
}
