
#include <stdio.h>
#include <stdlib.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION reset_unique_set_ids
#include "scheme.h"

int reset_unique_set_ids(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  int unique_set_id = 1;
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    shape->unique_set_id = unique_set_id++;
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
}
