
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include <mysql.h>

#include "scheme.h"

int main(int argc, char *argv[])
{
  char * sql = (argc > 1) ? argv[1] : "SELECT lon AS x, lat AS y, id AS unique_set_id FROM civicsets.points where created_at > '2011-03-16 20:00:00' and source = 'gps_reporter' ORDER BY unique_set_id";
  
  if (!stdout_is_piped())
  {
    fprintf(stderr, "%s outputs binary content. Pipe it to something that can read it.\n", argv[0]);
    exit(1);
  }
  
  MYSQL mysql;
  
  if ((mysql_init(&mysql) == NULL)) { printf("mysql_init error\n"); return 0; }
  if (!mysql_real_connect(&mysql, "localhost", "root", "", "", 0, NULL, 0))
  {
    fprintf(stderr, "mysql_real_connect error (%s) (password for root should be blank)\n", mysql_error(&mysql));
    return 0;
  }
  
  if (mysql_query(&mysql, sql) == 0)
  {
	  MYSQL_RES * res = mysql_store_result(&mysql);
    MYSQL_ROW row;
    MYSQL_FIELD *field;
    
    if (!write_header(stdout, CURRENT_VERSION)) exit(1);
    
    int x_field_id = -1;
    int y_field_id = -1;
    int z_field_id = -1;
    int unique_set_field_id = -1;
    
    int i=0;
    for (i = 0 ; i < mysql_num_fields(res) ; i++)
    {
      field = mysql_fetch_field_direct(res, i);
      if (strcmp(field->name, "x") == 0) x_field_id = i;
      else if (strcmp(field->name, "y") == 0) y_field_id = i;
      else if (strcmp(field->name, "z") == 0) z_field_id = i;
      else if (strcmp(field->name, "unique_set_id") == 0) unique_set_field_id = i;
    }
    
    fprintf(stderr, "mysql_num_rows = %lld\n", mysql_num_rows(res));
    
    if (x_field_id == -1 || y_field_id == -1)
    {
      fprintf(stderr, "at least one field named 'x' and one field named 'y' is required\n");
      return 0;
    }
    if (unique_set_field_id == -1)
    {
      fprintf(stderr, "at least one integer field named 'unique_set_id' is required\n");
      return 0;
    }
    
    struct Shape * shape = NULL;
    int prev_unique_set_id = -1;
    
    while ((row = mysql_fetch_row(res)))
    {
      if (atol(row[unique_set_field_id]) != prev_unique_set_id)
      {
        if (shape != NULL)
        {
          write_shape(stdout, shape);
          free_shape(shape);
          shape = NULL;
        }
        shape = new_shape();
        shape->gl_type = GL_LINE_STRIP;
        
        shape->unique_set_id = atol(row[unique_set_field_id]);
        if (z_field_id != -1) shape->vertex_arrays[0].num_dimensions++;
      }
      struct VertexArray * va = get_array(shape, GL_VERTEX_ARRAY);
      
      if (z_field_id != -1)
        append_vertex3f(va, atof(row[x_field_id]), atof(row[y_field_id]), atof(row[z_field_id]));
      else
        append_vertex2f(va, atof(row[x_field_id]), atof(row[y_field_id]));
      
      prev_unique_set_id = atol(row[unique_set_field_id]);
    }
    if (shape != NULL)
    {
      write_shape(stdout, shape);
      free_shape(shape);
      shape = NULL;
    }
    mysql_free_result(res);
  }
  else
  {
    fprintf(stderr, "Error: %s\n", mysql_error(&mysql));
  }
  mysql_close(&mysql);
}
