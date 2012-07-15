
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "mongoose.h"
#include <mysql.h>
#include "mysql_has.h"

void sources(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
  
  if (mysql_query(&mysql, "SELECT replace(name, '''', '\\'') as name, replace(source_url, '''', '\\'') as source_url FROM `civicsets`.`sources`") == 0)
  {
    MYSQL_RES * res = mysql_store_result(&mysql);
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
    {
      mg_printf(conn, "<a href='%s'>%s</a><br />", row[0]);
    }
    mysql_free_result(res);
  }
}

void add_source(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
  if (strcmp(ri->request_method, "GET")==0)
  {
    mg_printf(conn, "<form method='POST'><table>");
    mg_printf(conn, "<tr><td>Name:</td><td><input type='text' name='name' style='width:200px'></td></tr>");
    mg_printf(conn, "<tr><td>Source URL:</td><td><input type='text' name='source_url' style='width:200px'></td></tr>");
    mg_printf(conn, "<tr><td>Publisher:</td><td><input type='text' name='publisher' style='width:200px'></td></tr>");
    mg_printf(conn, "<tr><td>Published Date:</td><td><input type='text' name='published_date' style='width:200px'></td></tr>");
    mg_printf(conn, "<tr><td>File Format:</td><td><input type='text' name='file_format' value='shapefile' style='width:200px'></td></tr>");
    mg_printf(conn, "<tr><td colspan=2><input type='submit' value='Post File'></td></tr>");
    mg_printf(conn, "</table></form>");
  }
  else if (strcmp(ri->request_method, "POST")==0)
  {
    mg_printf(conn, "thants!\n");
  }
}

void setup_sources(struct mg_context *ctx)
{
  mg_set_uri_callback(ctx, "/sources", &sources, NULL);
  mg_set_uri_callback(ctx, "/sources/new", &add_source, NULL);
}