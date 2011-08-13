
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION reduce_by_distance
#include "scheme.h"

int reduce_by_distance(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  float min_distance = (argc == 1) ? 0.5 : atof(argv[1]);
  
  long vertexes_reduced = 0;
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    long i,j;
    if (shape->num_vertex_arrays > 1) { fprintf(stderr, "reduce only handles one vertex array at the moment.\n"); exit(1); }
    for (i = 0 ; i < shape->num_vertex_arrays ; i++)
    {
      struct VertexArray * va = &shape->vertex_arrays[i];
      if (va->array_type != GL_VERTEX_ARRAY) continue;
      
      float * prev_vertex = NULL;
      for (j = 0 ; j < shape->num_vertexs ; j++)
      {
        float * vertex = &va->vertexs[j*va->num_dimensions];
        
        float distance = 0.0;
        if (prev_vertex != NULL)
        {
          float x_diff = fabs(vertex[0] - prev_vertex[0]);
          float y_diff = fabs(vertex[1] - prev_vertex[1]);
          distance = sqrt(x_diff*x_diff + y_diff*y_diff);
        }
        
        if (prev_vertex != NULL && distance < min_distance)
        {
          j--;
          shape->num_vertexs--;
          memmove(vertex, vertex+va->num_dimensions, sizeof(float)*(shape->num_vertexs-j)*va->num_dimensions);
          vertexes_reduced++;
        }
        else
        {
          prev_vertex = vertex;
        }
      }
    }
    
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
  
  fprintf(stderr, "%s: %ld vertexes removed\n", argv[0], vertexes_reduced);
}
