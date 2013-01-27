
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION add_random_colors
#include "scheme.h"

void set_random_color(float * v)
{
  v[0] = rand() / (float)RAND_MAX;// / 2.0 + 0.2;
  v[1] = rand() / (float)RAND_MAX;// / 2.0 + 0.2;
  v[2] = rand() / (float)RAND_MAX;// / 2.0 + 0.5;
  v[3] = rand() / (float)RAND_MAX / 4.0 + 0.5;
}

static int verbose_flag;

int add_random_colors(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  /*int c;
  while (1)
  {
    static struct option long_options[] =
    {
      // These options set a flag.
      {"verbose", no_argument,       &verbose_flag, 1},
      {"brief",   no_argument,       &verbose_flag, 0},
      // These options don't set a flag.
      //  We distinguish them by their indices.
      {"add",     no_argument,       0, 'a'},
      {"append",  no_argument,       0, 'b'},
      {"delete",  required_argument, 0, 'd'},
      {"create",  required_argument, 0, 'c'},
      {"file",    required_argument, 0, 'f'},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    
    if ((c = getopt_long (argc, argv, "abc:d:f:", long_options, &option_index)) == -1) break;
    
    switch (c)
    {
      
    }
  }*/
  
  int num_dimensions = 4;
  int per_shape_or_vertex = 0;
  int c;
  while ((c = getopt(argc, argv, "d:p:")) != -1)
  switch (c)
  {
    case 'd':
      num_dimensions = clamp_int(atoi(optarg), 3, 4); // with / without alpha
      break;
    case 'p':
      if (strcmp(optarg, "shape")==0) per_shape_or_vertex = 0;
      else if (strcmp(optarg, "vertex")==0) per_shape_or_vertex = 1;
      else {
        fprintf(pipe_err, "-p [shape|vertex] ('%s' is invalid)\n", optarg);
        exit(1);
      }
      break;
    default:
      abort();
  }
  srand(time(NULL));
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    if (shape->num_vertex_arrays == 1 && shape->vertex_arrays[0].array_type == GL_VERTEX_ARRAY)
    {
      struct VertexArray * cva = get_or_add_array(shape, GL_COLOR_ARRAY);
      cva->num_dimensions = num_dimensions;
      cva->vertexs = realloc(cva->vertexs, sizeof(float)*shape->num_vertexs*cva->num_dimensions);

      long i;
      float color[4];
      set_random_color(color);
      
      for (i = 0 ; i < shape->num_vertexs ; i++)
      {
        if (per_shape_or_vertex == 1) set_random_color(color);
        set_vertex(shape, 1, i, color);
      }
    }
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
}
