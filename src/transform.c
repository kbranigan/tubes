
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION transform
#include "scheme.h"

int transform(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  float * offsets = NULL;
  int num_offsets = 0;
  
  float * scales = NULL;
  int num_scales = 0;
  
  int array_index = 0; // will operate only on a single array, default the first
  
  int i;
  while ((i = getopt(argc, argv, "a:o:s:")) != -1)
  switch (i)
  {
    case 'a':
      array_index = atoi(optarg);
      break;
    case 'o':
    {
      char * pch = strtok(optarg, ",");
      while (pch != NULL)
      {
        num_offsets++;
        offsets = realloc(offsets, sizeof(float)*num_offsets);
        offsets[num_offsets-1] = atof(pch);
        pch = strtok(NULL, ",");
      }
      break;
    }
    case 's':
    {
      char * pch = strtok(optarg, ",");
      while (pch != NULL)
      {
        num_scales++;
        scales = realloc(scales, sizeof(float)*num_scales);
        scales[num_scales-1] = atof(pch);
        pch = strtok(NULL, ",");
      }
      break;
    }
    default:
      abort();
  }
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    int j;
    float * v;
    if (array_index < shape->num_vertex_arrays && array_index >= 0)
    {
      struct VertexArray * va = &shape->vertex_arrays[array_index];
      for (i = 0 ; i < shape->num_vertexs ; i++)
      {
        v = get_vertex(shape, 0, i);
        for (j = 0 ; j < va->num_dimensions && j < num_offsets ; j++)
          v[j] += offsets[j];
        for (j = 0 ; j < va->num_dimensions && j < num_scales ; j++)
          v[j] *= scales[j];
      }
    }
    
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
  
  free(scales);
  free(offsets);
}
