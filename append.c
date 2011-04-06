
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheme.h"

int main(int argc, char ** argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "usage: ./%s [filename]\n", argv[0]);
    exit(1);
  }
  
  FILE *fp = fopen(argv[1], "r");
  
  if (fp == NULL)
  {
    fprintf(stderr, "%s does not exist\n", argv[0]);
    exit(1);
  }
  
  assert_stdin_is_piped();
  assert_stdout_is_piped();
  
  read_header(stdin, CURRENT_VERSION);
  read_header(fp, CURRENT_VERSION);
  write_header(stdout, CURRENT_VERSION);
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(stdin)))
  {
    write_shape(stdout, shape);
    free_shape(shape);
  }
  while ((shape = read_shape(fp)))
  {
    write_shape(stdout, shape);
    free_shape(shape);
  }
}