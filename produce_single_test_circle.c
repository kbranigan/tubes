
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDOUT_IS_PIPED
#define SCHEME_FUNCTION produce_single_test_circle
#include "scheme.h"

int produce_single_test_circle(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  int num_dimensions = argc < 2 ?  2 : atoi(argv[1]);
  int num_vertexs    = argc < 3 ? 64 : atoi(argv[2]);
  if (num_dimensions != 2 && num_dimensions != 3) { fprintf(pipe_err, "num_dimensions (%d) is invalid, defaulting to 2\n", num_dimensions); num_dimensions = 2; }
  if (num_vertexs < 3 || num_vertexs > 10000) { fprintf(pipe_err, "num_vertexs (%d) is invalid, defaulting to 64\n", num_vertexs); num_vertexs = 64; }
  
  struct Shape * shape = new_shape();
  shape->gl_type = GL_LINE_LOOP;
  
  struct VertexArray *va = get_or_add_array(shape, GL_VERTEX_ARRAY);
  va->num_dimensions = num_dimensions;
  
  long i;
  for (i = 0 ; i < num_vertexs ; i++)
  {
    float v[3] = { cos(i/(float)num_vertexs*(2*3.14159265)), sin(i/(float)num_vertexs*(2*3.14159265)), 0.0 };
    append_vertex(shape, v);
  }
  write_shape(pipe_out, shape);
  free_shape(shape);
}
