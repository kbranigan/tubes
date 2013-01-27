
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION normalize
#include "scheme.h"

/*  
normalize each dimension to [0,1]
*/

int normalize(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  //char filename[300] = "";
  //int num_attributes = -1;
  //int c;
  //while ((c = getopt(argc, argv, "f:a:")) != -1)
  //switch (c)
  //{
  //  case 'f':
  //    strncpy(filename, optarg, 300);
  //    break;
  //  case 'a':
  //    num_attributes = atoi(optarg);
  //    break;
  //  default:
  //    abort();
  //}
  
  int j,k;
  
  int bbox_num_dimensions = 0;
  struct minmax {
    float min;
    float max;
  };
  struct minmax * bbox = NULL;
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    struct VertexArray * va = get_or_add_array(shape, GL_VERTEX_ARRAY);
    
    // alloc bbox
    if (va->num_dimensions > bbox_num_dimensions)
    {
      bbox = (struct minmax*)realloc(bbox, sizeof(struct minmax)*va->num_dimensions);
      for (j = bbox_num_dimensions ; j < va->num_dimensions ; j++)
      {
        bbox[j].min =  FLT_MAX;
        bbox[j].max = -FLT_MAX;
      }
      bbox_num_dimensions = va->num_dimensions;
    }
    
    // get bbox
    for (j = 0 ; j < shape->num_vertexs ; j++)
    {
      float * v = get_vertex(shape, 0, j);
      
      for (k = 0 ; k < va->num_dimensions ; k++)
      {
        if (v[k] < bbox[k].min) bbox[k].min = v[k];
        if (v[k] > bbox[k].max) bbox[k].max = v[k];
      }
    }
    
    // normalize
    for (j = 0 ; j < shape->num_vertexs ; j++)
    {
      float * v = get_vertex(shape, 0, j);
      for (k = 0 ; k < va->num_dimensions ; k++)
        v[k] = (v[k] - bbox[k].min) / (bbox[k].max - bbox[k].min);
    }
    
    write_shape(pipe_out, shape);
    free_shape(shape);
    
    free(bbox);
    bbox_num_dimensions = 0;
  }
  
}
