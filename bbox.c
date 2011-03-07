
#include <stdio.h>
#include <stdlib.h>

#include "scheme.h"

#ifdef MAIN
int main(int argc, char *argv[])
{
  #ifdef FUNC
  return FUNC(stdin, stdout, stderr);
  #else
  fprintf(stderr, "MAIN is defined but no FUNC was defined\n");
  return EXIT_FAILURE;
  #endif
}
#endif

int bbox(FILE * fin, FILE * fout, FILE * ferr)
{
  if (read_header(stdin, CURRENT_VERSION))
  {
    struct Shape * shape = NULL;
    
    int num_dimensions = 0;
    struct minmax {
      float min;
      float max;
    };
    struct minmax *bbox = NULL;
    
    while ((shape = read_shape(fin)))
    {
      long i, j, k;
      for (i = 0 ; i < shape->num_vertex_arrays ; i++)
      {
        struct VertexArray * va = &shape->vertex_arrays[i];
        
        if (va->array_type != GL_VERTEX_ARRAY) continue;
        if (va->vertexs == NULL) { fprintf(ferr, "vertex array %ld is NULL\n", i); return EXIT_FAILURE; }
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
    
    fprintf(ferr, "num_dimensions %d\n", num_dimensions);
    long i;
    for (i = 0 ; i < num_dimensions ; i++)
      fprintf(ferr, "  %ld: %f to %f\n", i, bbox[i].min, bbox[i].max);
  }
  
  return EXIT_SUCCESS;
}
