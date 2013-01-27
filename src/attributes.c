
#include "block.h"

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  assert_stdin_is_piped();
  assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
  static char remove_attributes_all[1000] = "";
  static char int_attributes_all[1000] = "";
  static int output_header = 1;
  static int debug = 0;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"remove", required_argument, 0, 'r'},
      {"makeint", required_argument, 0, 'i'},
      //{"add", no_argument, &output_header, 1},
      {"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "r:i:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'r': strncpy(remove_attributes_all, optarg, sizeof(remove_attributes_all)); break;
      case 'i': strncpy(int_attributes_all, optarg, sizeof(int_attributes_all)); break;
      default: abort();
    }
  }
  
  char remove_attributes_copy[1000] = "";
  strncpy(remove_attributes_copy, remove_attributes_all, 1000);
  
  char int_attributes_copy[1000] = "";
  strncpy(int_attributes_copy, int_attributes_all, 1000);
  
  int num_remove_attributes = 0;
  char ** remove_attributes = NULL;
  char * pch = strtok(remove_attributes_all, ",");
  while (pch != NULL)
  {
    num_remove_attributes++;
    remove_attributes = realloc(remove_attributes, sizeof(char*)*num_remove_attributes);
    remove_attributes[num_remove_attributes-1] = pch;
    pch = strtok(NULL, ",");
  }
  
  int num_int_attributes = 0;
  char ** int_attributes = NULL;
  pch = strtok(int_attributes_all, ",");
  while (pch != NULL)
  {
    num_int_attributes++;
    int_attributes = realloc(int_attributes, sizeof(char*)*num_int_attributes);
    int_attributes[num_int_attributes-1] = pch;
    pch = strtok(NULL, ",");
  }
  
  int i,j;
  struct Block * block = NULL;
  while ((block = read_block(stdin)))
  {
    struct Block * newblock = new_block();
    newblock = copy_all_attributes(newblock, block);
    
    if (remove_attributes_copy[0] != 0)
      newblock = add_string_attribute(newblock, "remove attributes", remove_attributes_copy);
    
    if (int_attributes_copy[0] != 0)
      newblock = add_string_attribute(newblock, "convert attributes to int", int_attributes_copy);
    
    int num_remove_attribute_ids = 0;
    int * remove_attribute_ids = NULL;
    
    for (i = 0 ; i < block->num_attributes ; i++)
    {
      struct attribute * col = get_attribute(block, i);
      for (j = 0 ; j < num_remove_attributes ; j++)
      {
        // fprintf(stderr, "%s(%d(%d)) vs %s(%d) = %d\n", 
        //                  attribute_get_name(col), col->name_length, strlen(attribute_get_name(col)), 
        //                  remove_attributes[j], strlen(remove_attributes[j]), 
        //                   strcmp(attribute_get_name(col), remove_attributes[j]));
        if (strcmp(attribute_get_name(col), remove_attributes[j])==0) break;
      }
      if (j == num_remove_attributes) // not a attribute to be removed
      {
        //fprintf(stderr, "%d: %s (%d) - do not remove\n", i, attribute_get_name(col), strlen(attribute_get_name(col)));
        for (j = 0 ; j < num_int_attributes ; j++)
          if (strcmp(attribute_get_name(col), int_attributes[j])==0) break;
        if (j == num_int_attributes)
        {
          num_remove_attribute_ids++;
          remove_attribute_ids = realloc(remove_attribute_ids, sizeof(int)*num_remove_attribute_ids);
          remove_attribute_ids[num_remove_attribute_ids-1] = i;
          newblock = _add_attribute(newblock, col->type, col->bsize, attribute_get_name(col));
        }
      }
    }
    
    int num_int_attribute_ids = 0;
    int * int_attribute_ids = NULL;
    
    for (i = 0 ; i < block->num_attributes ; i++)
    {
      struct attribute * col = get_attribute(block, i);
      for (j = 0 ; j < num_int_attributes ; j++)
        if (strcmp(attribute_get_name(col), int_attributes[j])==0) break;
      if (j != num_int_attributes)
      {
        num_int_attribute_ids++;
        int_attribute_ids = realloc(int_attribute_ids, sizeof(int)*num_int_attribute_ids);
        int_attribute_ids[num_int_attribute_ids-1] = i;
        newblock = add_int32_attribute(newblock, attribute_get_name(col));
      }
    }
    
    newblock = set_num_rows(newblock, block->num_rows);
    
    for (i = 0 ; i < newblock->num_rows ; i++)
    {
      for (j = 0 ; j < num_remove_attribute_ids ; j++)
      {
        void * dst = get_cell(newblock, i, j);
        void * src = get_cell(block, i, remove_attribute_ids[j]);
        struct attribute * col = get_attribute(block, remove_attribute_ids[j]);
        
        memcpy(dst, src, col->bsize);
      }
      for (j = 0 ; j < num_int_attribute_ids ; j++)
      {
        set_cell_from_int32(newblock, i, j + num_remove_attribute_ids, get_cell_as_int32(block, i, int_attribute_ids[j]));
      }
    }
    free(remove_attribute_ids);
    free(int_attribute_ids);
    
    write_block(stdout, newblock);
    free_block(newblock);
    free_block(block);
  }
}
