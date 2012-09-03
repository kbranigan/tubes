
#include "block.h"

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  //assert_stdin_is_piped();
  assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
  static char filename[1000] = "";
  static int random = 0;
  static int debug = 0;
  
  static float red = 0;
  static float green = 0;
  static float blue = 0;
  //static float alpha = 0;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"random", no_argument, &random, 1},
      {"filename", required_argument, 0, 'f'},
      {"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "f:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'f': strncpy(filename, optarg, sizeof(filename)); break;
      default: abort();
    }
  }
  
  struct Block * rules = NULL;
  
  if (filename[0] != 0)
  {
    FILE * rules_fp = fopen(filename, "r");
    if (rules_fp == NULL) fprintf(stderr, "ERROR reading from '%s' file\n", filename);
    else
    {
      fprintf(stderr, "reading color descriptors from '%s'\n", filename);
      rules = read_block(rules_fp);
      fclose(rules_fp);
      if (rules == NULL) fprintf(stderr, "rules descriptor file invalid block file\n");
      else
      {
        if (get_column_id_by_name(rules, "red")   == -1) fprintf(stderr, "rules descriptor has no 'red' field\n");
        if (get_column_id_by_name(rules, "green") == -1) fprintf(stderr, "rules descriptor has no 'green' field\n");
        if (get_column_id_by_name(rules, "blue")  == -1) fprintf(stderr, "rules descriptor has no 'blue' field\n");
        if (rules->num_columns < 2) fprintf(stderr, "rules descriptor doesn't have enough columns\n");
        if (rules->num_rows < 1)    fprintf(stderr, "rules descriptor doesn't have enough rows\n");
      }
    }
  }
  
  struct Block * block = NULL;
  while ((block = read_block(stdin)))
  {
    if (rules != NULL)
    {
      char colors[4][10] = {"red", "green", "blue", "alpha"};
      int rules_color_column_ids[4] = { -1, -1, -1, -1 };
      int block_color_column_ids[4] = { -1, -1, -1, -1 };
      
      int i;
      for (i = 0 ; i < 4 ; i++)
      {
        rules_color_column_ids[i] = get_column_id_by_name(rules, colors[i]);
        if (rules_color_column_ids[i] != -1 && get_column_id_by_name(block, colors[i]) == -1)
          block = add_float_column(block, colors[i]);
        block_color_column_ids[i] = get_column_id_by_name(block, colors[i]);
      }
      
      int rule_column_id;
      for (rule_column_id = 0 ; rule_column_id < rules->num_columns ; rule_column_id++)
      {
        for (i = 0 ; i < 4 ; i++)
          if (rules_color_column_ids[i] == rule_column_id)
            break;
        if (i != 4) continue;
        
        struct Column * rule = get_column(rules, rule_column_id);
        int block_column_id;
        for (block_column_id = 0 ; block_column_id < block->num_columns ; block_column_id++)
        {
          struct Column * column = get_column(block, block_column_id);
          
          // rule->type == column->type &&  // can't do this cause of string length and floats put in string fields
          if (strcmp(column_get_name(rule), column_get_name(column))==0)
          {
            int block_row_id;
            for (block_row_id = 0 ; block_row_id < block->num_rows ; block_row_id++)
            {
              float temp = 0;
              for (i = 0 ; i < 4 ; i++)
                if (block_color_column_ids[i] != -1)
                {
                  if (i == 3) temp = 1.0;
                  else temp = 0.0;
                  set_cell(block, block_row_id, block_color_column_ids[i], &temp);
                }
              
              if (rule->type == column->type && rule->type == INT_TYPE)
              {
                
              }
              else if (column_is_string(rule) && column_is_string(column)) // strings
              {
                int width = (rule->type > column->type) ? column->type : rule->type;
                int rule_row_id;
                for (rule_row_id = 0 ; rule_row_id < rules->num_rows ; rule_row_id++)
                {
                  if (strncmp((char*)get_cell(block, block_row_id, block_column_id), (char*)get_cell(rules, rule_row_id, rule_column_id), width)==0)
                  {
                    for (i = 0 ; i < 4 ; i++)
                    {
                      if (rules_color_column_ids[i] != -1 && block_color_column_ids[i] != -1)
                      {
                        struct Column * color_column = get_column(rules, rules_color_column_ids[i]);
                        if (column_is_string(color_column))
                        {
                          float color = get_cell_as_float(rules, rule_row_id, rules_color_column_ids[i]);
                          set_cell(block, block_row_id, block_color_column_ids[i], &color);
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    
    if (random)
    {
      fprintf(stderr, "random colors\n");
      block = add_float_column(block, "red");
      block = add_float_column(block, "green");
      block = add_float_column(block, "blue");
      block = add_float_column(block, "alpha");
      
      float temp;
      int row_id;
      for (row_id = 0 ; row_id < block->num_rows ; row_id++)
      {
        temp = rand() / (float)RAND_MAX;
        set_cell(block, row_id, block->num_columns-4, &temp);
        temp = rand() / (float)RAND_MAX;
        set_cell(block, row_id, block->num_columns-3, &temp);
        temp = rand() / (float)RAND_MAX;
        set_cell(block, row_id, block->num_columns-2, &temp);
        temp = 1; //rand() / (float)RAND_MAX;
        set_cell(block, row_id, block->num_columns-1, &temp);
      }
    }
    
    write_block(stdout, block);
    free_block(block);
  }
  free_block(rules);
}
