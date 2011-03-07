
#ifndef SCHEME_H
#define SCHEME_H

#include <inttypes.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "vectmath.h"

#define FILE_VERSION_2 2
#define FILE_VERSION_3 3

#define CURRENT_VERSION FILE_VERSION_3

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
  
  uint32_t         file header          42
  uint32_t         file version         2
  [ // shapes (read until feof or fread error)
    float          shape header         INFINITY
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
                                        //32885 = GL_NORMAL_ARRAY
                                        //32886 = GL_COLOR_ARRAY
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
  float * vertexs; // shape->num_vertexs * num_dimensions
};

struct Attribute {
  uint32_t key_length;
  char * key;
  uint32_t value_length;
  char * value;
};

struct Shape {
  uint32_t unique_set_id;
  
  uint32_t num_attributes;
  struct Attribute * attributes;
  
  uint32_t gl_type;
  
  uint32_t num_vertexs;
  uint32_t num_vertex_arrays;
  struct VertexArray * vertex_arrays;
};

extern int stdin_is_piped();
extern int stdout_is_piped();

extern int stdin_has_data();

extern int write_header(FILE * fp, uint32_t file_version);
extern int read_header(FILE * fp, uint32_t req_file_version);

extern void inspect_shape(FILE * fp, struct Shape * shape);
extern struct Shape * read_shape(FILE * fp);
extern int write_shape(FILE * fp, struct Shape * shape);

/*extern struct Shapes * load_shapes(FILE * fp);

extern void write_shape(struct Shape * s, FILE * fp);
extern void write_shapes(struct Shapes * ss, FILE * fp);

int point_in_triangle(vec2d A, vec2d B, vec2d C, vec2d P);*/

#endif
