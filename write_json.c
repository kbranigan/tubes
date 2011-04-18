
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDIN_IS_PIPED
#define SCHEME_FUNCTION write_json
#include "scheme.h"

int write_json(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char gl_types_c[8][20] = {"GL_POINTS", "GL_LINES", "GL_LINE_LOOP", "GL_LINE_STRIP", "GL_TRIANGLES", "GL_TRIANGLE_STRIP", "GL_TRIANGLE_FAN"};
  
  int num_shapes = 0;
  fprintf(pipe_out, "[\n");
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    if (num_shapes != 0) fprintf(pipe_out, ",\n");
    
    fprintf(pipe_out, "  {\n");
    fprintf(pipe_out, "    \"unique_set_id\":\"%d\",\n", shape->unique_set_id);
    if (shape->num_attributes > 0)
    {
      fprintf(pipe_out, "    \"attributes\": {\n");
      int i;
      for (i = 0 ; i < shape->num_attributes ; i++)
      {
        struct Attribute * attribute = &shape->attributes[i];
        fprintf(pipe_out, "      \"%s\":\"%s\"%s\n", attribute->name, attribute->value, (i == shape->num_attributes-1)?"":",");
      }
      fprintf(pipe_out, "    },\n");
    }
    if (shape->num_vertex_arrays > 0 || shape->num_vertexs > 0)
    {
      fprintf(pipe_out, "    \"vertex_arrays\": [\n");
      int i;
      for (i = 0 ; i < shape->num_vertex_arrays ; i++)
      {
        struct VertexArray * va = &shape->vertex_arrays[i];
        fprintf(pipe_out, "%s      {\n", (i!=0?",\n":""));
        fprintf(pipe_out, "        \"array_type\": \"%d\",\n", va->array_type);
        fprintf(pipe_out, "        \"num_dimensions\": \"%d\",\n", va->num_dimensions);
        fprintf(pipe_out, "        \"vertexs\": [");
        int j;
        for (j = 0 ; j < shape->num_vertexs ; j++)
        {
          float * v = get_vertex(shape, j, i);
          if (va->num_dimensions == 2) fprintf(pipe_out, "[%f,%f]%s", v[0], v[1], j==shape->num_vertexs-1?"":",");
          else if (va->num_dimensions == 3) fprintf(pipe_out, "[%f,%f,%f]%s", v[0], v[1], v[2], j==shape->num_vertexs-1?"":",");
          else if (va->num_dimensions == 4) fprintf(pipe_out, "[%f,%f,%f,%f]%s", v[0], v[1], v[2], v[3], j==shape->num_vertexs-1?"":",");
        }
        fprintf(pipe_out, "]\n");
        fprintf(pipe_out, "      }");
      }
      fprintf(pipe_out, "\n    ],\n");
    }
    fprintf(pipe_out, "    \"gl_type\":\"%s\"\n", (shape->gl_type >= 0 && shape->gl_type <= 6) ? gl_types_c[shape->gl_type] : "????");
    free_shape(shape);
    fprintf(pipe_out, "  }");
    num_shapes++;
  }
  fprintf(pipe_out, "\n]\n");
}