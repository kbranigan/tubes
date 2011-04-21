
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDIN_IS_PIPED
#define SCHEME_FUNCTION inspect
#include "scheme.h"

int inspect(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  int num_vertexs_to_show = 4;
  int c;
  while ((c = getopt(argc, argv, "n:")) != -1)
  switch (c)
  {
    case 'n':
      num_vertexs_to_show = clamp_int(atoi(optarg), 3, 100);
      break;
    default:
      abort();
  }
  
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
    {
      long count_zero = 0;
      long i;
  
      char gl_types_c[8][20] = {"GL_POINTS", "GL_LINES", "GL_LINE_LOOP", "GL_LINE_STRIP", "GL_TRIANGLES", "GL_TRIANGLE_STRIP", "GL_TRIANGLE_FAN"};
  
      fprintf(stderr, "shape:\n");
      fprintf(stderr, "  unique_set_id: %d\n", shape->unique_set_id);
      fprintf(stderr, "  gl_type: %s\n", (shape->gl_type >= 0 && shape->gl_type < 8) ? gl_types_c[shape->gl_type] : "????");
      fprintf(stderr, "  num_attributes: %d\n", shape->num_attributes);
      if (shape->num_attributes) fprintf(stderr, "  attributes:\n");
      for (i = 0 ; i < shape->num_attributes ; i++)
      {
        struct Attribute * attribute = &shape->attributes[i];
        fprintf(stderr, "    %s(%d): '%s'\n", attribute->name, attribute->value_length, attribute->value);
      }
      fprintf(stderr, "  num_vertexs: %d\n", shape->num_vertexs);
      fprintf(stderr, "  num_vertex_arrays: %d\n", shape->num_vertex_arrays);
      if (shape->num_vertex_arrays) fprintf(stderr, "  vertex_arrays:\n");
      for (i = 0 ; i < shape->num_vertex_arrays ; i++)
      {
        fprintf(stderr, "    array_type: %d\n", shape->vertex_arrays[i].array_type);
        fprintf(stderr, "    num_dimensions: %d\n", shape->vertex_arrays[i].num_dimensions);
        fprintf(stderr, "    vertexs:\n");
        if (shape->num_vertexs > 0 && shape->vertex_arrays[i].num_dimensions > 0)
        {
          long j,k;
          for (k = 0 ; k < shape->num_vertexs ; k++)
          {
            if (k < num_vertexs_to_show) fprintf(stderr, "      ");
            int is_zero = 1;
            for (j = 0 ; j < shape->vertex_arrays[i].num_dimensions ; j++)
            {
              if (shape->vertex_arrays[i].vertexs[k*shape->vertex_arrays[i].num_dimensions + j] != 0.0) is_zero = 0;
              if (k < num_vertexs_to_show) fprintf(stderr, "%f ", shape->vertex_arrays[i].vertexs[k*shape->vertex_arrays[i].num_dimensions + j]);
            }
            if (is_zero) count_zero ++;
            if (k < num_vertexs_to_show) fprintf(stderr, "\n");
            else if (k == num_vertexs_to_show) fprintf(stderr, "      ...\n");
          }
        }
        if (i == num_vertexs_to_show) fprintf(stderr, "    [...]\n");
      }
      if (count_zero > 0) fprintf(stderr, "  count_zero: %ld\n", count_zero);
    }
  }
  
  fprintf(pipe_err, "{\n");
  if (num_shapes_with_no_vertexs > 0) printf("  \"num_shapes_with_no_vertexs\": %d,\n", num_shapes_with_no_vertexs);
  fprintf(pipe_err, "  \"num_shapes\": %d,\n", num_shapes);
  fprintf(pipe_err, "  \"num_vertexs\": %d,\n", num_vertexs);
  fprintf(pipe_err, "  \"num_each_gl_type\": [%d,%d,%d,%d,%d,%d,%d]\n", num_each_gl_type[0], num_each_gl_type[1], num_each_gl_type[2], num_each_gl_type[3], num_each_gl_type[4], num_each_gl_type[5], num_each_gl_type[6]);
  fprintf(pipe_err, "}\n");
}