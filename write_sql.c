
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
  int c;
  while ((c = getopt(argc, argv, "f:")) != -1)
  switch (c)
  {
    case 'f':
      strncpy(filename, optarg, 300);
      break;
    default:
      abort();
  }
  
  FILE * sql_out = fopen(filename, "w");
  if (sql_out == NULL) { fprintf(pipe_err, "%s -f [%s] isn't a valid file.\n", argv[0], filename); exit(1); }
  
  fprintf(sql_out, "DROP TABLE DBF;\nCREATE TABLE DBF (dbf_id INT PRIMARY KEY AUTO_INCREMENT);\n");
  fprintf(sql_out, "DROP TABLE shape_points;\nCREATE TABLE shape_points (id INT PRIMARY KEY AUTO_INCREMENT, dbf_id INT, part_id INT, x FLOAT(15, 5), y FLOAT(15, 5));\n");
  
  int num_fields = 0;
  char ** fields = NULL;
  
  int dbf_id = 1;
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    int i, j;
    
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
      if (!found)
      {
        fields = realloc(fields, sizeof(char*)*(num_fields+1));
        fields[num_fields] = malloc(strlen(shape->attributes[i].name)+1);
        strcpy(fields[num_fields], shape->attributes[i].name);
        fprintf(sql_out, "ALTER TABLE DBF ADD %s VARCHAR(%d);\n", shape->attributes[i].name, shape->attributes[i].value_length);
        num_fields++;
      }
    }
    
    shape->unique_set_id = dbf_id;
    
    fprintf(sql_out, "INSERT INTO DBF (dbf_id");
    for (i = 0 ; i < shape->num_attributes ; i++)
      fprintf(sql_out, ", %s", shape->attributes[i].name);
    fprintf(sql_out, ") VALUES (%d", shape->unique_set_id);
    for (i = 0 ; i < shape->num_attributes ; i++)
      fprintf(sql_out, ", \"%s\"", shape->attributes[i].value);
    fprintf(sql_out, ");\n");
    
    fprintf(sql_out, "INSERT INTO shape_points (dbf_id, x, y) VALUES ");
    for (i = 0 ; i < shape->num_vertexs ; i++)
    {
      float * v = get_vertex(shape, 0, i);
      fprintf(sql_out, "(%d, %f, %f)%s", shape->unique_set_id, v[0], v[1], ((i==shape->num_vertexs-1 || (i!=0&&i%20==0)) ? "" : ", "));
      if (i % 20 == 0 && i!=0 && i != shape->num_vertexs-1) fprintf(sql_out, ";\nINSERT INTO shape_points (dbf_id, x, y) VALUES ");
    }
    fprintf(sql_out, ";\n");
    
    free_shape(shape);
    dbf_id ++;
  }
  
  //int i;
  //for (i = 0 ; i < num_fields ; i++)
  //  fprintf(stderr, "%d: %s\n", i, fields[i]);
}
