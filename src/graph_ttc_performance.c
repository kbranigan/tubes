
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION graph_ttc_performance
#include "scheme.h"

int graph_ttc_performance(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char distance_attribute[300] = "dist_line_192";
  int c;
  while ((c = getopt(argc, argv, "a:")) != -1)
  switch (c)
  {
    case 'a':
      strncpy(distance_attribute, optarg, 300);
      break;
    default:
      abort();
  }
  
  struct Shape * line = NULL;
  int distance_value = INT_MAX;
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    if (get_attribute(shape, "vehicle_number") == NULL) { free_shape(shape); continue; }
    
    if (line == NULL || strcmp(get_attribute(line, "vehicle_number"), get_attribute(shape, "vehicle_number")) != 0 || atoi(get_attribute(shape, distance_attribute)) < distance_value)
    {
      if (line != NULL && line->num_vertexs > 0) { write_shape(pipe_out, line); free_shape(line); }
      
      line = new_shape();
      line->gl_type = GL_LINE_STRIP;
      set_attribute(line, "vehicle_number", get_attribute(shape, "vehicle_number"));
      //struct VertexArray * cva = get_or_add_array(line, GL_COLOR_ARRAY); // just to create it
      //set_num_dimensions(line, 1, 4);
    }
    
    float v[3] = { atof(get_attribute(shape, distance_attribute)), atof(get_attribute(shape, "reported_at")), 0 };
    //float c[4] = { 0, 0, 0, 1 };
    //append_vertex2(line, v, c);
    
    append_vertex(line, v);
    
    struct Shape * marker = new_shape();
    marker->gl_type = GL_LINES;
    append_vertex(marker, v);
    v[1] -= 100;
    append_vertex(marker, v);
    write_shape(pipe_out, marker);
    free_shape(marker);
    
    distance_value = atoi(get_attribute(shape, distance_attribute));
    free_shape(shape);
  }
  if (line != NULL && line->num_vertexs > 0) { write_shape(pipe_out, line); free_shape(line); }
}
