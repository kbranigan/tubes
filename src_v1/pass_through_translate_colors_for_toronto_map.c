
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
//#include <getopt.h>   // for getopt_long

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION pass_through
#include "scheme.h"

int pass_through(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  //char filename[300] = "";
  //int num_attributes = -1;
  //int c;
  //while (1)
  //{
  //  static struct option long_options[] = {
  //    //{"row_id", required_argument, 0, 'r'},
  //    //{"part_id", required_argument, 0, 'p'},
  //    {"filename", required_argument, 0, 'f'},
  //    {"debug", no_argument, &debug, 1},
  //    {0, 0, 0, 0}
  //  };
  //  
  //  int option_index = 0;
  //  c = getopt_long(argc, argv, "f:", long_options, &option_index);
  //  if (c == -1) break;
  //  
  //  switch (c)
  //  {
  //    case 0: break;
  //    //case 'r': row_id = atoi(optarg); break;
  //    //case 'p': part_id = atoi(optarg); break;
  //    case 'f': strncpy(filename, optarg, sizeof(filename)); break;
  //    default: abort();
  //  }
  //}
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    if (shape->num_vertex_arrays > 1 && shape->vertex_arrays[1].array_type == GL_COLOR_ARRAY)
    {
      if (shape->vertex_arrays[1].num_dimensions >= 3)
      {
        int i;
        for (i = 0 ; i < shape->num_vertexs ; i++)
        {
          float * cv = get_vertex(shape, 1, i);
          
          cv[1] = cv[2] = 1.0 - cv[0]*2.0;
          if (cv[1] > 1.0) cv[1] = cv[2] = 1.0;
          if (cv[0] < cv[1]) cv[0] = cv[1];// - cv[0];
          else
          {
            cv[2] += (cv[0]-cv[2])*0.4;
            cv[1] += (cv[0]-cv[1])*0.4;
          }
          
          // turn it blue, instead of red
          float ftemp = cv[2];
          cv[2] = cv[0];
          cv[0] = ftemp;
          
          //cv[0] = cv[0] - 0.4;
          //cv[1] = cv[1] - 0.4;
          
          //cv[0] = 1.0;
        }
      }
      else
        fprintf(stderr, "ERROR: only supports grayscaling 3 or 4 dimensional colors\n");
    }
    
    // manipulate data here if you like
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
}
