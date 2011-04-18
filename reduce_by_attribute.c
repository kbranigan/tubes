
#include <stdio.h>
#include <stdlib.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION reduce_by_attribute
#include "scheme.h"

int reduce_by_attribute(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char * name = argc > 1 ? argv[1] : "";
  char * value = argc > 2 ? argv[2] : "";
  
  long shapes_read = 0;
  long shapes_write = 0;
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    shapes_read++;
    if (shape->num_attributes == 0) continue;
    
    int i;
    for (i = 0 ; i < shape->num_attributes ; i++)
    {
      struct Attribute * attribute = &shape->attributes[i];
      if (strcmp(attribute->name, name)==0 && strcmp(attribute->value, value)==0)
      {
        shapes_write++;
        write_shape(pipe_out, shape);
      }
    }
    
    free_shape(shape);
  }
  
  fprintf(stderr, "%s: %ld shapes reduced to %ld\n", argv[0], shapes_read, shapes_write);
}
