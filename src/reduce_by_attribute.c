
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION reduce_by_attribute
#include "scheme.h"

int reduce_by_attribute(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char name[21] = "";
  char value[200] = "";
  int invert_search = 0;
  int c;
  while ((c = getopt(argc, argv, "n:v:i")) != -1)
  switch (c)
  {
    case 'n':
      strncpy(name, optarg, 20);
      break;
    case 'v':
      strncpy(value, optarg, 20);
      break;
    case 'i':
      invert_search = 1;
      break;
    default:
      abort();
  }
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    if (shape->num_attributes == 0) continue;
    if (!invert_search)
    {
      if (strcmp(get_attribute(shape, name), value)!=0) continue;
    }
    else
    {
      if (strcmp(get_attribute(shape, name), value)==0) continue;
    }
    
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
}
