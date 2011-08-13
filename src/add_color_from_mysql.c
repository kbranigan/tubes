
#include <stdio.h>
#include <stdlib.h>

#include <mysql.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION add_color_from_mysql
#include "scheme.h"

int add_color_from_mysql(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  MYSQL mysql;
  
  if ((mysql_init(&mysql) == NULL)) { printf("mysql_init error\n"); return 0; }
  if (!mysql_real_connect(&mysql, "localhost", "root", "", "ADDRESS_POINT_WGS84", 0, NULL, 0))
  {
    fprintf(stderr, "mysql_real_connect error (%s) (password for root should be blank)\n", mysql_error(&mysql));
    return 0;
  }
  
  int min_cost = 0;
  int max_cost = 0;
  
  char sql[500] = "SELECT MIN(cost), MAX(cost) FROM results WHERE exec_id = 1096411009";
  if (mysql_query(&mysql, sql) == 0)
  {
    MYSQL_RES * res = mysql_store_result(&mysql);
    MYSQL_ROW row;
    row = mysql_fetch_row(res);
    min_cost = atoi(row[0]);
    max_cost = atoi(row[1]);
    mysql_free_result(res);
  }
  
  int count_bad = 0;
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    sprintf(sql, "SELECT cost FROM results WHERE dbf_id = %d AND exec_id = 1096411009 LIMIT 1", shape->unique_set_id);
    if (mysql_query(&mysql, sql) == 0)
    {
  	  MYSQL_RES * res = mysql_store_result(&mysql);
  	  MYSQL_ROW row = mysql_fetch_row(res);
  	  
      if (shape->num_vertex_arrays != 1 || shape->num_vertexs != 1) { fprintf(stderr, "shape->num_vertex_arrays != 1 || shape->num_vertexs != 1\n"); exit(1); }
      shape->num_vertex_arrays ++;
      shape->vertex_arrays = realloc(shape->vertex_arrays, sizeof(struct VertexArray)*shape->num_vertex_arrays);
      struct VertexArray *va = &shape->vertex_arrays[1];
      va->array_type = GL_COLOR_ARRAY;
      va->num_dimensions = 3;
      va->vertexs = (float*)malloc(sizeof(float)*va->num_dimensions*shape->num_vertexs);
      if (row == NULL || row[0] == NULL)
      {
        va->vertexs[0] = 1;
        va->vertexs[1] = 0;
        va->vertexs[2] = 0;
        count_bad ++;
      }
      else
      {
        va->vertexs[0] = atof(row[0]) / (float)(max_cost - min_cost);
        va->vertexs[1] = atof(row[0]) / (float)(max_cost - min_cost);
        va->vertexs[2] = atof(row[0]) / (float)(max_cost - min_cost);
      }
      mysql_free_result(res);
    }
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
  fprintf(stderr, "count_bad = %d\n", count_bad);
}








