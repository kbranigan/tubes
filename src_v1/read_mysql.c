
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include <mysql.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDOUT_IS_PIPED
#define SCHEME_FUNCTION read_mysql
#include "scheme.h"

int read_mysql(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char * sql = (argc > 1) ? argv[1] : "SELECT lon AS x, lat AS y, id AS unique_set_id FROM civicsets.points where created_at > '2011-03-16 15:00:00' and source = 'gps_reporter' ORDER BY unique_set_id";
  
  //exec("tail -n1 read_mysql.sql.log");
  
  char sql_log_filename[100];
  sprintf(sql_log_filename, "%s.sql.log", argv[0]);
  FILE * sql_log = fopen(sql_log_filename, "a");
  if (sql_log != NULL)
  {
    fprintf(sql_log, "%s\n", sql);
    fclose(sql_log);
  }
  else
    fprintf(stderr, "failed to open sql log file '%s'\n", sql_log_filename);
  
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
    
    int x_field_id = -1;
    int y_field_id = -1;
    int z_field_id = -1;
    int unique_set_field_id = -1;
    
    int r_field_id = -1;
    int g_field_id = -1;
    int b_field_id = -1;
    int a_field_id = -1;
    
    int i=0;
    for (i = 0 ; i < mysql_num_fields(res) ; i++)
    {
      field = mysql_fetch_field_direct(res, i);
      if (strcmp(field->name, "x") == 0) x_field_id = i;
      else if (strcmp(field->name, "y") == 0) y_field_id = i;
      else if (strcmp(field->name, "z") == 0) z_field_id = i;
      else if (strcmp(field->name, "r") == 0) r_field_id = i;
      else if (strcmp(field->name, "g") == 0) g_field_id = i;
      else if (strcmp(field->name, "b") == 0) b_field_id = i;
      else if (strcmp(field->name, "a") == 0) a_field_id = i;
      else if (strcmp(field->name, "unique_set_id") == 0) unique_set_field_id = i;
    }
    if (unique_set_field_id == -1)
    {
      for (i = 0 ; i < mysql_num_fields(res) ; i++)
      {
        field = mysql_fetch_field_direct(res, i);
        if (strcmp(field->name, "id") == 0) unique_set_field_id = i;
      }
    }
    
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
      if (row[unique_set_field_id] == NULL) continue;
      if (atol(row[unique_set_field_id]) != prev_unique_set_id)
      {
        if (shape != NULL)
        {
          if (shape->num_vertexs == 1) shape->gl_type = GL_POINTS;
          write_shape(pipe_out, shape);
          free_shape(shape);
          shape = NULL;
        }
        shape = new_shape();
        shape->gl_type = GL_LINE_STRIP;
        
        shape->unique_set_id = atol(row[unique_set_field_id]);
        if (z_field_id != -1) set_num_dimensions(shape, 0, 3); //shape->vertex_arrays[0].num_dimensions++;
        
        //get_or_add_array(shape, GL_VERTEX_ARRAY); // this is auto, but you, helps doc the code
        
        if (r_field_id != -1 &&
            g_field_id != -1 &&
            b_field_id != -1)
        {
          get_or_add_array(shape, GL_COLOR_ARRAY);
          if (a_field_id != -1) set_num_dimensions(shape, 1, 4);
        }
        
        for (i = 0 ; i < mysql_num_fields(res) ; i++)
        {
          field = mysql_fetch_field_direct(res, i);
          if (strcmp(field->name, "x") != 0 && strcmp(field->name, "y") != 0 && strcmp(field->name, "z") != 0 && 
              strcmp(field->name, "r") != 0 && strcmp(field->name, "g") != 0 && strcmp(field->name, "b") != 0 && strcmp(field->name, "a") != 0 && 
              strcmp(field->name, "id") != 0 && strcmp(field->name, "unique_set_id") != 0)
            set_attribute(shape, field->name, row[i]);
        }
      }
      
      float v[3] = { row[x_field_id]==NULL ? 0 : atof(row[x_field_id]), row[y_field_id]==NULL ? 0 : atof(row[y_field_id]), 0.0 };
      
      if (z_field_id != -1 && row[z_field_id])
        v[2] = atof(row[z_field_id]);
      
      if (r_field_id != -1 &&
        g_field_id != -1 &&
        b_field_id != -1)
      {
        float v2[4] = { atof(row[r_field_id]), atof(row[g_field_id]), atof(row[b_field_id]), 0 };
        if (a_field_id != -1) v2[3] = atof(row[a_field_id]);
        append_vertex2(shape, v, v2);
      }
      else
        append_vertex(shape, v);
      
      prev_unique_set_id = atol(row[unique_set_field_id]);
    }
    if (shape != NULL)
    {
      if (shape->num_vertexs == 1) shape->gl_type = GL_POINTS;
      write_shape(pipe_out, shape);
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
