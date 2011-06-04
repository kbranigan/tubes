
#ifndef SCHEME_H
#define SCHEME_H

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

extern int stdin_is_piped();
extern int stdin_is_piped_t(float timeout);
extern void assert_stdin_is_piped();
extern void assert_stdin_is_piped_t(float timeout);

extern int stdout_is_piped();
extern void assert_stdout_is_piped();

extern struct VertexArray* get_or_add_array(struct Shape * shape, int array_type);

extern void append_vertex(struct Shape * shape, float * v);
extern void append_vertex2(struct Shape * shape, float * v1, float * v2);

extern void set_vertex(struct Shape * shape, int va_index, int index, float * v);
extern float * get_vertex(struct Shape * shape, int va_index, int index);

extern char * get_attribute(struct Shape * shape, char * name);
extern void set_attribute(struct Shape * shape, char * name, char * value);

extern int clamp_int(int v, int min, int max);
extern float clamp_float(float v, float min, float max);

extern struct Shape * new_shape();
extern struct Shape * read_shape(FILE * fp);
extern int write_shape(FILE * fp, struct Shape * shape);

/*int point_in_triangle(vec2d A, vec2d B, vec2d C, vec2d P);*/

extern char * array_type_names[];
extern char * gl_type_names[];

extern const char * get_array_type_name(int array_type);
extern const char * get_gl_type_name(int gl_type);

#ifdef SCHEME_CREATE_MAIN
  extern int ARGC;
  extern char ** ARGV;
  extern char * COMMAND;
  int main(int argc, char ** argv)
  {
    ARGC = argc;
    ARGV = argv;
    
    int i = 0, length = 0;
    while (ARGV[i] != NULL)
      length += strlen(ARGV[i++]) + 1; // +1 for space
    
    i = 0;
    COMMAND = malloc(length + 1); // +1 for the NULL
    while (ARGV[i] != NULL)
    {
      strcat(COMMAND, ARGV[i++]);
      strcat(COMMAND, " ");
    }
    COMMAND[length-1] = 0; // get rid of the last space
    
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
      fprintf(stderr, "%s\n", COMMAND);
      return SCHEME_FUNCTION(argc, argv, stdin, stdout, stderr);
    #else
      #error SCHEME_CREATE_MAIN is defined but no SCHEME_FUNCTION was defined
      fprintf(stderr, "%s: SCHEME_CREATE_MAIN is defined but no SCHEME_FUNCTION was defined\n", argv[0]);
      return EXIT_FAILURE;
    #endif
  }
#endif

#endif
