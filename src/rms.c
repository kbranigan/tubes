
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION rms
#include "scheme.h"

/*

rms stands for Root Mean Square, this is intended for processing of read_soundwave output

*/

int rms(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  //char filename[300] = "";
  //int num_attributes = -1;
  
  int num_rms_vertexs = 0;
  int num_rms_samples_per_vertex = 0;
  int c;
  while ((c = getopt(argc, argv, "s:v:")) != -1)
  switch (c)
  {
    case 's':
      num_rms_samples_per_vertex = atoi(optarg); // result shape will be of unknown length, x number of samples used for each result vertex
      break;
    case 'v':
      num_rms_vertexs = atoi(optarg); // result shape should have x number of vertexs
      break;
    default:
      fprintf(stderr, "%s ERROR: invalid arguments\n", argv[0]);
      abort();
  }
  
  if (num_rms_vertexs == 0 && num_rms_samples_per_vertex == 0) num_rms_vertexs = 1000;
  else if (num_rms_vertexs != 0 && num_rms_samples_per_vertex != 0)
  {
    fprintf(stderr, "%s: cannot specify number of vertexs and number of samples per vertex (defaulting to 1000 vertexs)\n", argv[0]);
    num_rms_vertexs = 1000;
    num_rms_samples_per_vertex = 0;
  }
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    if (num_rms_samples_per_vertex == 0)
      num_rms_samples_per_vertex = ceil(shape->num_vertexs / num_rms_vertexs);
    
    //fprintf(stderr, "num_rms_samples_per_vertex = %d\n", num_rms_samples_per_vertex);
    //fprintf(stderr, "shape->num_vertexs = %d\n", shape->num_vertexs);
    //fprintf(stderr, "num_rms_samples_per_vertex = %d\n", num_rms_samples_per_vertex);
    //fprintf(stderr, "num_rms_vertexs = %d\n", num_rms_vertexs);
    
    struct Shape * rms_shape = new_shape();
    rms_shape->gl_type = shape->gl_type;
    set_num_vertexs(rms_shape, num_rms_vertexs);
    set_num_dimensions(rms_shape, 0, shape->vertex_arrays[0].num_dimensions);
    
    char value[50];
    sprintf(value, "%d", num_rms_samples_per_vertex);
    set_attribute(rms_shape, "samples per vertex", value);
    
    int i = 0, j = 0, k = 0;
    
    for (i = 0 ; i < num_rms_vertexs ; i++)
    {
      float rms_v[10];
      memset(rms_v, 0, sizeof(rms_v));
      
      for (j = 0 ; j < num_rms_samples_per_vertex ; j++)
      {
        if (i * num_rms_samples_per_vertex + j >= shape->num_vertexs) break;
        
        float * v = get_vertex(shape, 0, i * num_rms_samples_per_vertex + j);
        for (k = 0 ; k < shape->vertex_arrays[0].num_dimensions ; k++)
          rms_v[k] += v[k] * v[k];
      }
      for (k = 0 ; k < shape->vertex_arrays[0].num_dimensions ; k++)
        rms_v[k] = sqrt(rms_v[k] * (1.0 / num_rms_vertexs));
      
      set_vertex(rms_shape, 0, i, rms_v);
    }
    
    write_shape(pipe_out, rms_shape);
    free_shape(shape);
  }
}
