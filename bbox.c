
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include "scheme.h"

int main(int argc, char *argv[])
{
  double b[3][2] = {
    {10000000, -10000000},
    {10000000, -10000000},
    {10000000, -10000000}
  };
  
  if (read_header(stdin, CURRENT_VERSION))
  {
    struct Shape * shape = NULL;
    
    while ((shape = read_shape(stdin)))
    {
      long i, j;
      for (i = 0 ; i < shape->num_vertex_arrays ; i++)
      {
        struct VertexArray * va = &shape->vertex_arrays[i];
        
        if (va->array_type != GL_VERTEX_ARRAY) continue;
        
        //if (va->num_dimensions != 3) fprintf(stderr, "vertex_array has %d dimensions (expected 3)\n", va->num_dimensions);
        if (va->vertexs == NULL) { fprintf(stderr, "vertex array %ld is NULL\n", i); exit(1); }
      
        for (j = 0 ; j < shape->num_vertexs ; j++)
        {
          double x = va->vertexs[j*va->num_dimensions];
          double y = va->vertexs[j*va->num_dimensions+1];
          double z = va->vertexs[j*va->num_dimensions+2];
          
          if (x < b[0][0]) b[0][0] = x; if (x > b[0][1]) b[0][1] = x;
          if (y < b[1][0]) b[1][0] = y; if (y > b[1][1]) b[1][1] = y;
          if (va->num_dimensions >= 3) { if (z < b[2][0]) b[2][0] = z; if (z > b[2][1]) b[2][1] = z; }
        }
      }
      free_shape(shape);
    }
  }
  
  /*
  if (stdout_is_piped())
  {
    uint32_t file_header = 42;
    assert(fwrite(&file_header, sizeof(file_header), 1, stdout) == 1);
    
    //uint32_t num_shapes = 1;
    //assert(fwrite(&num_shapes, sizeof(num_shapes), 1, stdout) == 1);
    
    double inf = 1.0 / 0.0;
    assert(fwrite(&inf, sizeof(inf), 1, stdout) == 1);
    
    uint32_t unique_set_id = 0;
    assert(fwrite(&unique_set_id, sizeof(unique_set_id), 1, stdout) == 1);
    
    char name[150];
    memset(name, 0, sizeof(name));
    assert(fwrite(name, sizeof(name), 1, stdout) == 1);
    
    uint32_t frame_type = GL_LINE_LOOP;
    assert(fwrite(&frame_type, sizeof(frame_type), 1, stdout) == 1);
    
    uint32_t vertex_type = GL_VERTEX_ARRAY;
    assert(fwrite(&vertex_type, sizeof(vertex_type), 1, stdout) == 1);
    
    uint32_t number_of_vertexes = 4;
    assert(fwrite(&number_of_vertexes, sizeof(number_of_vertexes), 1, stdout) == 1);
    
    assert(fwrite(&b[0][0], sizeof(double), 1, stdout) == 1);
    assert(fwrite(&b[1][0], sizeof(double), 1, stdout) == 1);
    assert(fwrite(&b[2][0], sizeof(double), 1, stdout) == 1);
    
    assert(fwrite(&b[0][0], sizeof(double), 1, stdout) == 1);
    assert(fwrite(&b[1][1], sizeof(double), 1, stdout) == 1);
    assert(fwrite(&b[2][0], sizeof(double), 1, stdout) == 1);
    
    assert(fwrite(&b[0][1], sizeof(double), 1, stdout) == 1);
    assert(fwrite(&b[1][1], sizeof(double), 1, stdout) == 1);
    assert(fwrite(&b[2][0], sizeof(double), 1, stdout) == 1);
    
    assert(fwrite(&b[0][1], sizeof(double), 1, stdout) == 1);
    assert(fwrite(&b[1][0], sizeof(double), 1, stdout) == 1);
    assert(fwrite(&b[2][0], sizeof(double), 1, stdout) == 1);
  }
  else*/
    printf("{\n  'bbox': {\n    'x': [%f, %f],\n    'y': [%f, %f],\n    'z': [%f, %f]\n  }\n}\n", 
                                 b[0][0], b[0][1],   b[1][0], b[1][1],   b[2][0], b[2][1]);
  
  return 0;
}
