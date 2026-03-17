#include <stdio.h>
#include "hello copy.c"

void hello(const char* name) {
  hello2(name);
  
  printf("Hello %s from C 1!\n", name);
}