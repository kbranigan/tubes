
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION add_attribute
#include "scheme.h"

int add_attribute(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char * name = NULL;
  char * value = NULL;
  int c;
  while ((c = getopt(argc, argv, "n:v:")) != -1)
  switch (c)
  {
    case 'n':
      name = malloc(strlen(optarg)+1);
      strcpy(name, optarg);
      break;
    case 'v':
      value = malloc(strlen(optarg)+1);
      strcpy(value, optarg);
      break;
    default:
      abort();
  }
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    fprintf(stderr, "%d\n", shape->num_attributes);
    set_attribute(shape, name, value);
    fprintf(stderr, "%d\n", shape->num_attributes);
    
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
  free(name);
  free(value);
}
