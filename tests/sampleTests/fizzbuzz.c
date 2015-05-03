void print_string(char *s);
void print_int(int i);
int read_int(void);


void print_fizz_buzz(int length) {
    int i;
    int remainderOnDividingByThree;
    int remainderOnDividingByFive;
    if(i < 1) {
        print_string("Please enter a number greater than zero..\n");
    } else {
        i = length;
        /* for(i = 1; i <= length; i++) { */
        remainderOnDividingByThree = (i % 3);
        remainderOnDividingByFive = (i % 5);
        if((!remainderOnDividingByThree) && (!remainderOnDividingByFive)) {
            print_string("FizzBuzz\n");
        } else if(!remainderOnDividingByFive) {
            print_string("Fizz\n");
        } else if(!remainderOnDividingByThree) {
            print_string("Buzz\n");
        }
        /* } */
    }
}

int main(void) {
    int length;
    print_string("Please enter a number: ");
    length = read_int();
    print_fizz_buzz(length);
}
