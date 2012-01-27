
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDOUT_IS_PIPED
#define SCHEME_FUNCTION produce_unit_circle
#include "scheme.h"

int produce_unit_circle(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  int num_dimensions = 2;
  int num_vertexs = 64;
  int add_color = 0;
  int c;
  while ((c = getopt(argc, argv, "d:v:c")) != -1)
  switch (c)
  {
    case 'd': num_dimensions = clamp_int(atoi(optarg), 2, 3); break;
    case 'v': num_vertexs    = clamp_int(atoi(optarg), 3, 10000); break;
    case 'c': add_color = 1; break;
    default: abort();
  }
  
  struct Shape * shape = new_shape();
  shape->gl_type = GL_LINE_LOOP;
  
  get_or_add_array(shape, GL_VERTEX_ARRAY);
  set_num_vertexs(shape, num_vertexs);
  set_num_dimensions(shape, 0, num_dimensions);
  
  if (add_color)
    get_or_add_array(shape, GL_COLOR_ARRAY);
  
  long i;
  for (i = 0 ; i < num_vertexs ; i++)
  {
    float v[3] = { cos(i/(float)num_vertexs*(2*3.14159265)), sin(i/(float)num_vertexs*(2*3.14159265)), 0.0 };
    set_vertex(shape, 0, i, v);
    
    float cv[3] = { cos(i/(float)num_vertexs*(2*3.14159265)), sin(i/(float)num_vertexs*(2*3.14159265)), cos(i/(float)num_vertexs*(2*3.14159265)) * sin(i/(float)num_vertexs*(2*3.14159265)) };
    if (add_color)
      set_vertex(shape, 1, i, cv);
  }
  write_shape(pipe_out, shape);
  free_shape(shape);
}
