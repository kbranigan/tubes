
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "scheme.h"

int main(int argc, char ** argv)
{
  float unique_set_id = (argc == 1) ? 0.5 : atof(argv[1]);
  
  if (!stdin_is_piped())
  {
    fprintf(stderr, "%s needs a data source. (redirected pipe, using |)\n", argv[0]);
    exit(1);
  }
  
  if (!stdout_is_piped())
  {
    fprintf(stderr, "%s outputs binary content. Pipe it to something that can read it.\n", argv[0]);
    exit(1);
  }
  
  if (!read_header(stdin, CURRENT_VERSION)) { fprintf(stderr, "read header failed.\n"); exit(1); }
  if (!write_header(stdout, CURRENT_VERSION)) { fprintf(stderr, "write header failed.\n"); exit(1); }
  
  long shapes_read = 0;
  long shapes_write = 0;
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(stdin)))
  {
    shapes_read++;
    if (shape->unique_set_id == unique_set_id)
    {
      shapes_write++;
      write_shape(stdout, shape);
    }
    
    free_shape(shape);
  }
  
  fprintf(stderr, "%s: %ld shapes reduced to %ld\n", argv[0], shapes_read, shapes_write);
}
