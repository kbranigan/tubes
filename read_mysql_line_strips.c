
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include <mysql.h>

/*
usage: ./read_mysql_line_strips [database] [table] [group_field] [order_field] [x_field] [y_field] [z_field] | ./[something_else]

example:

./read_mysql_line_strips ttc shape_points shape_id position lat lng 0 | ./make_jpg

group_field becomes the unique_id, it currently needs to be an integer

*/

#include <OpenGL/gl.h>

int main(int argc, char *argv[])
{
  char * database    = (argc > 1) ? argv[1] : "ttc";
  char * table       = (argc > 2) ? argv[2] : "shape_points";
  char * group_field = (argc > 3) ? argv[3] : "shape_id";
  char * order_field = (argc > 4) ? argv[4] : "position";
  char * x_field     = (argc > 5) ? argv[5] : "lng";
  char * y_field     = (argc > 6) ? argv[6] : "lat";
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
    
    uint32_t file_header = 42;
    assert(fwrite(&file_header, sizeof(file_header), 1, stdout) == 1);
    
    //uint32_t num_shapes = mysql_num_rows(res);
    //assert(fwrite(&num_shapes, sizeof(num_shapes), 1, stdout) == 1);
    
    while ((row = mysql_fetch_row(res)))
    {
      uint32_t unique_set_id = atoi(row[0]);
      
      sprintf(query, "SELECT %s, %s, %s FROM %s.%s WHERE %s = '%s' ORDER BY %s, id", x_field, y_field, z_field, database, table, group_field, row[0], order_field);
      if (mysql_query(&mysql, query) == 0)
      {
    	  MYSQL_RES * res2 = mysql_store_result(&mysql);
        MYSQL_ROW row2;
        
        double inf = 1.0 / 0.0;
        assert(fwrite(&inf, sizeof(inf), 1, stdout) == 1);
        assert(fwrite(&unique_set_id, sizeof(unique_set_id), 1, stdout) == 1);
        
        char name[150];
        memset(name, 0, sizeof(name));
        sprintf(name, "%s.%s.%s = '%s'", database, table, group_field, row[0]);
        assert(fwrite(name, sizeof(name), 1, stdout) == 1);
        
        uint32_t frame_type = GL_LINE_STRIP;
        assert(fwrite(&frame_type, sizeof(frame_type), 1, stdout) == 1);
        
        uint32_t vertex_type = GL_VERTEX_ARRAY;
        assert(fwrite(&vertex_type, sizeof(vertex_type), 1, stdout) == 1);
        
        uint32_t number_of_vertexes = mysql_num_rows(res2);
        assert(fwrite(&number_of_vertexes, sizeof(number_of_vertexes), 1, stdout) == 1);
        
        while ((row2 = mysql_fetch_row(res2)))
        {
          double x = atof(row2[0]);
          double y = atof(row2[1]);
          double z = 0.0;
          
          assert(fwrite(&x, sizeof(double), 1, stdout) == 1);
          assert(fwrite(&y, sizeof(double), 1, stdout) == 1);
          assert(fwrite(&z, sizeof(double), 1, stdout) == 1);
        }
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
