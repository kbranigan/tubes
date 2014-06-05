
#include <stdio.h>
#include <stdlib.h>

#include "../src/block.h"
#include "../ext/cJSON.h"

//#define DEBUG

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  assert_stdin_is_piped();
  assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();

  static char changes_filename[1000] = "";
  int limit = 0;
  int port = 0; // default
  
  static int debug = 0;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"changes", required_argument, 0, 'c'},
      {"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "c:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'c': strncpy(changes_filename, optarg, sizeof(changes_filename)); break;
      default: abort();
    }
  }
  
  if (changes_filename[0] == 0)
  {
    fprintf(stderr, "ERROR: changelog (-c) required (should be a block, from <(./bin/read_mysql -q \"select * from iroquois.changes\") most likely)\n");
    return 0;
  }

  FILE * fp = fopen(changes_filename, "r");
  if (fp == NULL)
  {
    fprintf(stderr, "'%s' is not a value filename\n", changes_filename);
    return 0;
  }
  struct Block * changes = read_block(fp);
  if (changes == NULL)
  {
    fprintf(stderr, "'%s' is not a value file (does not contain a valid tubes block)\n", changes_filename);
    return 0;
  }
  int32_t changes_id_column_id             = get_column_id_by_name_or_exit(changes, "id");
  int32_t changes_created_at_column_id     = get_column_id_by_name_or_exit(changes, "created_at");
  int32_t changes_feed_directory_column_id = get_column_id_by_name_or_exit(changes, "feed_directory");
  int32_t changes_feed_group_column_id     = get_column_id_by_name_or_exit(changes, "feed_group");
  int32_t changes_filename_column_id       = get_column_id_by_name_or_exit(changes, "filename");
  int32_t changes_operation_column_id      = get_column_id_by_name_or_exit(changes, "operation");
  int32_t changes_search_fields_column_id  = get_column_id_by_name_or_exit(changes, "search_fields");
  int32_t changes_update_fields_column_id  = get_column_id_by_name_or_exit(changes, "update_fields");
  fclose(fp);

  struct Block * block = NULL;
  while ((block = read_block(stdin)))
  {
    block = add_command(block, argc, argv);
    if (get_attribute_id_by_name(changes, "query") != -1)
    {
      block = add_string_attribute(block, "changes_query", get_attribute_value_as_string(changes, "query"));
    }
    block = add_int32_attribute(block, "num_changess_matched", 0);
    block = add_int32_attribute(block, "num_rows_matched", 0);
    block = add_int32_attribute(block, "num_rows_created", 0);
    block = add_int32_attribute(block, "num_rows_updated", 0);
    block = add_int32_attribute(block, "num_rows_deleted", 0);

    // this is going to get pretty slow
    for (int changes_row_id = 0 ; changes_row_id < changes->num_rows ; changes_row_id++)
    {
      if (strcmp(get_cell(changes, changes_row_id, changes_filename_column_id), "")==0 || 
          strstr(get_attribute_value_as_string(block, "source csv file"), 
                 get_cell(changes, changes_row_id, changes_filename_column_id)) != NULL)
      {
        #ifdef DEBUG
        //fprintf(stderr, "%s vs %s\n", get_attribute_value_as_string(block, "source csv file"), get_cell(changes, changes_row_id, changes_filename_column_id));
        //fprintf(stderr, "changes id %d matched\n", get_cell_as_int32(changes, changes_row_id, changes_id_column_id));
        #endif

        cJSON * search_fields = cJSON_Parse(get_cell(changes, changes_row_id, changes_search_fields_column_id));
        cJSON * update_fields = cJSON_Parse(get_cell(changes, changes_row_id, changes_update_fields_column_id));

        set_attribute_value_as_int32(block, "num_changess_matched", get_attribute_value_as_int32(block, "num_changess_matched") + 1);
        for (int row_id = 0 ; row_id < block->num_rows ; row_id++)
        {
          int search_params_match = 1;

          for (int i = 0 ; i < cJSON_GetArraySize(search_fields) ; i++)
          {
            cJSON * search_field = cJSON_GetArrayItem(search_fields, i);
            int column_id = get_column_id_by_name(block, search_field->string);
            if (column_id == -1) { fprintf(stderr, "field '%s' wasn't found\n", search_field->string); search_params_match = 0; break; }
            if (search_field->type == cJSON_String)
            {
              if (strcmp(get_cell(block, row_id, column_id), search_field->valuestring)!=0) { search_params_match = 0; break; }
            }
            else { fprintf(stderr, "field '%s' is of unsupported type %d\n", search_field->string, search_field->type); search_params_match = 0; break; }
          }

          if (search_params_match == 0)
          {
            continue;
          }

          set_attribute_value_as_int32(block, "num_rows_matched", get_attribute_value_as_int32(block, "num_rows_matched") + 1);

          if (strcmp(get_cell(changes, changes_row_id, changes_operation_column_id), "update")==0)
          {
            if (update_fields == NULL)
            {
              fprintf(stderr, "operation is 'update' but update_fields is empty\n");
            }
            else
            {
              for (int i = 0 ; i < cJSON_GetArraySize(update_fields) ; i++)
              {
                cJSON * update_field = cJSON_GetArrayItem(update_fields, i);
                int column_id = get_column_id_by_name(block, update_field->string);
                if (column_id == -1)
                {
                  if (update_field->type == cJSON_String)
                  {
                    block = add_string_column_with_length_and_blank(block, update_field->string, strlen(update_field->valuestring) + 1);
                    column_id = block->num_columns - 1;
                  }
                  else
                  {
                    fprintf(stderr, "field '%s' wasn't found, tried adding but the field also didn't have a string value\n", update_field->string);
                    continue;
                  }
                }
                struct Column * column = get_column_by_name(block, update_field->string);
                if (update_field->type == cJSON_String)
                {
                  if (strlen(update_field->valuestring) > column->bsize)
                  {
                    set_string_column_length(block, column_id, strlen(update_field->valuestring) + 1);
                  }
                  set_cell_from_string(block, row_id, column_id, update_field->valuestring);
                }
                else { fprintf(stderr, "field '%s' is of unsupported type %d\n", update_field->string, update_field->type); continue; }
              }
            }
          }
          else if (strcmp(get_cell(changes, changes_row_id, changes_operation_column_id), "delete")==0)
          {
            fprintf(stderr, "unknown operation %s\n", get_cell(changes, changes_row_id, changes_operation_column_id));
          }
          else
          {
            fprintf(stderr, "unknown operation %s\n", get_cell(changes, changes_row_id, changes_operation_column_id));
          }
        }

        cJSON_Delete(search_fields);
        cJSON_Delete(update_fields);
      }
    }

    write_block(stdout, block);
    free_block(block);
  }
}













