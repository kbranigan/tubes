
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION delay
#include "scheme.h"

int delay(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  int msecs = 100000;
  int c;
  while ((c = getopt(argc, argv, "n:")) != -1)
  switch (c)
  {
    case 'n':
      msecs = clamp_int(atoi(optarg), 1, 999999);
      break;
    default:
      abort();
  }
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    usleep(msecs);
    
    write_shape(pipe_out, shape);
    free_shape(shape);
    fflush(pipe_out);
  }
}
