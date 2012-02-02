
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "mongoose.h"
#include <mysql.h>
#include "mysql_has.h"

void map(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
  mg_printf(conn, "<html>\n<head>\n<link href=\"http://code.google.com/apis/maps/documentation/javascript/examples/default.css\" rel=\"stylesheet\" type=\"text/css\" />\n");
  mg_printf(conn, "<script type=\"text/javascript\" src=\"//maps.googleapis.com/maps/api/js?sensor=false\"></script>\n");
  mg_printf(conn, "<script type=\"text/javascript\">\n");
  
  char * table_name = mg_get_var(conn, "table");
  if (table_name == NULL) { mg_printf(conn, "'table' argument required.\n"); return; }
  
  int limit = 100;
  char * temp = mg_get_var(conn, "limit");
  if (temp != NULL) { limit = atoi(temp); mg_free(temp); }
  
  char sql[5000];
  sprintf(sql, "p_%s", table_name);
  if (!mysql_has(&mysql, "civicsets", sql, NULL) || !mysql_has(&mysql, "civicsets", sql, "lat") || !mysql_has(&mysql, "civicsets", sql, "lng"))
  {
    mg_printf(conn, "table '%s' not found\n", table_name);
    mg_free(table_name);
    return;
  }
  
  sprintf(sql, "SELECT AVG(lat), AVG(lng) FROM `civicsets`.`p_%s`", table_name);
  if (mysql_query(&mysql, sql) == 0)
  {
    MYSQL_RES * res = mysql_store_result(&mysql);
    MYSQL_ROW row = mysql_fetch_row(res);
    mg_printf(conn, "function initialize() {\n");
    mg_printf(conn, "  var myLatLng = new google.maps.LatLng(%s, %s);\n", row[0], row[1]);
    mysql_free_result(res);
  }
  
  sprintf(sql, "SELECT lat, lng FROM `civicsets`.`p_%s` ORDER BY id DESC LIMIT %d", table_name, limit);
  if (mysql_query(&mysql, sql) == 0)
  {
    int i = 0;
    MYSQL_RES * res = mysql_store_result(&mysql);
    MYSQL_ROW row;
    mg_printf(conn, "  var myOptions = { zoom: 16, center: myLatLng, mapTypeId: google.maps.MapTypeId.ROADMAP };\n");
    mg_printf(conn, "  var map = new google.maps.Map(document.getElementById(\"map_canvas\"), myOptions);\n");
    mg_printf(conn, "  var coords = [\n");
    while ((row = mysql_fetch_row(res)))
    {
      mg_printf(conn, "%s    new google.maps.LatLng(%s,%s)", (i==0)?"":",\n", row[0], row[1]);
      i++;
    }
    mg_printf(conn, "\n  ];\n");
    mg_printf(conn, "  var flightPath = new google.maps.Polyline({ path: coords, strokeColor: \"#FF0000\", strokeOpacity: 1.0, strokeWeight: 2 });\n");
    mg_printf(conn, "  flightPath.setMap(map);\n");
    mg_printf(conn, "}\n");
    mysql_free_result(res);
  }
  else
    { mg_printf(conn, "insert sql error. isn't that bad?\n%s\n", mysql_error(&mysql)); mg_free(table_name); return; }
  
  mg_printf(conn, "</script>\n</head>\n<body onload=\"initialize()\">\n<div id=\"map_canvas\"></div>\n</body>\n</html>\n");
  
  mg_free(table_name);
}

void record(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  char * table_name = mg_get_var(conn, "table");
  if (table_name == NULL) { mg_printf(conn, "'table' argument required.\n"); return; }
  
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
  mg_free(table_name);
}