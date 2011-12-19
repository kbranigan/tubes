
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int reduce_by_overlap(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err);

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION reduce_by_overlap
#include "scheme.h"

#include "2dtree.hpp"

int reduce_by_overlap(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  unsigned int num_shapes = 0;
  struct Shape ** shapes = read_all_shapes(pipe_in, &num_shapes);
  
  void * kdtree = kdtree_new();
  void ** kdtrees = (void**)malloc(sizeof(void*)*num_shapes);
  
  struct Shape * nearest_shape = NULL;
  int nearest_vertex_id = 0;
  float nearest_distance = 0;
  
  int i, j;
  struct Shape * shape = NULL;
  for (i = 0 ; i < num_shapes ; i++)
  {
    shape = shapes[i];
    //kdtree_insert_shape(kdtree, shape);
    if (shape->num_vertexs > 1)
    {
      kdtrees[i] = kdtree_new();
      kdtree_insert_shape(kdtrees[i], shape);
    }
    //write_shape(pipe_out, shape);
  }
  for (i = 0 ; i < num_shapes ; i++)
  {
    float * s1 = get_vertex(shapes[i], 0, 0);
    float * e1 = get_vertex(shapes[i], 0, shapes[i]->num_vertexs-1);
    
    //fprintf(stderr, "%f,%f to %f,%f\n", s1[0], s1[1], e1[0], e1[1]);
    
    for (j = 0 ; j < num_shapes ; j++)
    {
      if (i == j) continue;
      
      float * s2 = get_vertex(shapes[j], 0, 0);
      float * e2 = get_vertex(shapes[j], 0, shapes[j]->num_vertexs-1);
      
      //fprintf(stderr, "shape %d to %d\n", i, j);
      //fprintf(stderr, "%f,%f to %f,%f\n", s2[0], s2[1], e2[0], e2[1]);
      
      nearest_vertex_id = 0;
      nearest_distance = 0;
      
      kdtree_find_nearest(kdtrees[j], s1[0], s1[1], NULL, &nearest_vertex_id, &nearest_distance);
      if (nearest_distance < 0.001)
      {
        float * v = get_vertex(shapes[j], 0, nearest_vertex_id);
        /*
        fprintf(stderr, "nearest_vertex_id = %d\n", nearest_vertex_id);
        
        float dx = v[0]-s1[0];
        float dy = v[1]-s1[1];
        
        fprintf(stderr, " sqrt = %f\n", sqrt(dx*dx + dy*dy));
        
        fprintf(stderr, "  %f,%f\n", s1[0], s1[1]);
        fprintf(stderr, "  %f,%f\n", v[0], v[1]);
        */
        fprintf(stderr, "nearest_vertex_id = %d\n", nearest_vertex_id);
        set_num_vertexs(shapes[j], nearest_vertex_id+1);
        
        struct Shape * shape_new = new_shape();
        shape_new->gl_type = GL_LINES;
        get_or_add_array(shape_new, GL_COLOR_ARRAY);
        
        float cv[4] = { 1,0,0,0 };
        append_vertex2(shape_new, s1, cv);
        append_vertex2(shape_new, v, cv);
        write_shape(pipe_out, shape_new);
        free_shape(shape_new);
        
        //fprintf(stderr, "start, %d/%d, %f\n", nearest_vertex_id, shapes[j]->num_vertexs, nearest_distance);
        //float * t = get_vertex(shapes[j], 0, nearest_vertex_id);
        //fprintf(stderr, "%f,%f\n", t[1], t[0]);
      }
      
      //nearest_shape = NULL;
      //nearest_vertex_id = 0;
      //nearest_distance = 0;
      
      //kdtree_find_nearest(kdtrees[j], e2[0], e2[1], &nearest_shape, &nearest_vertex_id, &nearest_distance);
      //if (nearest_distance < 0.001)
      //{
      //  fprintf(stderr, "end, %d/%d, %f\n", nearest_vertex_id, shapes[i]->num_vertexs, nearest_distance);
      //}
      
      //fprintf(stderr, "\n\n");
    }
  }
  
  for (i = 0 ; i < num_shapes ; i++)
  {
    write_shape(pipe_out, shapes[i]);
    //break;
  }
  
  free_all_shapes(shapes, num_shapes);
}
