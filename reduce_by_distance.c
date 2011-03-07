
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "scheme.h"

int main(int argc, char ** argv)
{
  float min_distance = (argc == 1) ? 0.5 : atof(argv[1]);
  
  if (!stdin_is_piped())
  {
    fprintf(stderr, "%s needs a data source. (redirected pipe, using |)\n", argv[0]);
    exit(1);
  }
  
  if (!stdout_is_piped())
  {
    fprintf(stderr, "%s outputs binary content. Pipe it to something that can read it.\n", argv[0]);
    exit(1);
  }
  
  if (!read_header(stdin, CURRENT_VERSION)) { fprintf(stderr, "read header failed.\n"); exit(1); }
  if (!write_header(stdout, CURRENT_VERSION)) { fprintf(stderr, "write header failed.\n"); exit(1); }
  
  long vertexes_reduced = 0;
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(stdin)))
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
    
    write_shape(stdout, shape);
    free_shape(shape);
  }
  
  fprintf(stderr, "%s: %ld vertexes removed\n", argv[0], vertexes_reduced);
}
