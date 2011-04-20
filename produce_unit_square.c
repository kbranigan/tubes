
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDOUT_IS_PIPED
#define SCHEME_FUNCTION produce_unit_square
#include "scheme.h"

int produce_unit_square(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  int num_dimensions = 2;
  int c;
  float width = 1.0;
  float height = 1.0;
  while ((c = getopt(argc, argv, "d:w:h:")) != -1)
  switch (c)
  {
    case 'd':
      num_dimensions = clamp_int(atoi(optarg), 2, 3);
      break;
    case 'w':
      width = clamp_float(atof(optarg), -10000000, 10000000);
      break;
    case 'h':
      height = clamp_float(atof(optarg), -10000000, 10000000);
      break;
  }
  
  struct Shape * shape = new_shape();
  shape->gl_type = GL_LINE_LOOP;
  
  struct VertexArray *va = get_or_add_array(shape, GL_VERTEX_ARRAY);
  va->num_dimensions = num_dimensions;
  
  float v0[3] = { 0.0, 0.0, 0.0 };
  append_vertex(shape, v0);
  float v1[3] = { 0.0, height, 0.0 };
  append_vertex(shape, v1);
  float v2[3] = { width, height, 0.0 };
  append_vertex(shape, v2);
  float v3[3] = { width, 0.0, 0.0 };
  append_vertex(shape, v3);
  
  write_shape(pipe_out, shape);
  free_shape(shape);
}
