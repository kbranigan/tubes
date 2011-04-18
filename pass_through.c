
#include <stdio.h>
#include <stdlib.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION pass_through
#include "scheme.h"

int pass_through(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    /* manipulate data here if you like */
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
}
