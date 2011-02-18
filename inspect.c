
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheme.h"

int main(int argc, char ** argv)
{
  if (!stdin_is_piped())
  {
    fprintf(stderr, "%s needs a data source. (redirected pipe, using |)\n", argv[0]);
    exit(1);
  }
  
  read_header(stdin, CURRENT_VERSION);
  
  int num_shapes = 0;
  int num_shapes_with_no_vertexs = 0;
  int num_vertexs = 0;
  int num_each_gl_type[7] = {0,0,0,0,0,0,0};
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(stdin)))
  {
    if (shape->gl_type < 7) num_each_gl_type[shape->gl_type]++;
    if (shape->num_vertexs == 0) num_shapes_with_no_vertexs++;
    
    num_vertexs += shape->num_vertexs;
    num_shapes++;
    if (num_shapes <= 4)
      inspect_shape(stdout, shape);
  }
  
  printf("{\n");
  if (num_shapes_with_no_vertexs > 0) printf("  \"num_shapes_with_no_vertexs\": %d,\n", num_shapes_with_no_vertexs);
  printf("  \"num_shapes\": %d,\n", num_shapes);
  printf("  \"num_vertexs\": %d,\n", num_vertexs);
  printf("  \"num_each_gl_type\": [%d,%d,%d,%d,%d,%d,%d]\n", num_each_gl_type[0], num_each_gl_type[1], num_each_gl_type[2], num_each_gl_type[3], num_each_gl_type[4], num_each_gl_type[5], num_each_gl_type[6]);
  printf("}\n");
}