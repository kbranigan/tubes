
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDIN_IS_PIPED
#define SCHEME_FUNCTION bbox
#include "scheme.h"

int bbox(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  int each_shape = 0;
  int c;
  while ((c = getopt(argc, argv, "e")) != -1)
  switch (c)
  {
    case 'e': // one bbox for each shape, only works if stdout is piped
      each_shape = 1;
      break;
    default:
      abort();
  }
  
  struct Shape * shape = NULL;
  
  int num_bbox_shapes = 0;
  struct Shape ** bbox_shapes = NULL;
  
  int bbox_num_dimensions = 0;
  struct minmax {
    float min;
    float max;
  };
  struct minmax *bbox = NULL;
  
  while ((shape = read_shape(pipe_in)))
  {
    long i, j, k;
    struct VertexArray * va = get_or_add_array(shape, GL_VERTEX_ARRAY);
    
    
    if (each_shape || va->num_dimensions > bbox_num_dimensions)
    {
      bbox = (struct minmax*)realloc(bbox, sizeof(struct minmax)*va->num_dimensions);
      for (j = (each_shape ? 0 : bbox_num_dimensions) ; j < va->num_dimensions ; j++)
      {
        bbox[j].min =  FLT_MAX;
        bbox[j].max = -FLT_MAX;
      }
      bbox_num_dimensions = va->num_dimensions;
    }
    
    for (j = 0 ; j < shape->num_vertexs ; j++)
    {
      for (k = 0 ; k < va->num_dimensions ; k++)
      {
        if (va->vertexs[j*va->num_dimensions+k] < bbox[k].min) bbox[k].min = va->vertexs[j*va->num_dimensions+k];
        if (va->vertexs[j*va->num_dimensions+k] > bbox[k].max) bbox[k].max = va->vertexs[j*va->num_dimensions+k];
      }
    }
    
    if (each_shape && stdout_is_piped())
    {
      struct Shape * bbox_shape = new_shape();
      
      float bl[2] = { bbox[0].min, bbox[1].min }; append_vertex(bbox_shape, bl);
      float br[2] = { bbox[0].max, bbox[1].min }; append_vertex(bbox_shape, br);
      float tl[2] = { bbox[0].max, bbox[1].max }; append_vertex(bbox_shape, tl);
      float tr[2] = { bbox[0].min, bbox[1].max }; append_vertex(bbox_shape, tr);
      
      write_shape(stdout, bbox_shape);
      free_shape(bbox_shape);
    }
    free_shape(shape);
  }
  
  if (!each_shape && stdout_is_piped())
  {
    if (bbox_num_dimensions != 2) { fprintf(stderr, "ERROR: %s requested to produce a bbox of %d dimensional content\n", argv[0], bbox_num_dimensions); exit(1); }
    
    struct Shape * bbox_shape = new_shape();
    bbox_shape->gl_type = GL_LINE_LOOP;
    
    float bl[2] = { bbox[0].min, bbox[1].min }; append_vertex(bbox_shape, bl);
    float br[2] = { bbox[0].max, bbox[1].min }; append_vertex(bbox_shape, br);
    float tl[2] = { bbox[0].max, bbox[1].max }; append_vertex(bbox_shape, tl);
    float tr[2] = { bbox[0].min, bbox[1].max }; append_vertex(bbox_shape, tr);
    
    write_shape(stdout, bbox_shape);
    free_shape(bbox_shape);
  }
  else
  {
    fprintf(pipe_err, "bbox_num_dimensions %d\n", bbox_num_dimensions);
    long i;
    for (i = 0 ; i < bbox_num_dimensions ; i++)
      fprintf(pipe_err, "  %ld: %f to %f [range: %f]\n", i, bbox[i].min, bbox[i].max, (bbox[i].max - bbox[i].min));
  }
  
  return EXIT_SUCCESS;
}
