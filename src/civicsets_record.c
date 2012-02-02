
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "mongoose.h"
#include <mysql.h>
#include "mysql_has.h"

void map(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  char * table_name = mg_get_var(conn, "table");
  if (table_name == NULL) { mg_printf(conn, "'table' argument required.\n"); return; }
  
  // kbfu
}

void record(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  if ((mysql_init(&mysql) == NULL)) { printf("mysql_init error\n"); }
  if (!mysql_real_connect(&mysql, "localhost", "root", "", "", 0, NULL, 0)) { printf("mysql_real_connect error\n"); }
  
  char * table_name = mg_get_var(conn, "table");
  if (table_name == NULL) { mg_printf(conn, "'table' argument required.\n"); return; }
  
  //mg_printf(conn, "%s\n", ri->query_string);
  
  char sql[5000];
  sprintf(sql, "p_%s", table_name);
  if (!mysql_has(&mysql, "civicsets", sql, NULL))
  {
    sprintf(sql, "CREATE TABLE `civicsets`.`p_%s` (id INT PRIMARY KEY AUTO_INCREMENT, created_at DATETIME)", table_name);
    //mg_printf(conn, "%s\n", sql);
    if (mysql_query(&mysql, sql) != 0) { mg_printf(conn, "create table sql error. isn't that bad?\n%s\n", mysql_error(&mysql)); mg_free(table_name); return; }
  }
  
  char fields[1000] = "`created_at`";
  char values[1000] = "NOW()";
  char * pch = strtok(ri->query_string, "&");
  while (pch != NULL)
  {
    char * field_name = pch;
    char * value = strstr(pch, "=");
    *(value++) = 0;
    
    if (strcmp(field_name, "table") == 0) { pch = strtok(NULL, "&"); continue; }
    
    sprintf(sql, "p_%s", table_name);
    if (!mysql_has(&mysql, "civicsets", sql, field_name))
    {
      if (strcmp(field_name, "lat") == 0 || strcmp(field_name, "lng") == 0 || strcmp(field_name, "lon") == 0)
        sprintf(sql, "ALTER TABLE `civicsets`.`p_%s` ADD COLUMN `%s` FLOAT", table_name, field_name);
      else
        sprintf(sql, "ALTER TABLE `civicsets`.`p_%s` ADD COLUMN `%s` VARCHAR(225)", table_name, field_name);
      //mg_printf(conn, "%s\n", sql);
      if (mysql_query(&mysql, sql) != 0) { mg_printf(conn, "alter table sql error. isn't that bad?\n%s\n", mysql_error(&mysql)); mg_free(table_name); return; }
    }
    
    strcat(fields, ", `");
    strcat(fields, field_name);
    strcat(fields, "`");
    
    strcat(values, ", '");
    strcat(values, value);
    strcat(values, "'");
    
    pch = strtok (NULL, "&");
  }
  
  sprintf(sql, "INSERT INTO `civicsets`.`p_%s` (%s) VALUES (%s)", table_name, fields, values);
  //mg_printf(conn, "%s\n", sql);
  if (mysql_query(&mysql, sql) == 0)
  {
    MYSQL_RES * res = mysql_store_result(&mysql);
    
    mysql_free_result(res);
  }
  else
    { mg_printf(conn, "insert sql error. isn't that bad?\n%s\n", mysql_error(&mysql)); mg_free(table_name); return; }
  
  mg_printf(conn, "good");
}