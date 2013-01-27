
#include <stdio.h>
#include <stdlib.h>

#include "mysql.h"

#include "block.h"

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  //assert_stdin_is_piped();
  assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
  static char database[1000] = "";
  static char table[1000] = "";
  int limit = 0;
  
  static int debug = 0;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"database", required_argument, 0, 'd'},
      {"table", required_argument, 0, 't'},
      {"limit", required_argument, 0, 'l'},
      {"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "d:t:l:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'd': strncpy(database, optarg, sizeof(database)); break;
      case 't': strncpy(table, optarg, sizeof(table)); break;
      case 'l': limit = atoi(optarg); break;
      default: abort();
    }
  }
  
  if (database[0] == 0 || table[0] == 0)
  {
    fprintf(stderr, "ERROR: Usage: %s -d [database] -t [table]\n", argv[0]);
    return -1;
  }
  
  MYSQL mysql;
  MYSQL_RES * res;
  MYSQL_ROW row;
  char query[500];
  
  if ((mysql_init(&mysql) == NULL)) { fprintf(stderr, "ERROR: mysql_init() error\n"); return 0; }
  if (!mysql_real_connect(&mysql, "localhost", "root", "", database, 0, NULL, 0))
  {
    fprintf(stderr, "ERROR: mysql_real_connect error (%s)\n", mysql_error(&mysql));
  }
  
  struct Block * block = new_block();
  
  block = add_string_attribute(block, "source", "mysql_table");
  block = add_string_attribute(block, "database", database);
  block = add_string_attribute(block, "table", table);
  
  sprintf(query, "DESCRIBE `%s`.`%s`", database, table);
  if (mysql_query(&mysql, query)==0)
  {
    res = mysql_store_result(&mysql);
    int num_rows = mysql_num_rows(res);
    while ((row = mysql_fetch_row(res)))
    {
      if (strncmp(row[1], "int", 3)==0)          block = add_int32_column(block, row[0]);
      else if (strncmp(row[1], "float", 5)==0)   block = add_float_column(block, row[0]);
      else if (strncmp(row[1], "double", 6)==0)  block = add_double_column(block, row[0]);
      else if (strncmp(row[1], "varchar", 7)==0) block = add_string_column_with_length(block, row[0], atoi(&row[1][8]));
      else { fprintf(stderr, "ERROR: not handling mysql type '%s' (aborting)\n", row[1]); return; }
    }
    mysql_free_result(res);
  }
  else
  {
    fprintf(stderr, "ERROR: mysql_query() error %s\n", mysql_error(&mysql));
  }
  
  sprintf(query, "SELECT * FROM `%s`.`%s`", database, table);
  if (limit > 0) { char temp[100]; sprintf(temp, " LIMIT %d", limit); strcat(query, temp); }
  if (mysql_query(&mysql, query)==0)
  {
    int row_id = 0;
    int column_id = 0;
    res = mysql_store_result(&mysql);
    block = set_num_rows(block, mysql_num_rows(res));
    while ((row = mysql_fetch_row(res)))
    {
      for (column_id = 0 ; column_id < block->num_columns ; column_id++)
      {
        struct Column * column = get_column(block, column_id);
        if      (column->type == TYPE_INT && column->bsize == 4)   { int value = atoi(row[column_id]); set_cell(block, row_id, column_id, &value); }
        else if (column->type == TYPE_INT && column->bsize == 8)   { long value = atoi(row[column_id]); set_cell(block, row_id, column_id, &value); }
        else if (column->type == TYPE_FLOAT && column->bsize == 4) { float value = atof(row[column_id]); set_cell(block, row_id, column_id, &value); }
        else if (column->type == TYPE_FLOAT && column->bsize == 8) { double value = atof(row[column_id]); set_cell(block, row_id, column_id, &value); }
        else if (column->type == TYPE_CHAR) set_cell(block, row_id, column_id, row[column_id]);
      }
      row_id++;
    }
    mysql_free_result(res);
  }
  else
  {
    fprintf(stderr, "ERROR: mysql_query() error %s\n", mysql_error(&mysql));
  }
  
  write_block(stdout, block);
  fprintf(stderr, "%s: Finished.  %d rows, %d columns read from mysql `%s`.`%s`, enjoy!\n", argv[0], block->num_rows, block->num_columns, database, table);
  free_block(block);
  
  mysql_close(&mysql);
}
