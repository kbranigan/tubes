
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

/*void record_a_position(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
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
}*/

void get_image_name(char * temp)
{
  sprintf(temp, "cache_images/ttc_routes/%d.png", rand());
}

void output_and_delete_image(struct mg_connection *conn, char * filename)
{
  mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nConnection: close\r\n\r\n");
  
  FILE * fp = fopen(filename, "r");
  if (fp != NULL)
  {
    int actual_size = 0;
    int malloc_size = 0;
    char * data = NULL;
    int c;
    do {
      c = fgetc(fp);
      if (c != EOF)
      {
        actual_size ++;
        if (actual_size > malloc_size)
        {
          malloc_size += 1000;
          data = (char *)realloc(data, malloc_size);
        }
        data[actual_size-1] = c;
      }
    } while (c != EOF);
    
    mg_write(conn, data, actual_size);
    fclose(fp);
    free(data);
    
    //char command[1000];
    //sprintf(command, "rm %s", filename);
    //system(command);
  }
}

void ttc_performance(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  char * routeTag = mg_get_var(conn, "routeTag");
  if (routeTag == NULL) { mg_printf(conn, "You need to specify a routeTag [like 125, 60 or 510]<br />"); return; }
  
  char * dirTag = mg_get_var(conn, "dirTag");
  if (dirTag == NULL) { mg_printf(conn, "You need to specify a dirTag [like 125_0_125, 60_1_60D or 510_0_510]<br />"); return; }
  
  char * shape_id = mg_get_var(conn, "shape_id");
  if (shape_id == NULL) { mg_printf(conn, "You need to specify a shape_id, from the ttc_gtfs database [like 192, 1027 or something]<br />"); return; }
  
  mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
  
  mg_printf(conn, "<body style='font-family:Arial'> \
    <br /> \
    Scrolling down goes back in time.<br /> \
    The top of the image is most recent, the bottom is 3 hours previous.<br /> \
    <br /> \
    The far left is where the vehicle begins, the far right when it arrives at the end of the route. <br /> \
    The vertical ticks are indicators that the GPS in the bus updated at that point.<br /> \
    <br /> \
    This is '%s' TTC vehicle (bus or streetcar). <br /> \
    Looking closely, you might be able to see where this vehicle has difficulty going quickly, or stops regularly.<br /> \
    The solid red line, the horizontal one - indicates now. Ticks attached to this line are where the vehicle is scheduled to stop.<br /> \
    <br /> \
    This is generated the second you hit the page, so bare with it if it seems slow. \
    <br /> \
    If you see nothing under the red line, service is not currently running for this route. \
    <br /> \
    <br /> \
    ------ DRIVING FORWARD -----><br /> \
    <img src='/ttc_performance_image?routeTag=%s&dirTag=%s&shape_id=%s' />", routeTag, routeTag, dirTag, shape_id);
  
  free(routeTag);
  free(dirTag);
  free(shape_id);
}

void ttc_performance_image(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  char * routeTag = mg_get_var(conn, "routeTag");
  if (routeTag == NULL) { mg_printf(conn, "You need to specify a routeTag [like 125, 60 or 510]<br />"); return; }
  
  char * dirTag = mg_get_var(conn, "dirTag");
  if (dirTag == NULL) { mg_printf(conn, "You need to specify a dirTag [like 125_0_125, 60_1_60D or 510_0_510]<br />"); return; }
  
  char * shape_id = mg_get_var(conn, "shape_id");
  if (shape_id == NULL) { mg_printf(conn, "You need to specify a shape_id, from the ttc_gtfs database [like 192, 1027 or something]<br />"); return; }
  
  char image[200];
  get_image_name(image);
  
  char command[4000];
  
  int nextbus_rand_int = rand();
  
  sprintf(command, "./bin/read_mysql \"SELECT lat as y, lng as x, shape_id as unique_set_id from ttc_gtfs.shape_points where shape_id = %s order by shape_id, position\" \
    > ttc_shape_points_%d.b", shape_id, nextbus_rand_int);
  
  fprintf(stderr, "%s\n", command);
  system(command);
  
  sprintf(command, "./bin/read_mysql \"select x, y, id, -1 * (unix_timestamp() - unix_timestamp(created_at) - secsSinceReport) as reported_at, unique_set_id as vehicle_number, dirTag from nextbus.points where routeTag = %s and round(secsSinceReport) < 6 order by dirTag, unique_set_id, created_at\" \
    | ./bin/reduce_by_attribute -n dirTag -v %s \
    | ./bin/align_points_to_line_strips -f ttc_shape_points_%d.b \
    | ./bin/graph_ttc_performance -a dist_line_%s \
    > nextbus_temp_%d.b ", routeTag, dirTag, nextbus_rand_int, shape_id, nextbus_rand_int);
  
  fprintf(stderr, "%s\n", command);
  system(command);
  
  sprintf(command, "./bin/read_mysql \"select 1 as r, 0 as g, 0 as b, 1 as a, lat as y, lng as x, ss.id as id, 1 as reported_at, 0 as vehicle_number from ttc_gtfs.shape_stops ss left join ttc_gtfs.stops s on ss.stop_id = s.id where shape_id = %s\" \
    | ./bin/align_points_to_line_strips -f ttc_shape_points_%d.b \
    | ./bin/graph_ttc_performance -a dist_line_%s \
    > nextbus_temp_header_%d.b", shape_id, nextbus_rand_int, shape_id, nextbus_rand_int);
  
  fprintf(stderr, "%s\n", command);
  system(command);
  
  sprintf(command, "cat nextbus_temp_%d.b nextbus_temp_header_%d.b | ./bin/write_png -f %s", nextbus_rand_int, nextbus_rand_int, image);
  
  fprintf(stderr, "%s\n", command);
  system(command);
  
  sprintf(command, "rm nextbus_temp_%d.b nextbus_temp_header_%d.b ttc_shape_points_%d.b", nextbus_rand_int, nextbus_rand_int, nextbus_rand_int);
  
  fprintf(stderr, "%s\n", command);
  system(command);
  
  output_and_delete_image(conn, image);
  
  free(routeTag);
  free(dirTag);
  free(shape_id);
}

void ttc_route(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  char * route = mg_get_var(conn, "r");
  if (route == NULL) { mg_printf(conn, "You need to specify a route [like ?r=7, or ?r=510]<br />"); return; }
  
  mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
  
  mg_printf(conn, "<body>\n");
  
  char command[1000];
  
  MYSQL_RES * res;
  MYSQL_ROW row;
  
  sprintf(command, "SELECT id FROM ttc_gtfs.routes WHERE route_short_name = '%s'", route);
  fprintf(stderr, "%s\n", command);
  mysql_query(&mysql, command);
  res = mysql_store_result(&mysql);
  row = mysql_fetch_row(res);
  mysql_free_result(res);
  int route_id = atoi(row[0]);
  
  int i;
  sprintf(command, "SELECT shape_id FROM ttc_gtfs.trips WHERE route_id = %d GROUP BY shape_id", route_id);
  fprintf(stderr, "%s\n", command);
  mysql_query(&mysql, command);
  res = mysql_store_result(&mysql);
  while ((row = mysql_fetch_row(res)))
  {
    i++;
    mg_printf(conn, "<img src='/ttc_shape?shape_id=%s' title='%s'>%s\n", row[0], row[0], (i%2==1 ? "<br />" : ""));
  }
  mysql_free_result(res);
  
  free(route);
}

void ttc_shape(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  char * shape_id = mg_get_var(conn, "shape_id");
  if (shape_id == NULL) { mg_printf(conn, "You need to specify a shape_id<br />"); return; }
  
  char image[200];
  get_image_name(image);
  
  char command[1000];
  
  MYSQL_RES * res;
  MYSQL_ROW row;
  
  int num_shape_ids = 0;
  int * shape_ids = NULL;
  
  sprintf(command, "select shape_id from ttc_gtfs.trips where route_id = (SELECT route_id FROM ttc_gtfs.trips WHERE shape_id = '%s' GROUP BY route_id) group by shape_id;", shape_id);
  //fprintf(stderr, "%s\n", command);
  mysql_query(&mysql, command);
  res = mysql_store_result(&mysql);
  while ((row = mysql_fetch_row(res)))
  {
    num_shape_ids++;
    shape_ids = (int*)realloc(shape_ids, sizeof(int)*num_shape_ids);
    shape_ids[num_shape_ids-1] = atoi(row[0]);
  }
  mysql_free_result(res);
  
  sprintf(command, "./bin/read_mysql \"SELECT shape_id = %s as r, 0 as g, 0 as b, round((shape_id = %s) * 0.5) + (!(shape_id = %s))* 0.1 as a, lat as y, lng as x, shape_id as id FROM ttc_gtfs.shape_points WHERE shape_id IN (", shape_id, shape_id, shape_id);
  {
    char temp[20];
    int i;
    for (i = 0 ; i < num_shape_ids ; i++)
    {
      sprintf(temp, "%s%d", (i==0?"":","), shape_ids[i]);
      strcat(command,temp);
    }
  }
  strcat(command, ") ORDER BY shape_id = '");
  strcat(command, shape_id);
  strcat(command, "' asc, shape_id, position\" | ./bin/write_png -w 120 -f ");
  strcat(command, image);
  fprintf(stderr, "%s\n", command);
  
  system(command);
  
  output_and_delete_image(conn, image);
  
  free(shape_id);
}

void ttc_route_realtime(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  char * route = mg_get_var(conn, "r");
  if (route == NULL) { mg_printf(conn, "You need to specify a route [like ?r=7, or ?r=510]<br />"); return; }
  
  char temp[200];
  get_image_name(temp);
  
  char command[1000];
  
  if (strlen(route) == 0)
    sprintf(command, "%s/bin/read_mysql \"SELECT x, y, id FROM nextbus_null.points WHERE routeTag IS NULL\" | %s/bin/write_png -f %s/%s", cwd, cwd, cwd, temp);
  else
    sprintf(command, "%s/bin/read_mysql \"SELECT x, y, id FROM nextbus.points WHERE routeTag = '%d'\" | %s/bin/write_png -f %s/%s", cwd, atoi(route), cwd, cwd, temp);
  fprintf(stderr, "%s\n", command);
  
  system(command);
  
  output_and_delete_image(conn, temp);
  
  free(route);
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
  
  //mg_set_uri_callback(ctx, "/fields", &fields, NULL);
  //mg_set_uri_callback(ctx, "/shapes", &shapes, NULL);
  //mg_set_uri_callback(ctx, "/records", &records, NULL);
  //mg_set_uri_callback(ctx, "/record_a_position", &record_a_position, NULL);
  
  printf("[http server on port:%s]\n", port);
  
  for (;;) sleep(10000);
  mg_stop(ctx);
}
