
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDOUT_IS_PIPED
#define SCHEME_FUNCTION produce_random_data
#include "scheme.h"

int produce_random_data(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  int num_dimensions = 2;
  int num_vertexs = 64;
  float width = 1.0;
  float height = 1.0;
  int c;
  while ((c = getopt(argc, argv, "d:v:w:h:")) != -1)
  switch (c)
  {
    case 'd':
      num_dimensions = clamp_int(atoi(optarg), 2, 3);
      break;
    case 'v':
      num_vertexs = clamp_int(atoi(optarg), 3, 10000);
      break;
    case 'w':
      width = clamp_float(atof(optarg), -10000000, 10000000);
      break;
    case 'h':
      height = clamp_float(atof(optarg), -10000000, 10000000);
      break;
    default:
      abort();
  }
  
  struct Shape * shape = new_shape();
  srand(time(NULL));
  
  struct VertexArray *va = get_or_add_array(shape, GL_VERTEX_ARRAY);
  va->num_dimensions = num_dimensions;
  
  long i;
  for (i = 0 ; i < num_vertexs ; i++)
  {
    float v[3] = { rand()/(float)RAND_MAX*width, rand()/(float)RAND_MAX*height, rand()/(float)RAND_MAX };
    append_vertex(shape, v);
  }
  write_shape(pipe_out, shape);
  free_shape(shape);
}
