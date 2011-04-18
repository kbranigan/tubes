
#include <stdio.h>
#include <stdlib.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDIN_IS_PIPED
#define SCHEME_FUNCTION bbox
#include "scheme.h"

int bbox(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  struct Shape * shape = NULL;
  
  int num_dimensions = 0;
  struct minmax {
    float min;
    float max;
  };
  struct minmax *bbox = NULL;
  
  while ((shape = read_shape(pipe_in)))
  {
    long i, j, k;
    for (i = 0 ; i < shape->num_vertex_arrays ; i++)
    {
      struct VertexArray * va = &shape->vertex_arrays[i];
      
      if (va->array_type != GL_VERTEX_ARRAY) continue;
      if (va->vertexs == NULL) { fprintf(pipe_err, "vertex array %ld is NULL\n", i); return EXIT_FAILURE; }
      if (va->num_dimensions > num_dimensions)
      {
        bbox = (struct minmax*)realloc(bbox, sizeof(struct minmax)*va->num_dimensions);
        for (j = num_dimensions ; j < va->num_dimensions ; j++)
        {
          bbox[j].min =  1000000.0;
          bbox[j].max = -1000000.0;
        }
        num_dimensions = va->num_dimensions;
      }
      
      for (j = 0 ; j < shape->num_vertexs ; j++)
      {
        for (k = 0 ; k < va->num_dimensions ; k++)
        {
          if (va->vertexs[j*va->num_dimensions+k] < bbox[k].min) bbox[k].min = va->vertexs[j*va->num_dimensions+k];
          if (va->vertexs[j*va->num_dimensions+k] > bbox[k].max) bbox[k].max = va->vertexs[j*va->num_dimensions+k];
        }
      }
    }
    free_shape(shape);
  }
  
  fprintf(pipe_err, "num_dimensions %d\n", num_dimensions);
  long i;
  for (i = 0 ; i < num_dimensions ; i++)
    fprintf(pipe_err, "  %ld: %f to %f\n", i, bbox[i].min, bbox[i].max);
  
  return EXIT_SUCCESS;
}
