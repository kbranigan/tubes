
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>   // for getopt_long

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION add_color_from_numberic_attribute
#include "scheme.h"

int add_color_from_numberic_attribute(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char attribute[300] = "";
  float min = -1;
  float max = -1;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"min", required_argument, 0, 'm'},
      {"max", required_argument, 0, 'n'},
      {"attribute", required_argument, 0, 'a'},
      //{"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "m:n:a:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'm': min = atof(optarg); break;
      case 'n': max = atof(optarg); break;
      case 'a': strncpy(attribute, optarg, sizeof(attribute)); break;
      default: abort();
    }
  }
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    struct VertexArray * cva = get_or_add_array(shape, GL_COLOR_ARRAY);
    
    int i;
    for (i = 0 ; i < shape->num_vertexs ; i++)
    {
      char * value_c = get_attribute(shape, attribute);
      if (!value_c) continue;
      
      float value = atof(value_c);
      float value_in_range = value / (max - min);
      
      float * v = get_vertex(shape, 0, i);
      float * cv = get_vertex(shape, 1, i);
      
      int d;
      for (d = 0 ; d < shape->vertex_arrays[1].num_dimensions ; d++)
      {
        cv[d] = value_in_range;
      }
    }
    
    // manipulate data here if you like
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
}
