
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION add_color_from_source_interpolation
#include "scheme.h"

int add_color_from_source_interpolation(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char filename[300] = "";
  int num_attributes = -1;
  int c;
  while ((c = getopt(argc, argv, "f:a:")) != -1)
  switch (c)
  {
    case 'f': strncpy(filename, optarg, 300); break;
    case 'a': num_attributes = atoi(optarg); break;
    default: abort();
  }
  
  if (filename[0] == 0) { fprintf(stderr, "%s: ERROR, -f [filename] required.\n", argv[0]); return EXIT_FAILURE; }
  
  FILE * fp = fopen(filename, "r");
  if (fp == NULL) { fprintf(stderr, "%s: ERROR, -f '%s' is not a file or does not exist.\n", argv[0], filename); return EXIT_FAILURE; }
  struct Shape * source_shape = read_shape(fp);
  fclose(fp);
  
  if (source_shape->num_vertex_arrays != 1) { fprintf(stderr, "%s: ERROR, source shape has %d vertex_arrays (should be only 1)\n", argv[0], source_shape->num_vertex_arrays); return EXIT_FAILURE; }
  if (source_shape->num_vertexs == 0) { fprintf(stderr, "%s: ERROR, source shape has no vertexs\n", argv[0]); return EXIT_FAILURE; }
  if (source_shape->vertex_arrays[0].num_dimensions != 1) { fprintf(stderr, "%s: ERROR, source shape has %d dimensions, supports only 1\n", argv[0], source_shape->vertex_arrays[0].num_dimensions); return EXIT_FAILURE; }
  
  //write_shape(pipe_out, source_shape);
  struct BBox * source_bbox = get_bbox(source_shape, NULL);
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    if (shape->vertex_arrays[0].num_dimensions != 1) { fprintf(stderr, "%s: ERROR, shape has %d dimensions, supports only 1\n", argv[0], shape->vertex_arrays[0].num_dimensions); return EXIT_FAILURE; }
    
    struct VertexArray * cva = get_or_add_array(shape, GL_COLOR_ARRAY);
    set_num_dimensions(shape, 1, 1); // only need one dimension for this data
    
    struct BBox * bbox = get_bbox(shape, NULL);
    
    int i;
    for (i = 0 ; i < shape->num_vertexs ; i++)
    {
      float j = i / (float)shape->num_vertexs * (float)source_shape->num_vertexs;
      
      float * v = get_vertex(shape, 0, i);
      float * cv = get_vertex(shape, 1, i);
      
      float * s1 = get_vertex(source_shape, 0, (int)floor(j));
      //float * s2 = ((int)ceil(j) == shape->num_vertexs) ? s1 : get_vertex(source_shape, 0, (int)ceil(j));
      
      // kbfu, should table s2 into account, would produce a better result
      
      float value = (s1[0] - bbox->minmax[0].min) / (bbox->minmax[0].max - bbox->minmax[0].min) + bbox->minmax[0].min;
      
      //fprintf(stderr, "s%d[0] = %f s%d[0] = %f\n", (int)floor(j), s1[0], (int)ceil(j), s2[0]);
      
      cv[0] = value; // i / (float)shape->num_vertexs;
      //cv[1] = 0.0; // i / (float)shape->num_vertexs;
      //cv[2] = 1.0 - value; // i / (float)shape->num_vertexs;
      
      //if (i > 10) break;
    }
    
    // manipulate data here if you like
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
  free_shape(source_shape);
  free_bbox(source_bbox);
}
