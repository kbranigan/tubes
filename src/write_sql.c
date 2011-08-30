
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDIN_IS_PIPED
#define SCHEME_FUNCTION write_sql
#include "scheme.h"

int write_sql(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char filename[300] = "";
  int drop_tables = 0;
  int expire_old_content = 0; // this is for nextbus
  int expire_age = 6; // default 6 hours
  
  int c;
  while ((c = getopt(argc, argv, "f:u:p:de")) != -1)
  switch (c)
  {
    case 'f':
      strncpy(filename, optarg, 300);
      break;
    case 'd':
      drop_tables = 1;
      break;
    case 'e':
      expire_old_content = 1;
      break;
    case 'a':
      expire_old_content = 1;
      expire_age = atoi(optarg);
      break;
    default:
      abort();
  }
  
  FILE * sql_out = pipe_out;
  if (strlen(filename) > 0)
  {
    sql_out = fopen(filename, "w");
    if (sql_out == NULL) { fprintf(pipe_err, "%s -f [%s] isn't a valid file.\n", argv[0], filename); exit(1); }
  }
  
  if (drop_tables)
  {
    //fprintf(sql_out, "DROP TABLE IF EXISTS DBF;\n");
    //fprintf(sql_out, "DROP TABLE IF EXISTS shape_points;\n");
    fprintf(sql_out, "DROP TABLE IF EXISTS points;\n");
  }
  
  //fprintf(sql_out, "CREATE TABLE IF NOT EXISTS DBF (dbf_id INT PRIMARY KEY AUTO_INCREMENT, unique_set_id INT, created_at DATETIME);\n");
  //fprintf(sql_out, "CREATE TABLE IF NOT EXISTS shape_points (id INT PRIMARY KEY AUTO_INCREMENT, dbf_id INT, part_id INT, x FLOAT(15, 5), y FLOAT(15, 5));\n");
  fprintf(sql_out, "CREATE TABLE IF NOT EXISTS points (id INT PRIMARY KEY AUTO_INCREMENT, created_at DATETIME, x FLOAT(15, 5), y FLOAT(15, 5), unique_set_id INT);\n");
  
  int num_fields = 0;
  char ** fields = NULL;
  int * field_lengths = NULL;
  
  int dbf_id = 1;
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    int i, j;
    
    if (expire_old_content)
      fprintf(sql_out, "DELETE FROM points WHERE created_at < now() - interval %d hour;\n", expire_age);
    
    for (i = 0 ; i < shape->num_attributes ; i++)
    {
      int found = 0;
      for (j = 0 ; j < num_fields ; j++)
      {
        if (strcmp(fields[j], shape->attributes[i].name)==0)
        {
          found = 1;
          break;
        }
      }
      if (found && shape->attributes[i].value_length > field_lengths[j])
      {
        field_lengths[j] = shape->attributes[i].value_length;
        fprintf(sql_out, "ALTER TABLE points CHANGE %s %s VARCHAR(%d);\n", shape->attributes[i].name, shape->attributes[i].name, shape->attributes[i].value_length);
      }
      if (!found)
      {
        fields = realloc(fields, sizeof(char*)*(num_fields+1));
        fields[num_fields] = malloc(strlen(shape->attributes[i].name)+1);
        field_lengths = realloc(field_lengths, sizeof(int)*(num_fields+1));
        field_lengths[num_fields] = shape->attributes[i].value_length;
        strcpy(fields[num_fields], shape->attributes[i].name);
        fprintf(sql_out, "ALTER TABLE points ADD %s VARCHAR(%d);\n", shape->attributes[i].name, shape->attributes[i].value_length);
        num_fields++;
      }
    }
    
    //shape->unique_set_id = dbf_id;
    
    //if (shape->num_vertexs == 1)
    {
      float * v = get_vertex(shape, 0, 0);
      fprintf(sql_out, "INSERT INTO points (created_at, x, y, unique_set_id");
      for (i = 0 ; i < shape->num_attributes ; i++)
        fprintf(sql_out, ", `%s`", shape->attributes[i].name);
      fprintf(sql_out, ") VALUES (NOW(), %f, %f, %d", v[0], v[1], shape->unique_set_id);
      for (i = 0 ; i < shape->num_attributes ; i++)
        fprintf(sql_out, ", \"%s\"", shape->attributes[i].value);
      fprintf(sql_out, ");\n");
    }
    /*else
    {
      fprintf(sql_out, "INSERT INTO DBF (unique_set_id, created_at");
      for (i = 0 ; i < shape->num_attributes ; i++)
        fprintf(sql_out, ", %s", shape->attributes[i].name);
      fprintf(sql_out, ") VALUES (%d, NOW()", shape->unique_set_id);
      for (i = 0 ; i < shape->num_attributes ; i++)
        fprintf(sql_out, ", \"%s\"", shape->attributes[i].value);
      fprintf(sql_out, ");\n");
    
      fprintf(sql_out, "INSERT INTO shape_points (dbf_id, x, y) VALUES ");
      for (i = 0 ; i < shape->num_vertexs ; i++)
      {
        float * v = get_vertex(shape, 0, i);
        fprintf(sql_out, "((SELECT dbf_id FROM DBF ORDER BY dbf_id DESC LIMIT 1), %f, %f)%s", v[0], v[1], ((i==shape->num_vertexs-1 || (i!=0&&i%20==0)) ? "" : ", "));
        if (i % 20 == 0 && i!=0 && i != shape->num_vertexs-1) fprintf(sql_out, ";\nINSERT INTO shape_points (dbf_id, x, y) VALUES ");
      }
      fprintf(sql_out, ";\n");
    }*/
    
    free_shape(shape);
    dbf_id ++;
  }
  
  //int i;
  //for (i = 0 ; i < num_fields ; i++)
  //  fprintf(stderr, "%d: %s\n", i, fields[i]);
}
