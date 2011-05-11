
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDIN_IS_PIPED
#define SCHEME_FUNCTION inspect
#include "scheme.h"

int inspect(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  int num_vertexs_to_show = 4;
  char selected_attribute[100] = "";
  int c;
  while ((c = getopt(argc, argv, "n:a:")) != -1)
  switch (c)
  {
    case 'n':
      num_vertexs_to_show = clamp_int(atoi(optarg), 3, 100);
      break;
    case 'a':
      strncpy(selected_attribute, optarg, sizeof(selected_attribute));
      break;
    default:
      abort();
  }
  
  int num_shapes_without_selected_attribute = 0;
  int num_values = 0;
  char ** values = NULL;
  int * value_counts = NULL;
  
  int num_shapes = 0;
  int num_shapes_with_no_attributes = 0;
  int num_shapes_with_no_vertexs = 0;
  int num_vertexs = 0;
  int num_each_gl_type[7] = {0,0,0,0,0,0,0};
  
  char gl_types_c[8][20] = {"GL_POINTS", "GL_LINES", "GL_LINE_LOOP", "GL_LINE_STRIP", "GL_TRIANGLES", "GL_TRIANGLE_STRIP", "GL_TRIANGLE_FAN"};
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    if (shape->gl_type < 7) num_each_gl_type[shape->gl_type]++;
    if (shape->num_vertexs == 0) num_shapes_with_no_vertexs++;
    
    num_vertexs += shape->num_vertexs;
    num_shapes++;
    
    long i;
    
    if (selected_attribute[0] != 0) // -a [selected_attribute] provided
    {
      int has_attribute = 0;
      for (i = 0 ; i < shape->num_attributes ; i++)
      {
        struct Attribute * attribute = &shape->attributes[i];
        
        // only process the selected attribute
        if (strcmp(shape->attributes[i].name, &selected_attribute[0]) != 0) continue;
        
        has_attribute = 1;
        
        // has this value been found previously?
        int j, found = 0;
        for (j = 0 ; j < num_values ; j++)
          if (strcmp(shape->attributes[i].value, values[j])==0)
            { found = 1; value_counts[j]++; break; }
        if (found) continue;
        
        // value hasn't been found, add it to the list
        values = realloc(values, sizeof(char*)*(num_values+1));
        value_counts = realloc(value_counts, sizeof(int)*(num_values+1));
        values[num_values] = malloc(shape->attributes[i].value_length+1);
        value_counts[num_values] = 1;
        strncpy(values[num_values], shape->attributes[i].value, shape->attributes[i].value_length);
        num_values++;
      }
      if (has_attribute == 0) num_shapes_without_selected_attribute++;
    }
    else if (num_shapes <= 2)
    {
      long count_zero = 0;
      
      fprintf(stderr, "shape:\n");
      fprintf(stderr, "  unique_set_id: %d\n", shape->unique_set_id);
      fprintf(stderr, "  gl_type: %s\n", (shape->gl_type >= 0 && shape->gl_type < 8) ? gl_types_c[shape->gl_type] : "????");
      fprintf(stderr, "  num_attributes: %d\n", shape->num_attributes);
      if (shape->num_attributes) fprintf(stderr, "  attributes:\n");
      for (i = 0 ; i < shape->num_attributes ; i++)
      {
        struct Attribute * attribute = &shape->attributes[i];
        fprintf(stderr, "    %s(%d): '%s'\n", attribute->name, attribute->value_length, attribute->value);
      }
      fprintf(stderr, "  num_vertexs: %d\n", shape->num_vertexs);
      fprintf(stderr, "  num_vertex_arrays: %d\n", shape->num_vertex_arrays);
      if (shape->num_vertex_arrays) fprintf(stderr, "  vertex_arrays:\n");
      for (i = 0 ; i < shape->num_vertex_arrays ; i++)
      {
        fprintf(stderr, "    array_type: %d\n", shape->vertex_arrays[i].array_type);
        fprintf(stderr, "    num_dimensions: %d\n", shape->vertex_arrays[i].num_dimensions);
        fprintf(stderr, "    vertexs:\n");
        if (shape->num_vertexs > 0 && shape->vertex_arrays[i].num_dimensions > 0)
        {
          long j,k;
          for (k = 0 ; k < shape->num_vertexs ; k++)
          {
            if (k < num_vertexs_to_show) fprintf(stderr, "      ");
            int is_zero = 1;
            for (j = 0 ; j < shape->vertex_arrays[i].num_dimensions ; j++)
            {
              if (shape->vertex_arrays[i].vertexs[k*shape->vertex_arrays[i].num_dimensions + j] != 0.0) is_zero = 0;
              if (k < num_vertexs_to_show) fprintf(stderr, "%f ", shape->vertex_arrays[i].vertexs[k*shape->vertex_arrays[i].num_dimensions + j]);
            }
            if (is_zero) count_zero ++;
            if (k < num_vertexs_to_show) fprintf(stderr, "\n");
            else if (k == num_vertexs_to_show) fprintf(stderr, "      ...\n");
          }
        }
        if (i == num_vertexs_to_show) fprintf(stderr, "    [...]\n");
      }
      if (count_zero > 0) fprintf(stderr, "  count_zero: %ld\n", count_zero);
    }
  }
  
  fprintf(pipe_err, "{\n");
  
  if (selected_attribute[0] != 0) // -a [attribute_name] provided
  {
    fprintf(pipe_err, "  \"num_unique_values\": %d,\n", num_values);
    fprintf(pipe_err, "  \"selected_attribute\": \"%s\",\n", selected_attribute);
    
    if (num_shapes_without_selected_attribute > 0)
      fprintf(pipe_err, "  \"num_shapes_without_selected_attribute\": %d,\n", num_shapes_without_selected_attribute);
    
    if (num_values > 0) fprintf(pipe_err, "  \"first_ten_unique_values\": [\n");
    int i;
    for (i = 0 ; i < num_values ; i++)
    {
      if (i < 10) fprintf(pipe_err, "    \"%s (%d)\"%s\n", values[i], value_counts[i], (i==num_values-1)?"":",");
      free(values[i]);
    }
    if (num_values > 0) fprintf(pipe_err, "  ]\n");
    free(values);
    free(value_counts);
  }
  
  if (num_shapes_with_no_vertexs > 0) printf("  \"num_shapes_with_no_vertexs\": %d,\n", num_shapes_with_no_vertexs);
  fprintf(pipe_err, "  \"num_shapes\": %d,\n", num_shapes);
  fprintf(pipe_err, "  \"num_vertexs\": %d,\n", num_vertexs);
  fprintf(pipe_err, "  \"num_each_gl_type\": [%d,%d,%d,%d,%d,%d,%d]\n", num_each_gl_type[0], num_each_gl_type[1], num_each_gl_type[2], num_each_gl_type[3], num_each_gl_type[4], num_each_gl_type[5], num_each_gl_type[6]);
  fprintf(pipe_err, "}\n");
}