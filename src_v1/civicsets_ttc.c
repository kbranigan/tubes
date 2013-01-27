#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <mysql.h>

extern MYSQL mysql; // defined and setup in civicsets.c
extern char * cwd;  // defined and setup in civicsets.c

#include "mongoose.h"

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
  
  //mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nConnection: close\r\n\r\n");
  
  //mg_printf(conn, "<body>\n");
  
  char image[200];
  get_image_name(image);
  
  char command[8000] = "";
  
  MYSQL_RES * res;
  MYSQL_ROW row;
  
  sprintf(command, "SELECT id FROM ttc_gtfs.routes WHERE route_short_name = '%s'", route);
  fprintf(stderr, "%s\n", command);
  mysql_query(&mysql, command);
  res = mysql_store_result(&mysql);
  row = mysql_fetch_row(res);
  mysql_free_result(res);
  if (row == NULL || row[0] == NULL) { mg_printf(conn, "route '%s' not found.\n", route); free(route); return; }
  int route_id = atoi(row[0]);
  
  int num_shape_ids = 0;
  char ** shape_ids = NULL;
  
  int i = 0;
  sprintf(command, "SELECT shape_id FROM ttc_gtfs.trips WHERE route_id = %d GROUP BY shape_id", route_id);
  fprintf(stderr, "%s\n", command);
  mysql_query(&mysql, command);
  res = mysql_store_result(&mysql);
  while ((row = mysql_fetch_row(res)))
  {
    if (row[0] == NULL) continue;
    num_shape_ids ++;
    shape_ids = (char**)realloc(shape_ids, sizeof(char*)*num_shape_ids);
    shape_ids[num_shape_ids-1] = (char*)malloc(strlen(row[0])+1);
    strcpy(shape_ids[num_shape_ids-1], row[0]);
  }
  if (num_shape_ids == 0) { mg_printf(conn, "route %s has no shapes.\n", route); free(route); return; }
  
  command[0] = 0;
  
  for (i = 0 ; i < num_shape_ids ; i++)
  {
    sprintf(command, "./bin/read_mysql \"SELECT shape_id = %s as r, 0 as g, 0 as b, round((shape_id = %s) * 0.5) + (!(shape_id = %s))* 0.1 as a, lat*2.0 as y, lng as x, shape_id as id FROM ttc_gtfs.shape_points WHERE shape_id IN (%s", shape_ids[i], shape_ids[i], shape_ids[i], shape_ids[i]);
    int j;
    for (j = 0 ; j < num_shape_ids ; j++)
    {
      if (i != j)
      {
        strcat(command, ",");
        strcat(command, shape_ids[j]);
      }
    }
    
    strcat(command, ") ORDER BY shape_id = '");
    strcat(command, shape_ids[i]);
    strcat(command, "' asc, shape_id, position\" > ttc_shape_");
    strcat(command, shape_ids[i]);
    strcat(command, "_in_route_");
    strcat(command, route);
    strcat(command, ".b");
    
    fprintf(stderr, "%s\n", command);
    system(command);
  }
  mysql_free_result(res);
  
  sprintf(command, "cat ttc_shape_%s_in_route_%s.b", shape_ids[0], route);
  for (i = 1 ; i < num_shape_ids ; i++)
  {
    strcat(command, " | ./bin/tile -f ttc_shape_");
    strcat(command, shape_ids[i]);
    strcat(command, "_in_route_");
    strcat(command, route);
    strcat(command, ".b");
  }
  
  strcat(command, " | ./bin/write_png -f ");
  strcat(command, image);
  
  fprintf(stderr, "%s\n", command);
  system(command);
  
  for (i = 0 ; i < num_shape_ids ; i++)
  {
    sprintf(command, "rm ttc_shape_");
    strcat(command, shape_ids[i]);
    strcat(command, "_in_route_");
    strcat(command, route);
    strcat(command, ".b");
    fprintf(stderr, "%s\n", command);
    system(command);
  }
  
  output_and_delete_image(conn, image);
  
  for (i = 0 ; i < num_shape_ids ; i++)
    free(shape_ids[i]);
  free(shape_ids);
  
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
