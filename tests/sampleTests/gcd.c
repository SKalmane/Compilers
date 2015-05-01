int gcd(int a, int b) {
  int c;
  a = (a < 0? -a: a);
  b = (b < 0? -b: b);
  c = a % b;
  
  while(c > 0) {
    a = b;
    b = c;
    c = a % b;
  }
  return b;
}
