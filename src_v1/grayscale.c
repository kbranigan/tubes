
#include <stdio.h>
#include <stdlib.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION grayscale
#include "scheme.h"

int grayscale(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    if (shape->num_vertex_arrays > 1 && shape->vertex_arrays[1].array_type == GL_COLOR_ARRAY)
    {
      if (shape->vertex_arrays[1].num_dimensions >= 3)
      {
        int i;
        for (i = 0 ; i < shape->num_vertexs ; i++)
        {
          float * cv = get_vertex(shape, 1, i);
          cv[0] = cv[0] * 0.30 + cv[1] * 0.59 + cv[2] * 0.11; // http://en.wikipedia.org/wiki/Grayscale
          cv[1] = cv[2] = cv[0];
          // alpha, if there is one, can stay unchanged FOR ALL I CARE
        }
      }
      else
        fprintf(stderr, "ERROR: only supports grayscaling 3 or 4 dimensional colors\n");
    }
    
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
}
