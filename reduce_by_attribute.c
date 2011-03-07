
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "scheme.h"

int main(int argc, char ** argv)
{
  char * name = argc > 1 ? argv[1] : "";
  char * value = argc > 2 ? argv[2] : "";
  
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
    if (shape->num_attributes == 0) continue;
    
    int i;
    for (i = 0 ; i < shape->num_attributes ; i++)
    {
      struct Attribute * attribute = &shape->attributes[i];
      if (strcmp(attribute->name, name)==0 && strcmp(attribute->value, value)==0)
      {
        shapes_write++;
        write_shape(stdout, shape);
      }
    }
    
    free_shape(shape);
  }
  
  fprintf(stderr, "%s: %ld shapes reduced to %ld\n", argv[0], shapes_read, shapes_write);
}
