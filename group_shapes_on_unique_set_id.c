
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheme.h"

int main(int argc, char ** argv)
{
  if (!stdin_is_piped())
  {
    fprintf(stderr, "%s needs a data source. (redirected pipe, using |)\n", argv[0]);
    exit(1);
  }
  
  if (!stdout_is_piped())
  {
    fprintf(stderr, "%s outputs binary content. Pipe it to something that can read it.\n", argv[0]);
    //exit(1);
  }
  
  if (!read_header(stdin, CURRENT_VERSION)) { fprintf(stderr, "read header failed.\n"); exit(1); }
  if (!write_header(stdout, CURRENT_VERSION)) { fprintf(stderr, "write header failed.\n"); exit(1); }
  
  int num_shapes = 0;
  struct Shape ** shapes = NULL;
  struct Shape * shape = NULL;
  
  while ((shape = read_shape(stdin)))
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
    
    write_shape(stdout, nshape);
  }
  
  for (i = 0 ; i < num_shapes ; i++)
    free_shape(shapes[i]);
  free(shapes);
  
  /*struct Shape * prev_shape = NULL;
  struct Shape * shape = NULL;
  while ((shape = read_shape(stdin)))
  {
    long i, j, k, l;
    if (prev_shape != NULL && prev_shape->unique_set_id == shape->unique_set_id)
    {
      if (prev_shape->gl_type != shape->gl_type) { fprintf(stderr, "tryed to group two shapes with a different gl_type\n"); exit(1); }
      if (prev_shape->num_vertex_arrays != shape->num_vertex_arrays) { fprintf(stderr, "tryed to group two shapes with a different number of vertex arrays\n"); exit(1); }
      
      //fprintf(stderr, "merge %d %d\n", shape->num_vertexs, prev_shape->num_vertexs);
      for (i = 0 ; i < shape->num_vertex_arrays ; i++)
      {
        struct VertexArray * pva = &prev_shape->vertex_arrays[i];
        struct VertexArray * va = &shape->vertex_arrays[i];
        
        if (pva->array_type != va->array_type) { fprintf(stderr, "trying to group two vertex arrays with a different array type\n"); exit(1); }
        if (pva->num_dimensions != va->num_dimensions) { fprintf(stderr, "trying to group two vertex arrays with a different number of dimensions\n"); exit(1); }
        
        va->vertexs = (float*)realloc(va->vertexs, sizeof(float)*va->num_dimensions*(shape->num_vertexs + prev_shape->num_vertexs));
        memcpy(&va->vertexs[va->num_dimensions*shape->num_vertexs], pva->vertexs, sizeof(float)*pva->num_dimensions*prev_shape->num_vertexs);
        shape->num_vertexs += prev_shape->num_vertexs;
      }
    }
    else if (prev_shape != NULL)
    {
      write_shape(stdout, prev_shape);
      free_shape(prev_shape);
      //fprintf(stderr, "write\n");
    }
    prev_shape = shape;
  }
  
  if (prev_shape != NULL)
  {
    write_shape(stdout, prev_shape);
  }*/
  
}
