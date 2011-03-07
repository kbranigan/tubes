
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include <mysql.h>

/*
usage: ./read_mysql_shapes [database] [table] [group_field] [order_field] [x_field] [y_field] [z_field] | ./[something_else]

example:

./read_mysql_shapes ttc shape_points shape_id position lat lng 0 | ./make_jpg

group_field becomes the unique_id, it currently needs to be an integer

*/

#include "scheme.h"

int main(int argc, char *argv[])
{
  char * database    = (argc > 1) ? argv[1] : "icitw_wgs84";
  char * table       = (argc > 2) ? argv[2] : "shape_points";
  char * group_field = (argc > 3) ? argv[3] : "dbf_id";
  char * order_field = (argc > 4) ? argv[4] : "id";
  char * x_field     = (argc > 5) ? argv[5] : "x";
  char * y_field     = (argc > 6) ? argv[6] : "y";
  char * z_field     = (argc > 7) ? argv[7] : "0";
  
  if (!stdout_is_piped())
  {
    fprintf(stderr, "%s outputs binary content. Pipe it to something that can read it.\n", argv[0]);
    exit(1);
  }
  
  MYSQL mysql;
  
  if ((mysql_init(&mysql) == NULL)) { printf("mysql_init error\n"); return 0; }
  if (!mysql_real_connect(&mysql, "localhost", "root", "", database, 0, NULL, 0))
  {
    fprintf(stderr, "mysql_real_connect error (%s) (password for root should be blank)\n", mysql_error(&mysql));
    return 0;
  }
  
  char query[500];
  sprintf(query, "SELECT `TABLE_NAME` FROM `INFORMATION_SCHEMA`.`COLUMNS` WHERE `TABLE_SCHEMA` = '%s' AND `TABLE_NAME` = '%s' AND `COLUMN_NAME` IN ('%s', '%s')", database, table, group_field, order_field);
  if (mysql_query(&mysql, query) == 0)
  {
    MYSQL_RES * res = mysql_store_result(&mysql);
    if (mysql_num_rows(res) != 2)
    {
      fprintf(stderr, "%s\n%s.%s.%s or .%s does not exist in mysql.\n", query, database, table, group_field, order_field);
      exit(1);
    }
    mysql_free_result(res);
  }
  else
  {
    fprintf(stderr, "%s\n%s\n", query, mysql_error(&mysql));
    exit(1);
  }
  
  sprintf(query, "SELECT %s, COUNT(*) c FROM %s.%s WHERE %s IS NOT NULL GROUP BY %s", group_field, database, table, group_field, group_field);
  if (mysql_query(&mysql, query) == 0)
  {
	  MYSQL_RES * res = mysql_store_result(&mysql);
    MYSQL_ROW row;
    
    if (!write_header(stdout, CURRENT_VERSION)) exit(1);
    
    while ((row = mysql_fetch_row(res)))
    {
      struct Shape * shape = (struct Shape*)malloc(sizeof(struct Shape));
      memset(shape, 0, sizeof(struct Shape));
      
      shape->unique_set_id = atoi(row[0]);
      
      sprintf(query, "SELECT %s, %s, %s FROM %s.%s WHERE %s = '%s' ORDER BY %s, id", x_field, y_field, z_field, database, table, group_field, row[0], order_field);
      if (mysql_query(&mysql, query) == 0)
      {
    	  MYSQL_RES * res2 = mysql_store_result(&mysql);
        MYSQL_ROW row2;
        
        shape->gl_type = GL_LINE_LOOP;
        
        shape->num_attributes = 0;
        shape->num_vertexs = mysql_num_rows(res2);
        shape->num_vertex_arrays = 1;
        
        shape->attributes = NULL;
        shape->vertex_arrays = (struct VertexArray*)malloc(sizeof(struct VertexArray));
        
        struct VertexArray * va = &shape->vertex_arrays[0];
        memset(va, 0, sizeof(struct VertexArray));
        
        va->array_type = GL_VERTEX_ARRAY;
        va->num_dimensions = 3;
        va->vertexs = (float*)malloc(sizeof(float) * shape->num_vertexs * va->num_dimensions);
        
        { // num_vertex_arrays = 1;
          
          // num_vertexs
          long j = 0;
          while ((row2 = mysql_fetch_row(res2)))
          {
            // num_dimensions
            va->vertexs[j++] = atof(row2[0]);
            va->vertexs[j++] = atof(row2[1]);
            va->vertexs[j++] = 0.0;
          }
        }
        if (write_shape(stdout, shape))
        {
          fprintf(stderr, "fwrite error\n");
          exit(1);
        }
        free(shape);
        mysql_free_result(res2);
      }
      else
      {
        fprintf(stderr, "%s\n%s\n", query, mysql_error(&mysql));
        exit(1);
      }
    }
    mysql_free_result(res);
  }
  else
  {
    fprintf(stderr, "%s\n%s\n", query, mysql_error(&mysql));
    exit(1);
  }
  
  mysql_close(&mysql);
  return 0;
}
