
#ifndef SCHEME_H
#define SCHEME_H

#include <inttypes.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "vectmath.h"

#define FILE_VERSION_2 2
#define FILE_VERSION_3 3
#define FILE_VERSION_4 4

#define CURRENT_VERSION FILE_VERSION_4

/* include this file before OpenGL headers, that way these defines don't conflict */

#define GL_POINTS          0
#define GL_LINES           1
#define GL_LINE_LOOP       2
#define GL_LINE_STRIP      3
#define GL_TRIANGLES       4
#define GL_TRIANGLE_STRIP  5
#define GL_TRIANGLE_FAN    6

#define GL_VERTEX_ARRAY              32884
#define GL_NORMAL_ARRAY              32885
#define GL_COLOR_ARRAY               32886
#define GL_INDEX_ARRAY               32887
#define GL_TEXTURE_COORD_ARRAY       32888
#define GL_EDGE_FLAG_ARRAY           32889
#define GL_FOG_COORD_ARRAY           33879
#define GL_SECONDARY_COLOR_ARRAY     33886

/*
  c type           name                 values
  //////////////////////////////////////////
  
  [ // shapes (read until feof or fread error)
    float          shape header         INFINITY
    uint32_t       shape version        CURRENT_VERSION
    uint32_t       unique_set_id
    uint32_t       num_attributes
    [ // num_attributes
      char[20]     key
      uint32_t     length               including the \0
      char[length] value
    ]
    uint32_t       gl_type              0 = GL_POINTS
                                        1 = GL_LINES
                                        2 = GL_LINE_LOOP
                                        3 = GL_LINE_STRIP
                                        4 = GL_TRIANGLES
                                        5 = GL_TRIANGLE_STRIP
                                        6 = GL_TRIANGLE_FAN
    uint32_t       num_vertexs
    uint32_t       num_vertex_arrays
    [ // num_vertex_arrays
      uint32_t     array_type           32884 = GL_VERTEX_ARRAY
                                        32886 = GL_COLOR_ARRAY
                                        //32885 = GL_NORMAL_ARRAY
                                        //32887 = GL_INDEX_ARRAY
                                        //32888 = GL_TEXTURE_COORD_ARRAY
                                        //32889 = GL_EDGE_FLAG_ARRAY
                                        //33879 = GL_FOG_COORD_ARRAY
                                        //33886 = GL_SECONDARY_COLOR_ARRAY
      uint32_t     num_dimensions
      [ // num_vertexs
        [ // num_dimensions
          float
        ]
      ]
    ]
  ]
*/

struct VertexArray {
  struct Shape * shape;
  uint32_t array_type;
  uint32_t num_dimensions;
  float * vertexs; /* shape->num_vertexs * num_dimensions */
};

struct Attribute {
  char name[20];
  uint32_t value_length;
  char * value;
};

struct Shape {
  uint32_t unique_set_id;
  uint32_t version;
  
  uint32_t num_attributes;
  struct Attribute * attributes;
  
  uint32_t gl_type;
  
  uint32_t num_vertexs;
  uint32_t num_vertex_arrays;
  struct VertexArray * vertex_arrays;
};

extern void assert_stdin_is_piped();
extern void assert_stdout_is_piped();

extern struct VertexArray* get_or_add_array(struct Shape * shape, int array_type);
extern void append_vertex2f(struct VertexArray * va, float x, float y);
extern void append_vertex3f(struct VertexArray * va, float x, float y, float z);
extern void append_vertex4f(struct VertexArray * va, float x, float y, float z, float w);

extern void set_vertex(struct Shape * shape, int index, float * v);
extern void set_vertex2(struct Shape * shape, int index, float * v1, float * v2);
extern float * get_vertex(struct Shape * shape, int index, int va_index);


extern struct Shape * new_shape();
extern void inspect_shape(FILE * fp, struct Shape * shape);
extern struct Shape * read_shape(FILE * fp);
extern int write_shape(FILE * fp, struct Shape * shape);

/*int point_in_triangle(vec2d A, vec2d B, vec2d C, vec2d P);*/

#ifdef SCHEME_CREATE_MAIN
  int main(int argc, char ** argv)
  {
    #ifdef SCHEME_FUNCTION
      #ifdef SCHEME_ASSERT_STDIN_IS_PIPED
        assert_stdin_is_piped();
      #elif defined(SCHEME_ASSERT_STDOUT_IS_PIPED)
        assert_stdout_is_piped();
      #elif defined(SCHEME_ASSERT_STDINOUT_ARE_PIPED)
        assert_stdin_is_piped();
        assert_stdout_is_piped();
      #else
        #error SCHEME_CREATE_MAIN is defined but no SCHEME_ASSERT_STDIN_IS_PIPED, SCHEME_ASSERT_STDOUT_IS_PIPED or SCHEME_ASSERT_STDINOUT_ARE_PIPED
        fprintf(stderr, "%s: SCHEME_CREATE_MAIN is defined but no SCHEME_ASSERT_STDIN_IS_PIPED, SCHEME_ASSERT_STDOUT_IS_PIPED or SCHEME_ASSERT_STDINOUT_ARE_PIPED\n", argv[0]);
        return EXIT_FAILURE;
      #endif
      int i = 0;
      while (i < argc)
        fprintf(stderr, "%s ", argv[i++]);
      fprintf(stderr, "\n");
      return SCHEME_FUNCTION(argc, argv, stdin, stdout, stderr);
    #else
      #error SCHEME_CREATE_MAIN is defined but no SCHEME_FUNCTION was defined
      fprintf(stderr, "%s: SCHEME_CREATE_MAIN is defined but no SCHEME_FUNCTION was defined\n", argv[0]);
      return EXIT_FAILURE;
    #endif
  }
#endif

#endif
