
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ext/uthash.h"

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDOUT_IS_PIPED
#define SCHEME_FUNCTION join_on_attributes
#include "scheme.h"

struct Value {
  char value[60];
  int num_row_ids;
  int * row_ids;
  UT_hash_handle hh;
};

int join_on_attributes(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char file_name[300] = "";
  char left_field[50] = "";
  char right_field[50] = "";
  char join_type[50] = "left";
  int c;
  while ((c = getopt(argc, argv, "f:l:r:j:")) != -1)
  switch (c)
  {
    case 'f': strncpy(file_name, optarg, sizeof(file_name)); break;
    case 'l': strncpy(left_field, optarg, sizeof(left_field)); break;
    case 'r': strncpy(right_field, optarg, sizeof(right_field)); break;
    case 'j': strncpy(join_type, optarg, sizeof(join_type)); break;
    default:  abort();
  }
  
  if (strlen(file_name) == 0) { fprintf(stderr, "%s: must specify a valid file\n", argv[0]); exit(1); }
  if (strlen(left_field) == 0) { fprintf(stderr, "%s: must specify a left_field using -l\n", argv[0]); exit(1); }
  if (strlen(right_field) == 0) { fprintf(stderr, "%s: must specify a right_field using -l\n", argv[0]); exit(1); }
  
  int left_field_index = -1;
  int right_field_index = -1;
  
  int field_id;
  int row_id;
  
  struct Value * left_field_hash = NULL;
  struct Value * right_field_hash = NULL;
  
  FILE * right_fp = NULL;
  right_fp = fopen(file_name, "r");
  
  unsigned int num_right_shapes = 0;
  struct Shape ** right_shapes = read_all_shapes(right_fp, &num_right_shapes);
  
  fclose(right_fp);
  
  if (num_right_shapes == 0) { fprintf(stderr, "no shapes provided on the right side\n"); exit(1); }
  
  for (row_id = 0 ; row_id < num_right_shapes ; row_id++)
  {
    struct Shape * shape = right_shapes[row_id];
    if (shape->num_attributes == 0) continue;
    
    if (right_field_index == -1 || right_field_index >= shape->num_attributes || strcmp(shape->attributes[right_field_index].name, right_field) != 0)
      for (field_id = 0 ; field_id < shape->num_attributes ; field_id++)
        if (strcmp(shape->attributes[field_id].name, right_field)==0)
        {
          right_field_index = field_id;
          break;
        }
    
    struct Value * v = NULL;
    HASH_FIND_STR(right_field_hash, shape->attributes[right_field_index].value, v);
    if (v == NULL)
    {
      v = (struct Value*)malloc(sizeof(struct Value));
      strncpy(v->value, shape->attributes[right_field_index].value, 60);
      v->num_row_ids = 0;
      v->row_ids = NULL;
      HASH_ADD_STR(right_field_hash, value, v);
    }
    v->num_row_ids++;
    v->row_ids = (int*)realloc(v->row_ids, sizeof(int) * v->num_row_ids);
    v->row_ids[v->num_row_ids - 1] = row_id;
  }
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    if (left_field_index == -1)
      for (field_id = 0 ; field_id < shape->num_attributes ; field_id++)
        if (strcmp(left_field, shape->attributes[field_id].name)==0)
          left_field_index = field_id;
    
    if (left_field_index == -1)
      continue;
    
    struct Value * v = NULL;
    HASH_FIND_STR(right_field_hash, shape->attributes[left_field_index].value, v);
    
    if (v != NULL)
    {
      int g;
      for (g = 0 ; g < v->num_row_ids ; g++)
      {
        struct Shape * right_shape = right_shapes[v->row_ids[g]];
        
        for (field_id = 0 ; field_id < right_shape->num_attributes ; field_id++)
        {
          set_attribute(shape, right_shape->attributes[field_id].name, right_shape->attributes[field_id].value);
        }
        write_shape(pipe_out, shape);
      }
    }
    //else
    //  write_shape(pipe_out, shape);
    free_shape(shape);
  }
}
