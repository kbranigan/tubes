
#include <stdio.h>
#include <stdlib.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION remove_attributes
#include "scheme.h"

int remove_attributes(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    int i;
    for (i = 0 ; i < shape->num_attributes ; i++)
      free(shape->attributes[i].value);
    free(shape->attributes);
    shape->num_attributes = 0;
    
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
}
