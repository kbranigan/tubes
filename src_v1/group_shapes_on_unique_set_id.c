
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION group_shapes_on_unique_set_id
#include "scheme.h"

int group_shapes_on_unique_set_id(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  int num_shapes = 0;
  struct Shape ** shapes = NULL;
  struct Shape * shape = NULL;
  
  while ((shape = read_shape(pipe_in)))
  {
    num_shapes++;
    shapes = (struct Shape**)realloc(shapes, sizeof(struct Shape*)*num_shapes);
    shapes[num_shapes-1] = shape;
  }
  
  fprintf(stderr, "num_shapes = %d\n", num_shapes);
  
  int num_unique_set_ids = 0;
  int * unique_set_ids = NULL;
  long i,j;
  for (i = 0 ; i < num_shapes ; i++)
  {
    shape = shapes[i];
    int found = 0;
    for (j = 0 ; j < num_unique_set_ids ; j++)
    {
      if (unique_set_ids[j] == shape->unique_set_id)
      {
        found = 1;
        break;
      }
    }
    if (found == 0)
    {
      num_unique_set_ids++;
      unique_set_ids = (int*)realloc(unique_set_ids, sizeof(int)*num_unique_set_ids);
      unique_set_ids[num_unique_set_ids-1] = shape->unique_set_id;
    }
  }
  
  fprintf(stderr, "num_unique_set_ids = %d\n", num_unique_set_ids);
  
  for (j = 0 ; j < num_unique_set_ids ; j++)
  {
    int new_num_vertexs = 0;
    for (i = 0 ; i < num_shapes ; i++)
    {
      if (shapes[i]->unique_set_id == unique_set_ids[j])
      {
        shape = shapes[i];
        new_num_vertexs += shape->num_vertexs;
      }
    }
    //fprintf(stderr, "unique_set_id: %d has %d num_vertexs\n", unique_set_ids[j], num_vertexs);
    
    if (shape->num_vertex_arrays != 1) fprintf(stderr, "shape->num_vertex_arrays = %d (thats bad for grouping)\n", shape->num_vertex_arrays);
    
    struct Shape * nshape = (struct Shape*)malloc(sizeof(struct Shape));
    nshape->unique_set_id = unique_set_ids[j];
    nshape->num_attributes = 0;
    nshape->gl_type = shape->gl_type;
    nshape->num_vertexs = new_num_vertexs;
    nshape->num_vertex_arrays = shape->num_vertex_arrays;
    nshape->vertex_arrays = (struct VertexArray*)malloc(sizeof(struct VertexArray)*nshape->num_vertex_arrays);
    
    struct VertexArray * nva = &nshape->vertex_arrays[0];
    nva->array_type = shape->vertex_arrays[0].array_type;
    nva->num_dimensions = shape->vertex_arrays[0].num_dimensions;
    nva->vertexs = (float*)malloc(sizeof(float)*nva->num_dimensions*nshape->num_vertexs);
    
    new_num_vertexs = 0;
    for (i = 0 ; i < num_shapes ; i++)
    {
      if (shapes[i]->unique_set_id == unique_set_ids[j])
      {
        shape = shapes[i];
        struct VertexArray * ova = &shape->vertex_arrays[0];
        memcpy(&nva->vertexs[ova->num_dimensions*new_num_vertexs], ova->vertexs, sizeof(float)*ova->num_dimensions*shape->num_vertexs);
        new_num_vertexs += shape->num_vertexs;
      }
    }
    
    write_shape(pipe_out, nshape);
  }
  
  for (i = 0 ; i < num_shapes ; i++)
    free_shape(shapes[i]);
  free(shapes);
}
