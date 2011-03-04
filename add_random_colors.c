
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scheme.h"

int main(int argc, char ** argv)
{
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
  
  srand(time(NULL));
  
  if (!read_header(stdin, CURRENT_VERSION)) { fprintf(stderr, "read header failed.\n"); exit(1); }
  if (!write_header(stdout, CURRENT_VERSION)) { fprintf(stderr, "write header failed.\n"); exit(1); }
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(stdin)))
  {
    if (shape->num_vertex_arrays == 1 && shape->vertex_arrays[0].array_type == GL_VERTEX_ARRAY)
    {
      shape->num_vertex_arrays++;
      shape->vertex_arrays = (struct VertexArray*)realloc(shape->vertex_arrays, sizeof(struct VertexArray)*shape->num_vertex_arrays);
      struct VertexArray * va = &shape->vertex_arrays[shape->num_vertex_arrays-1];
      va->array_type = GL_COLOR_ARRAY;
      va->num_dimensions = 4;
      va->shape = shape;
      va->vertexs = (double*)malloc(sizeof(double)*va->num_dimensions*shape->num_vertexs);
      long i;
      double r = rand() / (float)RAND_MAX / 2.0 + 0.2;
      double g = rand() / (float)RAND_MAX / 2.0 + 0.2;
      double b = rand() / (float)RAND_MAX / 2.0 + 0.5;
      double a = 1.0;//rand() / (float)RAND_MAX / 4.0 + 0.5;
      for (i = 0 ; i < shape->num_vertexs ; i++)
      {
        va->vertexs[i*va->num_dimensions+0] = r;
        va->vertexs[i*va->num_dimensions+1] = g;
        va->vertexs[i*va->num_dimensions+2] = b;
        if (va->num_dimensions == 4) va->vertexs[i*va->num_dimensions+3] = a;
      }
    }
    write_shape(stdout, shape);
  }
}
