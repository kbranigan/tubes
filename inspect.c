
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDIN_IS_PIPED
#define SCHEME_FUNCTION inspect
#include "scheme.h"

int inspect(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  int num_shapes = 0;
  int num_shapes_with_no_vertexs = 0;
  int num_vertexs = 0;
  int num_each_gl_type[7] = {0,0,0,0,0,0,0};
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    if (shape->gl_type < 7) num_each_gl_type[shape->gl_type]++;
    if (shape->num_vertexs == 0) num_shapes_with_no_vertexs++;
    
    num_vertexs += shape->num_vertexs;
    num_shapes++;
    if (num_shapes <= 2)
      inspect_shape(pipe_out, shape);
  }
  
  fprintf(pipe_err, "{\n");
  if (num_shapes_with_no_vertexs > 0) printf("  \"num_shapes_with_no_vertexs\": %d,\n", num_shapes_with_no_vertexs);
  fprintf(pipe_err, "  \"num_shapes\": %d,\n", num_shapes);
  fprintf(pipe_err, "  \"num_vertexs\": %d,\n", num_vertexs);
  fprintf(pipe_err, "  \"num_each_gl_type\": [%d,%d,%d,%d,%d,%d,%d]\n", num_each_gl_type[0], num_each_gl_type[1], num_each_gl_type[2], num_each_gl_type[3], num_each_gl_type[4], num_each_gl_type[5], num_each_gl_type[6]);
  fprintf(pipe_err, "}\n");
}