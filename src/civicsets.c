
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <mysql.h>

//#include "shapefile_src/shapefil.h"
#include "mongoose.h"
#include "civicsets_shapefiles.c"
#include "civicsets_ttc.c"
#include "civicsets_record.c"
#include "civicsets_sources.c"

MYSQL mysql;

const char *port = "2222";

char * cwd = NULL;


/*struct Command {
  char * name;
  char ** arguments;
  unsigned int num_arguments;
};

void run(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  srand ( time(NULL) );
  
  mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
  
  mg_printf(conn, "<html>\n<head>\n<title>civicsets.ca</title>\n</head>\n<body>\n<br />\n");
  
  char *buf;
  char *cwd;
  long cwd_length = pathconf(".", _PC_PATH_MAX);
  
  if ((buf = (char *)malloc((size_t)cwd_length)) != NULL)
    cwd = getcwd(buf, (size_t)cwd_length);
  
  //c#      // command #
  //  a#_?  // command #  argument ?
  
  struct Command * commands = NULL;
  unsigned int num_commands = 0;
  
  do
  {
    char fname[50];
    sprintf(fname, "c%d", num_commands);
    char * command_name = mg_get_var(conn, fname);
    if (command_name == NULL) break; // all done
    
    num_commands ++;
    commands = (struct Command*)realloc(commands, sizeof(struct Command)*num_commands);
    struct Command * command = &commands[num_commands-1];
    command->name = command_name;
    command->num_arguments = 0;
    command->arguments = NULL;
    
    do
    {
      sprintf(fname, "a%d_%d", num_commands-1, command->num_arguments);
      char * argument = mg_get_var(conn, fname);
      if (argument == NULL) break; // all done
      
      command->num_arguments ++;
      command->arguments = (char**)realloc(command->arguments, sizeof(char*)*command->num_arguments);
      command->arguments[command->num_arguments-1] = argument;
      
    } while (1);
    
  } while (1);
  
  char * command_full = (char*)malloc(cwd_length + 7);
  sprintf(command_full, "cd %s ; ", cwd);
  
  unsigned int i = 0;
  for (i = 0 ; i < num_commands ; i++)
  {
    struct Command * command = &commands[i];
    printf("%d: %s\n", i, command->name);
    
    if (i > 0) command_full = (char*)realloc(command_full, strlen(command_full) + 3);
    if (i > 0) strcat(command_full, " | ");
    
    if (strncmp(command->name, "bin/write_", 6) == 0 && command->num_arguments == 0)
    {
      char temp[20];
      sprintf(temp, " cache/%d.%s", rand(), command->name+10);
      mg_printf(conn, "<a href='%s'>%s</a><br />\n", temp, temp);
      command_full = (char*)realloc(command_full, strlen(command_full) + 2 + strlen(command->name) + 10 + strlen(temp));
      strcat(command_full, "./");
      strcat(command_full, command->name);
      strcat(command_full, temp);
    }
    else if (strncmp(command->name, "bin/", 4) == 0)
    {
      command_full = (char*)realloc(command_full, strlen(command_full) + 2 + strlen(command->name) + 1);
      strcat(command_full, command->name);
    }
    else if (strcmp(command->name, "cat") == 0 && command->num_arguments == 1)
    {
      command_full = (char*)realloc(command_full, strlen(command_full) + strlen(command->name) + 6 + strlen(command->arguments[0]));
      strcat(command_full, command->name);
      strcat(command_full, " data/");
      strcat(command_full, command->arguments[0]);
    }
    else
    {
      unsigned int j = 0;
      for (j = 0 ; j < command->num_arguments ; j++)
      {
        
      }
      mg_printf(conn, "%s is no good. SOWWY <br />\n", command->name);
    }
    
    unsigned int j = 0;
    for (j = 0 ; j < command->num_arguments ; j++)
    {
      printf("  %d: %s\n", j, command->arguments[j]);
      free(command->arguments[j]);
    }
    free(command->name);
    free(command->arguments);
  }
  free(commands);
  
  system(command_full);
  printf("%s\n", command_full);
  free(command_full);
}*/

void record_a_position(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  char * recorded_at_c = mg_get_var(conn, "recorded_at");
  long recorded_at = (recorded_at_c == NULL) ? 0 : atoi(recorded_at_c);
  
  char * source_c = mg_get_var(conn, "source");
  
  char * lat_c = mg_get_var(conn, "lat");
  double lat = (lat_c == NULL) ? 0 : atof(lat_c);
  
  char * lon_c = mg_get_var(conn, "lon");
  double lon = (lon_c == NULL) ? 0 : atof(lon_c);
  
  char * altitude_c = mg_get_var(conn, "altitude");
  double altitude = (altitude_c == NULL) ? 0 : atof(altitude_c);
  
  char * speed_c = mg_get_var(conn, "speed");
  double speed = (speed_c == NULL) ? 0 : atof(speed_c);
  
  char * course_c = mg_get_var(conn, "course");
  double course = (course_c == NULL) ? 0 : atof(course_c);
  
  char * heading_c = mg_get_var(conn, "heading");
  double heading = (heading_c == NULL) ? 0 : atof(heading_c);
  
  char * haccuracy_c = mg_get_var(conn, "haccuracy");
  double haccuracy = (haccuracy_c == NULL) ? 0 : atof(haccuracy_c);
  
  char * vaccuracy_c = mg_get_var(conn, "vaccuracy");
  double vaccuracy = (vaccuracy_c == NULL) ? 0 : atof(vaccuracy_c);
  
  char query[1000];
  sprintf(query, "INSERT INTO points (created_at, recorded_at, source, lat, lon, altitude, speed, course, haccuracy, vaccuracy, heading) values (NOW(), %ld, '%s', %f, %f, %f, %f, %f, %f, %f, %f)", recorded_at, source_c, lat, lon, altitude, speed, course, haccuracy, vaccuracy, heading);
  mg_printf(conn, "%s", query);
  mysql_query(&mysql, query);
  //mysql_close(&mysql);
  
  free(source_c);
  free(recorded_at_c);
  free(lat_c);
  free(lon_c);
}

int main(int argc, char **argv)
{
  srand(time(NULL));
  
  if ((mysql_init(&mysql) == NULL)) { printf("mysql_init error\n"); }
  if (!mysql_real_connect(&mysql, "localhost", "root", "", "civicsets", 0, NULL, 0)) { printf("mysql_real_connect error\n"); }
  
  //mysql_query(&mysql, "TRUNCATE TABLE datasets");
  
  long size = pathconf(".", _PC_PATH_MAX);
  char *buf;
  
  if ((buf = (char *)malloc((size_t)size)) != NULL)
    cwd = getcwd(buf, (size_t)size);
  
  struct mg_context *ctx = mg_start();
  mg_set_option(ctx, "dir_list", "no");  // Set document root
  int ret = 0;
  ret = mg_set_option(ctx, "max_threads", "1"); // just you know, easier to work with
  ret = mg_set_option(ctx, "ports", port);
  while (ret != 1)
  {
    sleep(1);
    ret = mg_set_option(ctx, "ports", port);
  }
  
  mg_set_uri_callback(ctx, "/shapefiles", &shapefiles_list, NULL);
  mg_set_uri_callback(ctx, "/shapefiles/png", &shapefiles_png, NULL);
  mg_set_uri_callback(ctx, "/shapefiles/fields", &shapefiles_fields, NULL);
  
  //mg_set_uri_callback(ctx, "/run", &run, NULL);
  mg_set_uri_callback(ctx, "/ttc_shape", &ttc_shape, NULL);
  mg_set_uri_callback(ctx, "/ttc_route", &ttc_route, NULL);
  mg_set_uri_callback(ctx, "/ttc_route_realtime", &ttc_route_realtime, NULL);
  mg_set_uri_callback(ctx, "/ttc_performance", &ttc_performance, NULL);
  mg_set_uri_callback(ctx, "/ttc_performance_image", &ttc_performance_image, NULL);
  
  mg_set_uri_callback(ctx, "/map", &map, NULL);
  mg_set_uri_callback(ctx, "/record", &record, NULL);
  
  setup_sources(ctx);
  
  //mg_set_uri_callback(ctx, "/fields", &fields, NULL);
  //mg_set_uri_callback(ctx, "/shapes", &shapes, NULL);
  //mg_set_uri_callback(ctx, "/records", &records, NULL);
  //mg_set_uri_callback(ctx, "/record_a_position", &record_a_position, NULL);
  
  printf("[http server on port:%s]\n", port);
  
  for (;;) 
  {
    
    sleep(10);
  }
  mg_stop(ctx);
}
