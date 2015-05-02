void print_int(int i); 
void print_string(char *s);

void helper_print_int(int n) {
    print_int(n);
}

/* void helper_print_string(char *s) { */
/*     print_string(s); */
/* } */

int main(int n) {
    int a;
    int d[5];
    int bla;
    int c;
    int **p;
    char *f;
    f = "Hello, world\n";
    bla = 10;
    c = (int)bla;
    /* d[4] = c; */
    /* a = (bla && 5); */
    /* c = bla ? a : 10; */
    helper_print_int(c);
    /* helper_print_string(f); */
    return 10;
}
