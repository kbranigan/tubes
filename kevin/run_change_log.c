
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

  static char change_log_filename[1000] = "";
  int limit = 0;
  int port = 0; // default
  
  static int debug = 0;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"changelog", required_argument, 0, 'c'},
      {"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "c:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'c': strncpy(change_log_filename, optarg, sizeof(change_log_filename)); break;
      default: abort();
    }
  }
  
  if (change_log_filename[0] == 0)
  {
    fprintf(stderr, "ERROR: changelog (-c) required (should be a block, from <(./bin/read_mysql -q \"select * from iroquois.changes\") most likely)\n");
    return 0;
  }

  FILE * fp = fopen(change_log_filename, "r");
  if (fp == NULL)
  {
    fprintf(stderr, "'%s' is not a value filename\n", change_log_filename);
    return 0;
  }
  struct Block * change_log = read_block(fp);
  if (change_log == NULL)
  {
    fprintf(stderr, "'%s' is not a value file (does not contain a valid tubes block)\n", change_log_filename);
    return 0;
  }
  int32_t change_log_id_column_id             = get_column_id_by_name_or_exit(change_log, "id");
  int32_t change_log_created_at_column_id     = get_column_id_by_name_or_exit(change_log, "created_at");
  int32_t change_log_feed_directory_column_id = get_column_id_by_name_or_exit(change_log, "feed_directory");
  int32_t change_log_feed_group_column_id     = get_column_id_by_name_or_exit(change_log, "feed_group");
  int32_t change_log_filename_column_id       = get_column_id_by_name_or_exit(change_log, "filename");
  int32_t change_log_operation_column_id      = get_column_id_by_name_or_exit(change_log, "operation");
  int32_t change_log_search_fields_column_id  = get_column_id_by_name_or_exit(change_log, "search_fields");
  int32_t change_log_update_fields_column_id  = get_column_id_by_name_or_exit(change_log, "update_fields");
  fclose(fp);

  struct Block * block = NULL;
  while ((block = read_block(stdin)))
  {
    block = add_command(block, argc, argv);
    if (get_attribute_id_by_name(change_log, "query") != -1)
    {
      block = add_string_attribute(block, "change_log_query", get_attribute_value_as_string(change_log, "query"));
    }
    block = add_int32_attribute(block, "num_change_logs_matched", 0);
    block = add_int32_attribute(block, "num_rows_matched", 0);
    block = add_int32_attribute(block, "num_rows_created", 0);
    block = add_int32_attribute(block, "num_rows_updated", 0);
    block = add_int32_attribute(block, "num_rows_deleted", 0);

    // this is going to get pretty slow
    for (int change_log_row_id = 0 ; change_log_row_id < change_log->num_rows ; change_log_row_id++)
    {
      if (strcmp(get_cell(change_log, change_log_row_id, change_log_filename_column_id), "")==0 || 
          strstr(get_attribute_value_as_string(block, "source csv file"), 
                 get_cell(change_log, change_log_row_id, change_log_filename_column_id)) != NULL)
      {
        #ifdef DEBUG
        //fprintf(stderr, "%s vs %s\n", get_attribute_value_as_string(block, "source csv file"), get_cell(change_log, change_log_row_id, change_log_filename_column_id));
        //fprintf(stderr, "change_log id %d matched\n", get_cell_as_int32(change_log, change_log_row_id, change_log_id_column_id));
        #endif

        cJSON * search_fields = cJSON_Parse(get_cell(change_log, change_log_row_id, change_log_search_fields_column_id));
        cJSON * update_fields = cJSON_Parse(get_cell(change_log, change_log_row_id, change_log_update_fields_column_id));

        set_attribute_value_as_int32(block, "num_change_logs_matched", get_attribute_value_as_int32(block, "num_change_logs_matched") + 1);
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

          if (strcmp(get_cell(change_log, change_log_row_id, change_log_operation_column_id), "update")==0)
          {
            if (update_fields == NULL)
            {
              fprintf(stderr, "operation is 'update' but update_fields is invalid\n");
            }
            else
            {
              for (int i = 0 ; i < cJSON_GetArraySize(update_fields) ; i++)
              {
                cJSON * update_field = cJSON_GetArrayItem(update_fields, i);
                int column_id = get_column_id_by_name(block, update_field->string);
                if (column_id == -1) { fprintf(stderr, "field '%s' wasn't found\n", update_field->string); continue; }
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
          else if (strcmp(get_cell(change_log, change_log_row_id, change_log_operation_column_id), "delete")==0)
          {
            fprintf(stderr, "unknown operation %s\n", get_cell(change_log, change_log_row_id, change_log_operation_column_id));
          }
          else
          {
            fprintf(stderr, "unknown operation %s\n", get_cell(change_log, change_log_row_id, change_log_operation_column_id));
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













