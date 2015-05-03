int factorial1(int n, int accumulator) {
  if (n <= 0) return accumulator;
  return factorial1(n-1, n*accumulator);
}

int factorial(int n) {
  return factorial1(n, 1);
}
