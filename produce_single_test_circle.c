
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "scheme.h"

int main(int argc, char *argv[])
{
  if (!stdout_is_piped())
  {
    fprintf(stderr, "%s outputs binary content. Pipe it to something that can read it.\n", argv[0]);
    exit(1);
  }
  
  if (!write_header(stdout, CURRENT_VERSION)) exit(1);
  
  struct Shape * shape = (struct Shape*)malloc(sizeof(struct Shape));
  memset(shape, 0, sizeof(struct Shape));
  
  shape->gl_type = GL_LINE_LOOP;
  shape->num_vertexs = 64;
  shape->num_vertex_arrays = 1;
  
  shape->vertex_arrays = (struct VertexArray*)malloc(sizeof(struct VertexArray)*shape->num_vertex_arrays);
  struct VertexArray *va = &shape->vertex_arrays[0];
  memset(va, 0, sizeof(struct VertexArray));
  
  va->shape = shape;
  va->array_type = GL_VERTEX_ARRAY;
  va->num_dimensions = (argc > 1) ? (atoi(argv[1]) == 3 ? 3 : 2) : 2;
  va->vertexs = (double*)malloc(sizeof(double)*va->num_dimensions*shape->num_vertex_arrays);
  
  long i;
  for (i = 0 ; i < shape->num_vertexs ; i++)
  {
    va->vertexs[i*va->num_dimensions+0] = cos(i/(float)shape->num_vertexs*(2*3.14159265));
    va->vertexs[i*va->num_dimensions+1] = sin(i/(float)shape->num_vertexs*(2*3.14159265));
    if (va->num_dimensions == 3) va->vertexs[i*va->num_dimensions+2] = 0.0;
  }
  write_shape(stdout, shape);
  free_shape(shape);
  
  fprintf(stderr, "%s: %d vertexes created\n", argv[0], shape->num_vertexs);
}