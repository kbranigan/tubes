
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDIN_IS_PIPED
#define SCHEME_FUNCTION add_color_from_csv
#include "scheme.h"

struct ColorRule {
  int unique_set_id;
  char name[20];
  char value[200];
  int def; // default is a keyword
  float r,g,b,a;
};

int add_color_from_csv(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  /*
  example csv file:

TYPE:O,red:0.5,green:0.5,blue:0.8
TYPE:W,red:0.5,green:0.6,blue:0.8,alpha:1.0
TYPE:L,red:1.0,green:0.95,blue:0.7,alpha:1.0
unique_set_id:-1,red:0.5,green:0.6,blue:0.8,alpha:1.0

you can specify the unique id, or any arbitary name:value pair - which is matched to shape attributes
  
  */
  
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
  
  int num_rules = 0;
  struct ColorRule * rules = NULL;
  int def_id = -1;
  
  FILE * fp = fopen(filename, "r");
  if (fp == NULL) { fprintf(pipe_err, "%s -f [%s] isn't a valid file.\n", argv[0], filename); exit(1); }
  char buf[500];
  while (fgets(buf, sizeof(buf), fp))
  {
    if (strlen(buf) < 2) continue;
    
    int num_parts = 0;
    char * parts[50];
    char * pch = strtok (buf, ",\n");
    while (pch != NULL)
    {
      parts[num_parts] = pch;
      num_parts++;
      pch = strtok (NULL, ",\n");
    }
    
    rules = realloc(rules, sizeof(struct ColorRule)*(num_rules+1));
    struct ColorRule * rule = &rules[num_rules];
    num_rules++;
    memset(rule, 0, sizeof(struct ColorRule));
    rule->unique_set_id = -1;
    rule->a = 1.0;
    rule->def = 0;
    
    int i;
    for (i = 0 ; i < num_parts ; i++)
    {
      pch = strpbrk(parts[i], ":\n\t");
      if (pch == NULL) continue;
      *(pch++) = 0;
      
      if (strcmp(parts[i], "unique_set_id")==0) rule->unique_set_id = atoi(pch);
      else if (strcmp(parts[i], "red")==0) rule->r = atof(pch);
      else if (strcmp(parts[i], "green")==0) rule->g = atof(pch);
      else if (strcmp(parts[i], "blue")==0) rule->b = atof(pch);
      else if (strcmp(parts[i], "alpha")==0) rule->a = atof(pch);
      else if (strcmp(parts[i], "default")==0) { rule->def = 1; def_id = i; }
      else
      {
        strncpy(rule->name, parts[i], 20);
        strncpy(rule->value, pch, 200);
      }
    }
  }
  fclose(fp);
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(pipe_in)))
  {
    struct ColorRule * rule = NULL;
    
    int i,j;
    for (i = 0 ; i < num_rules ; i++)
    {
      struct ColorRule * temp_rule = &rules[i];
      if (temp_rule->unique_set_id != -1 && temp_rule->unique_set_id == shape->unique_set_id)
        rule = temp_rule;
      else if (temp_rule->name[0] != 0)
        for (j = 0 ; j < shape->num_attributes ; j++)
          if (strcmp(shape->attributes[j].name, temp_rule->name) == 0 && strcmp(shape->attributes[j].value, temp_rule->value) == 0)
            rule = temp_rule; //add_color = 1;
    }
    
    if (rule)
    {
      struct VertexArray * cva = get_or_add_array(shape, GL_COLOR_ARRAY);
      cva->num_dimensions = 4;
      cva->vertexs = realloc(cva->vertexs, sizeof(float)*shape->num_vertexs*cva->num_dimensions);
      
      float color[4] = { rule->r, rule->g, rule->b, rule->a };
      for (j = 0 ; j < shape->num_vertexs ; j++)
        set_vertex(shape, 1, j, color);
    }
    else 
    {
      //if (strcmp(shape->attributes[j].name, temp_rule->name) == 0)
      fprintf(stderr, "%s\n", get_attribute(shape, "COMMONAME1"));
      /*for (i = 0 ; i < num_rules ; i++)
      {
        struct ColorRule * temp_rule = &rules[i];
        for (j = 0 ; j < shape->num_attributes ; j++)
          if (strcmp(shape->attributes[j].name, temp_rule->name) == 0 && strcmp(shape->attributes[j].value, temp_rule->value) == 0)
          {
            
          }
          else
          {
            //if (strcmp(shape->attributes[j].name, temp_rule->name) == 0)
            //fprintf(stderr, "%s vs %s = %d\n", shape->attributes[j].value, temp_rule->value, strcmp(shape->attributes[j].value, temp_rule->value));
          }
      }*/
      
      //for (j = 0 ; j < shape->num_attributes ; j++)
      //  fprintf(stderr, "%s:\"%s\"\n", shape->attributes[j].name, shape->attributes[j].value);
        //if (strcmp(shape->attributes[j].name, rule->name) == 0 && strcmp(shape->attributes[j].value, rule->value) == 0)
      
      //char * value = get_attribute(shape, "COMMONAME1");
      //fprintf(stderr, "shape dont got a color (%s)\n", value);
      //exit(1);
    }
    
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
}
