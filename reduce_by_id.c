
#include <stdio.h>
#include <stdlib.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION reduce_by_id
#include "scheme.h"

int reduce_by_id(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  float unique_set_id = (argc == 1) ? 0.5 : atof(argv[1]);
  
  long shapes_read = 0;
  long shapes_write = 0;
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    shapes_read++;
    if (shape->unique_set_id == unique_set_id)
    {
      shapes_write++;
      write_shape(pipe_out, shape);
    }
    
    free_shape(shape);
  }
  
  fprintf(stderr, "%s: %ld shapes reduced to %ld\n", argv[0], shapes_read, shapes_write);
}
