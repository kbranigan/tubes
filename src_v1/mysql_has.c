
#include <stdio.h>
#include <stdlib.h>
#include "mysql_has.h"

int mysql_has(MYSQL * mysql, const char * database, const char * table, const char * field)
{
  if (database == NULL) return 0;
  
  char query[500];
  sprintf(query, "SELECT * FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = \"%s\"", database);
  //fprintf(stderr, "%s\n", query);
  if (mysql_query(mysql, query)==0)
  {
    MYSQL_RES * res = mysql_store_result(mysql);
    int num_rows = mysql_num_rows(res);
    mysql_free_result(res);
    if (num_rows == 0) return 0;
  }
  
  if (table == NULL) return 1;
  sprintf(query, "SELECT * FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = \"%s\" AND TABLE_NAME = \"%s\"", database, table);
  //fprintf(stderr, "%s\n", query);
  if (mysql_query(mysql, query)==0)
  {
    MYSQL_RES * res = mysql_store_result(mysql);
    int num_rows = mysql_num_rows(res);
    mysql_free_result(res);
    if (num_rows == 0) return 0;
  }
  
  if (field == NULL) return 1;
  sprintf(query, "SELECT * FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = \"%s\" AND TABLE_NAME = \"%s\" AND COLUMN_NAME = \"%s\"", database, table, field);
  //fprintf(stderr, "%s\n", query);
  if (mysql_query(mysql, query)==0)
  {
    MYSQL_RES * res = mysql_store_result(mysql);
    int num_rows = mysql_num_rows(res);
    mysql_free_result(res);
    if (num_rows == 0) return 0;
  }
  
  return 1;
}
