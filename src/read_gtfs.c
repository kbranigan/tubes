
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDOUT_IS_PIPED
#define SCHEME_FUNCTION read_gtfs
#include "scheme.h"

this file isn't finished and doesn't do anything yet - this is so it won't compile.

int read_gtfs(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char * filename = NULL;
  int c;
  while ((c = getopt(argc, argv, "f:")) != -1)
  switch (c)
  {
    case 'f':
      filename = malloc(strlen(optarg)+1);
      strcpy(filename, optarg);
      break;
  }
  
  FILE * fp = filename == NULL ? pipe_in : fopen(filename, "r");
  
  if (fp == NULL)
  {
    fprintf(pipe_err, "ERROR: Usage: %s -f [filename.csv]\n", argv[0]);
    return -1;
  }
  
  char line[1000];
  struct Shape * shape = NULL;
  
  int prev_unique_set_id = -1;
  while (fgets(line, sizeof(line), fp))
  {
    char * x = strtok (line, " ,");
    if (x == NULL) continue;
    char * y = strtok (NULL, " ,");
    if (y == NULL) continue;
    char * unique_set_id = strtok (NULL, " ,\n");
    if (unique_set_id == NULL) continue;

    if (shape == NULL || prev_unique_set_id != atoi(unique_set_id))
    {
      if (shape != NULL)
      {
        write_shape(pipe_out, shape);
        free_shape(shape);
      } 
      shape = new_shape();
      shape->gl_type = GL_POINTS;
      shape->unique_set_id = atoi(unique_set_id);
    }
    else
    {
      shape->gl_type = GL_LINE_STRIP;
    }
    
    float v[3] = { atof(x), atof(y), 0.0 };
    
    append_vertex(shape, v);
    prev_unique_set_id = shape->unique_set_id;
  }
  if (shape != NULL)
  {
    write_shape(pipe_out, shape);
    free_shape(shape);
  } 
  fprintf(stderr, "done\n");
}
