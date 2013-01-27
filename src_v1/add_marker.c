
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION add_marker
#include "scheme.h"

int add_marker(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  int xy_supplied = 0;
  float x = 0, y = 0;
  float distance = 1;
  
  int c;
  while ((c = getopt(argc, argv, "c:d:")) != -1)
  switch (c)
  {
    case 'c':
      if (strstr(optarg, ",")==0)
        fprintf(stderr, "-c argument requires a comma\n");
      else
      {
        xy_supplied = 1;
        x = atof(optarg);
        y = atof(strstr(optarg, ",")+1);
      }
      break;
    case 'd':
      distance = atof(optarg);
      break;
    default:
      abort();
  }
  
  if (xy_supplied == 0)
    fprintf(stderr, "%s: -c [x,y] required\n", argv[0]);
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
  
  if (xy_supplied)
  {
    struct Shape * shape = NULL;
    shape = new_shape();
    shape->gl_type = GL_LINES;
    float v[4] = { x-distance, y, 0, 0 };
    append_vertex(shape, v);
    v[0] += distance*2;
    append_vertex(shape, v);
    v[0] = x;
    v[1] = y - distance;
    append_vertex(shape, v);
    v[1] += distance*2;
    append_vertex(shape, v);
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
}
