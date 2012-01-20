
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDIN_IS_PIPED
#define SCHEME_FUNCTION write_json
#include "scheme.h"

int write_json(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char file_name[300] = "";
  int add_whitespace = 0;
  int c;
  while ((c = getopt(argc, argv, "f:w")) != -1)
  switch (c)
  {
    case 'f': strncpy(file_name, optarg, 300); break;
    case 'w': add_whitespace = 1; break;
    default: abort();
  }
  
  if (file_name[0] == 0 && argc == 2 && argv[1] != NULL && argv[1][0] != '-')
    strncpy(file_name, argv[1], sizeof(file_name));
  
  //char gl_types_c[8][20] = {"GL_POINTS", "GL_LINES", "GL_LINE_LOOP", "GL_LINE_STRIP", "GL_TRIANGLES", "GL_TRIANGLE_STRIP", "GL_TRIANGLE_FAN"};
  
  FILE * fp = NULL;
  if (file_name[0] != 0)
  {
    fp = fopen(file_name, "w");
    if (fp == NULL)
      fprintf(stderr, "%s: ERROR: could not write to '%s'\n", argv[0], file_name);
  }
  
  if (fp == NULL) fp = pipe_out;
  
  int num_shapes = 0;
  fprintf(fp, "[");
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    if (num_shapes != 0) fprintf(fp, ", ");
    
    fprintf(fp, "{%s", (add_whitespace ? "\n" : ""));
    fprintf(fp, "%s\"unique_set_id\":\"%d\",%s", (add_whitespace ? "  " : ""), shape->unique_set_id, (add_whitespace ? "\n" : ""));
    if (shape->num_attributes > 0)
    {
      fprintf(fp, "%s\"attributes\":{%s", (add_whitespace ? "  " : ""), (add_whitespace ? "\n" : ""));
      int i;
      for (i = 0 ; i < shape->num_attributes ; i++)
      {
        struct Attribute * attribute = &shape->attributes[i];
        fprintf(fp, "%s\"%s\":\"%s\"%s%s", (add_whitespace ? "    " : ""), attribute->name, attribute->value, (i == shape->num_attributes-1)?"":",", (add_whitespace ? "\n" : ""));
      }
      fprintf(fp, "%s},%s", (add_whitespace ? "  " : ""), (add_whitespace ? "\n" : ""));
    }
    if (shape->num_vertex_arrays > 0 || shape->num_vertexs > 0)
    {
      fprintf(fp, "%s\"vertex_arrays\":[", (add_whitespace ? "  " : ""));
      int i;
      for (i = 0 ; i < shape->num_vertex_arrays ; i++)
      {
        struct VertexArray * va = &shape->vertex_arrays[i];
        fprintf(fp, "%s%s{%s", (i!=0?",":""), ((i!=0&&add_whitespace)?"\n":""), (add_whitespace ? "\n" : ""));
        fprintf(fp, "%s\"array_type\":\"%d\",%s", (add_whitespace ? "    " : ""), va->array_type, (add_whitespace ? "\n" : ""));
        fprintf(fp, "%s\"num_dimensions\":\"%d\",%s", (add_whitespace ? "    " : ""), va->num_dimensions, (add_whitespace ? "\n" : ""));
        fprintf(fp, "%s\"vertexs\":[", (add_whitespace ? "    " : ""));
        int j;
        for (j = 0 ; j < shape->num_vertexs ; j++)
        {
          float * v = get_vertex(shape, i, j);
          if (va->num_dimensions == 1) fprintf(fp, "%f%s", v[0], j==shape->num_vertexs-1?"":",");
          else if (va->num_dimensions == 2) fprintf(fp, "[%f,%f]%s", v[0], v[1], j==shape->num_vertexs-1?"":",");
          else if (va->num_dimensions == 3) fprintf(fp, "[%f,%f,%f]%s", v[0], v[1], v[2], j==shape->num_vertexs-1?"":",");
          else if (va->num_dimensions == 4) fprintf(fp, "[%f,%f,%f,%f]%s", v[0], v[1], v[2], v[3], j==shape->num_vertexs-1?"":",");
        }
        fprintf(fp, "]%s", (add_whitespace ? "\n" : ""));
        fprintf(fp, "%s}", (add_whitespace ? "  " : ""));
      }
      fprintf(fp, "],%s", (add_whitespace ? "\n" : ""));
    }
    fprintf(fp, "%s\"gl_type\":\"%s\"%s", (add_whitespace ? "  " : ""), get_gl_type_name(shape->gl_type), (add_whitespace ? "\n" : ""));
    free_shape(shape);
    fprintf(fp, "}");
    num_shapes++;
  }
  fprintf(fp, "]%s", (add_whitespace ? "\n" : ""));
  
  if (fp != pipe_out) fclose(fp);
}