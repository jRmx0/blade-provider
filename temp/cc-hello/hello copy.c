#include <stdio.h>
#include "hello copy 2.c"

void hello2(const char* name) {
  hello3(name);
  
  printf("Hello %s from C 2!\n", name);
}