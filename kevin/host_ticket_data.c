
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h> // for gethostbyname
#include "../src/block.h"
#include "../src/block_kdtree.h"
#include "../ext/mongoose.h"

int received_kill = 0;
const char * port = "4512";

struct Block * addresses = NULL;
//struct Block * edges = NULL;

char boot_time[10] = "";

char addresses_filename[1000] = "";//data/toronto.addresses.fewercolumns.with.full_address.and.radius.and.streetedges.and.tickets.double.filtered.block";

void * addresses_kdtree = NULL;

int addresses_FULL_ADDRESS_column_id = -1;
int addresses_x_column_id = -1;
int addresses_y_column_id = -1;
int addresses_angle_column_id = -1;
int addresses_road_width_column_id = -1;
int addresses_radius_column_id = -1;

int addresses_num_AT_tickets_column_id = -1;
int addresses_num_AT_tickets_within_1_hour_column_id = -1;
int addresses_num_AT_tickets_within_3_hours_column_id = -1;
int addresses_num_AT_tickets_within_6_hours_column_id = -1;
int addresses_num_AT_meter_tickets_column_id = -1;
int addresses_num_AT_meter_tickets_within_1_hour_column_id = -1;
int addresses_num_AT_meter_tickets_within_3_hours_column_id = -1;
int addresses_num_AT_meter_tickets_within_6_hours_column_id = -1;

int addresses_num_OP_tickets_column_id = -1;
int addresses_num_OP_tickets_within_1_hour_column_id = -1;
int addresses_num_OP_tickets_within_3_hours_column_id = -1;
int addresses_num_OP_tickets_within_6_hours_column_id = -1;
int addresses_num_OP_meter_tickets_column_id = -1;
int addresses_num_OP_meter_tickets_within_1_hour_column_id = -1;
int addresses_num_OP_meter_tickets_within_3_hours_column_id = -1;
int addresses_num_OP_meter_tickets_within_6_hours_column_id = -1;

unsigned int get_msec(void)
{
  static struct timeval timeval, first_timeval;
  gettimeofday(&timeval, 0);
  if(first_timeval.tv_sec == 0) { first_timeval = timeval; return 0; }
  return (timeval.tv_sec - first_timeval.tv_sec) * 1000 + (timeval.tv_usec - first_timeval.tv_usec) / 1000;
}

void near(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  unsigned int start = get_msec();
  
  mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: application/javascript\r\nConnection: close\r\n\r\n");
  
  double lat, lng, range=2;
  time_t now_t = time(NULL);
  int i, j, k, l;
  
  char * temp = NULL;
  temp = mg_get_var(conn, "lat"); if (temp != NULL) { lat = atof(temp); mg_free(temp); } else if (range==0) { mg_printf(conn, "{\"error\":\"lat is required\"}"); return; }
  temp = mg_get_var(conn, "lng"); if (temp != NULL) { lng = atof(temp); mg_free(temp); } else if (range==0) { mg_printf(conn, "{\"error\":\"lng is required\"}"); return; }
  //temp = mg_get_var(conn, "range"); if (temp != NULL) { range = atof(temp); mg_free(temp); } else if (range==0) { mg_printf(conn,"{\"error\":\"range is required\"}"); return; }
  
  const char * uuid = mg_get_header(conn, "X-Device-Identifier"); if (uuid != NULL) { fprintf(stderr, "X-Device-Identifier: %s\n", uuid); }
  
  if (range > 6) range = 6;
  
  int num_range_in_minutes = 3;
  int range_in_minutes[10] = { 60, 180, 360, 0, 0, 0, 0, 0, 0, 0 }; // 1 hour, 3 hours, 6 hours
  char range_in_minutes_names[10][30] = { "1 hour", "3 hours", "6 hours" };
  int counts[10] = { 0,0,0,0,0,0,0,0,0,0 }; // accumulated later for each address
  
  int num_infraction_codes = 6;
  int infraction_codes[6] = { 5, 8, 9, 29, 264, 266 };
  
  fprintf(stderr, "%s %f %f %f\n", ri->uri, lat, lng, range);
  
  char now_time[10] = "";
  //int now_wday;
  int now_in_minutes;
  {
    struct tm * now = localtime(&now_t);
    //now_wday = now->tm_wday;
    now_in_minutes = now->tm_hour*60.0 + now->tm_min;
    //now_in_minutes = (int)(floor(now_in_minutes / 30.0) * 30.0);
    
    sprintf(now_time, "%02d:%02d", now->tm_hour, now->tm_min);
  }
  
  struct kdtree_results results = search_kdtree_find_within_range(addresses_kdtree, lng, lat, range);
  
  mg_printf(conn, "{\n  \"years\":[2008,2011],\n  \"api_version\":\"1\",\n  \"boot_time\":\"%s\",\n  \"params\": {\n    \"lat\":%lf,\n    \"lng\":%lf", boot_time, lat, lng);
  mg_printf(conn, ",\n    \"range_in_meters\":%lf,\n    \"range_in_minutes\":[", range);
  for (i = 0 ; i < num_range_in_minutes ; i++) mg_printf(conn, "%s\"%s\"", (i==0?"":","), range_in_minutes_names[i]);
  //mg_printf(conn, "],\n    \"infractions\":[");
  //for (i = 0 ; i < num_infraction_codes ; i++) mg_printf(conn, "%s%d", (i==0?"":","), infraction_codes[i]);
  mg_printf(conn, "]\n  },\n  \"addresses\": [");
  
  //int now_wday_hh_column_id = now_wday * 3.0 + now_in_minutes / 480.0; // half hour each, maybe KBFU ?
  
  int first_result = 1;
  
  int AT_OP;
  for (AT_OP = 1 ; AT_OP >= 0 ; AT_OP--) // list OP addresses first
  {
    for (i = 0 ; i < results.count ; i++)
    {
      int address_row_id = results.row_ids[i];
      
      char * FULL_ADDRESS = NULL;
      if (addresses_FULL_ADDRESS_column_id != -1)
        FULL_ADDRESS = (char*)get_cell(addresses, address_row_id, addresses_FULL_ADDRESS_column_id);
      
      double x          = get_cell_as_double(addresses, address_row_id, addresses_x_column_id);
      double y          = get_cell_as_double(addresses, address_row_id, addresses_y_column_id);
      double angle      = get_cell_as_double(addresses, address_row_id, addresses_angle_column_id);
      double road_width = get_cell_as_double(addresses, address_row_id, addresses_road_width_column_id);
      double radius     = get_cell_as_double(addresses, address_row_id, addresses_radius_column_id);
      
      int32_t num_tickets[2] = {
        get_cell_as_int32(addresses, address_row_id, addresses_num_AT_tickets_column_id),
        get_cell_as_int32(addresses, address_row_id, addresses_num_OP_tickets_column_id)
      };
      
      int32_t num_meter_tickets[2] = {
        get_cell_as_int32(addresses, address_row_id, addresses_num_AT_meter_tickets_column_id),
        get_cell_as_int32(addresses, address_row_id, addresses_num_OP_meter_tickets_column_id)
      };
      
      /*double num_tickets_within[2][3] = {
        {
          get_cell_as_int32(addresses, address_row_id, addresses_num_AT_tickets_within_1_hour_column_id),
          get_cell_as_int32(addresses, address_row_id, addresses_num_AT_tickets_within_3_hours_column_id),
          get_cell_as_int32(addresses, address_row_id, addresses_num_AT_tickets_within_6_hours_column_id),
        },{
          get_cell_as_int32(addresses, address_row_id, addresses_num_OP_tickets_within_1_hour_column_id),
          get_cell_as_int32(addresses, address_row_id, addresses_num_OP_tickets_within_3_hours_column_id),
          get_cell_as_int32(addresses, address_row_id, addresses_num_OP_tickets_within_6_hours_column_id)
        }
      };*/
      
      double num_meter_tickets_within[2][3] = {
        {
          get_cell_as_int32(addresses, address_row_id, addresses_num_AT_meter_tickets_within_1_hour_column_id),
          get_cell_as_int32(addresses, address_row_id, addresses_num_AT_meter_tickets_within_3_hours_column_id),
          get_cell_as_int32(addresses, address_row_id, addresses_num_AT_meter_tickets_within_6_hours_column_id),
        },{
          get_cell_as_int32(addresses, address_row_id, addresses_num_OP_meter_tickets_within_1_hour_column_id),
          get_cell_as_int32(addresses, address_row_id, addresses_num_OP_meter_tickets_within_3_hours_column_id),
          get_cell_as_int32(addresses, address_row_id, addresses_num_OP_meter_tickets_within_6_hours_column_id)
        }
      };
      
      if (num_tickets[AT_OP] > 10 || num_meter_tickets[AT_OP] > 10)
      {
        float redgreen[3] = { 0.0, 0.0, 0.0 };
        
        redgreen[0] = (num_meter_tickets_within[AT_OP][0] * num_meter_tickets[AT_OP]) / (num_meter_tickets[AT_OP] / 7.0 / 24.0 / 2.0);
        redgreen[1] = (num_meter_tickets_within[AT_OP][1] * num_meter_tickets[AT_OP]) / (num_meter_tickets[AT_OP] / 7.0 / 8.0 / 2.0);
        redgreen[2] = (num_meter_tickets_within[AT_OP][2] * num_meter_tickets[AT_OP]) / (num_meter_tickets[AT_OP] / 7.0 / 4.0 / 2.0);
        
				if (num_tickets[AT_OP] == 0 && num_meter_tickets[AT_OP] == 0)
				{
					redgreen[0] = redgreen[1] = redgreen[2] = 0;
				}
				
        //if (num_meter_tickets[AT_OP] < 10)
        //{
        //  redgreen[0] = redgreen[1] = redgreen[2] = 1.0; // no meter here
        //}
        
        // if there is no meter here
        //if (num_meter_tickets[AT_OP] < 0.05 * num_tickets[AT_OP])
        //{
        //  redgreen[0] = 1.0; // don't park
        //  redgreen[1] = 1.0;
        //  redgreen[2] = 1.0;
        //}
				
				if (isnan(redgreen[0])) redgreen[0] = 0.0;
				if (isnan(redgreen[1])) redgreen[1] = 0.0;
				if (isnan(redgreen[2])) redgreen[2] = 0.0;
        
        if (redgreen[0] > 1.0) redgreen[0] = 1.0; if (redgreen[0] < 0.0) redgreen[0] = 0.0;
        if (redgreen[1] > 1.0) redgreen[1] = 1.0; if (redgreen[1] < 0.0) redgreen[1] = 0.0;
        if (redgreen[2] > 1.0) redgreen[2] = 1.0; if (redgreen[2] < 0.0) redgreen[2] = 0.0;
        
        if (redgreen[2] < 0.5) redgreen[2] = 0.5; // cause of 3 hour parking limit, sigh
        
        mg_printf(conn, "%s\n    {\"id\":\"%s-%d\"", (first_result?"":","), AT_OP?"OP":"AT", address_row_id);
        if (FULL_ADDRESS != NULL) mg_printf(conn, ", \"name\":\"%s\"", FULL_ADDRESS);
        
        /*double x1 = get_cell_as_double(addresses, address_row_id, addresses_street_left_x_column_id);
        double y1 = get_cell_as_double(addresses, address_row_id, addresses_street_left_y_column_id);
        double x2 = get_cell_as_double(addresses, address_row_id, addresses_street_right_x_column_id);
        double y2 = get_cell_as_double(addresses, address_row_id, addresses_street_right_y_column_id);
        
        if (AT_OP == 1)
        {
          const double road_width = 0.0001 * 2.0;
          double angle = atan2(y1-y2, x1-x2) - 3.14159265359/2.0;
          x1 += cos(angle)*road_width; x2 += cos(angle)*road_width;
          y1 += sin(angle)*road_width; y2 += sin(angle)*road_width;
        }*/
        
        //mg_printf(conn, ", \"point\":[%f,%f]", y, x);
        
        //mg_printf(conn, ", \"polyline\":[[%f,%f],[%f,%f]]", y1, x1, y2, x2);
        
        double x1 = x+cos(angle+(AT_OP ? 3.14159265 : 0))*(road_width/2.0) + cos(angle+1.5707963)*(radius*0.8);
        double y1 = y+sin(angle+(AT_OP ? 3.14159265 : 0))*(road_width/2.0) + sin(angle+1.5707963)*(radius*0.8);
        double x2 = x+cos(angle+(AT_OP ? 3.14159265 : 0))*(road_width/2.0) + cos(angle-1.5707963)*(radius*0.8);
        double y2 = y+sin(angle+(AT_OP ? 3.14159265 : 0))*(road_width/2.0) + sin(angle-1.5707963)*(radius*0.8);
        
        mg_printf(conn, ", \"polyline\":[[%f,%f],[%f,%f]]", y1, x1, y2, x2);
        
        mg_printf(conn, ", \"tickets\":[%.2f,%.2f,%.2f], \"total\":%d, \"total_meter\":%d}", redgreen[0], redgreen[1], redgreen[2], num_tickets[AT_OP], num_meter_tickets[AT_OP]);
        first_result = 0;
      }
    }
  }
  mg_printf(conn, "\n  ]");
  
  mg_printf(conn, ",\n  \"bbox\":[[%f,%f],[%f,%f]]", lat-0.0021, lng-0.0021, lat+0.0021, lng+0.0021);
  mg_printf(conn, ",\n  \"exec_time\":%.5f\n}", (float)(get_msec() - start) / 1000.0);
  
  free(results.row_ids);
}

void far(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  unsigned int start = get_msec();
  
  mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: application/javascript\r\nConnection: close\r\n\r\n");
  
  char * id = NULL;
  id = mg_get_var(conn, "id"); if (id == NULL) { mg_printf(conn, "{\"error\":\"id is required\"}"); return; }
  
  mg_printf(conn, "{");
  mg_printf(conn, "\n  \"id\":\"%s\"", id);
  
  mg_printf(conn, ",\n  \"num_tickets\":500");
  
  mg_printf(conn, "\n}");
  
  mg_free(id);
}

void httpkill(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  //mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: application/javascript\r\nConnection: close\r\n\r\n");
  mg_printf(conn, "ok");
  received_kill = 1;
}

int main(int argc, char ** argv)
{
  unsigned int start = get_msec();
  
  addresses = read_block(stdin);
  
  addresses_FULL_ADDRESS_column_id  = get_column_id_by_name(addresses, "FULL_ADDRESS");
  addresses_x_column_id             = get_column_id_by_name_or_exit(addresses, "x");
  addresses_y_column_id             = get_column_id_by_name_or_exit(addresses, "y");
  addresses_angle_column_id         = get_column_id_by_name_or_exit(addresses, "angle");
  addresses_road_width_column_id    = get_column_id_by_name_or_exit(addresses, "road_width");
  addresses_radius_column_id        = get_column_id_by_name_or_exit(addresses, "radius");
  
  addresses_num_AT_tickets_column_id                      = get_column_id_by_name_or_exit(addresses, "num_AT_tickets");
  addresses_num_AT_tickets_within_1_hour_column_id        = get_column_id_by_name_or_exit(addresses, "num_AT_tickets_within_1_hour");
  addresses_num_AT_tickets_within_3_hours_column_id       = get_column_id_by_name_or_exit(addresses, "num_AT_tickets_within_3_hours");
  addresses_num_AT_tickets_within_6_hours_column_id       = get_column_id_by_name_or_exit(addresses, "num_AT_tickets_within_6_hours");
  addresses_num_AT_meter_tickets_column_id                = get_column_id_by_name_or_exit(addresses, "num_AT_meter_tickets");
  addresses_num_AT_meter_tickets_within_1_hour_column_id  = get_column_id_by_name_or_exit(addresses, "num_AT_meter_tickets_within_1_hour");
  addresses_num_AT_meter_tickets_within_3_hours_column_id = get_column_id_by_name_or_exit(addresses, "num_AT_meter_tickets_within_3_hours");
  addresses_num_AT_meter_tickets_within_6_hours_column_id = get_column_id_by_name_or_exit(addresses, "num_AT_meter_tickets_within_6_hours");
  
  addresses_num_OP_tickets_column_id                      = get_column_id_by_name_or_exit(addresses, "num_OP_tickets");
  addresses_num_OP_tickets_within_1_hour_column_id        = get_column_id_by_name_or_exit(addresses, "num_OP_tickets_within_1_hour");
  addresses_num_OP_tickets_within_3_hours_column_id       = get_column_id_by_name_or_exit(addresses, "num_OP_tickets_within_3_hours");
  addresses_num_OP_tickets_within_6_hours_column_id       = get_column_id_by_name_or_exit(addresses, "num_OP_tickets_within_6_hours");
  addresses_num_OP_meter_tickets_column_id                = get_column_id_by_name_or_exit(addresses, "num_OP_meter_tickets");
  addresses_num_OP_meter_tickets_within_1_hour_column_id  = get_column_id_by_name_or_exit(addresses, "num_OP_meter_tickets_within_1_hour");
  addresses_num_OP_meter_tickets_within_3_hours_column_id = get_column_id_by_name_or_exit(addresses, "num_OP_meter_tickets_within_3_hours");
  addresses_num_OP_meter_tickets_within_6_hours_column_id = get_column_id_by_name_or_exit(addresses, "num_OP_meter_tickets_within_6_hours");
  
  addresses_kdtree = create_kdtree_for_block(addresses);
  
  {
    struct Attribute * tickets_vs_now_attribute = get_attribute_by_name(addresses, "tickets_vs_now");
    
    time_t now_t = *(int64_t*)attribute_get_value(tickets_vs_now_attribute);//time(NULL);
    struct tm * now = localtime(&now_t);
    sprintf(boot_time, "%02d:%02d", now->tm_hour, now->tm_min);
  }
  
  struct mg_context *ctx = mg_start();
  mg_set_option(ctx, "dir_list", "no");  // Set document root
  int ret = 0;
  ret = mg_set_option(ctx, "ports", port);
  while (ret != 1)
  {
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    addr.sin_addr = *(struct in_addr *)gethostbyname("localhost")->h_addr;
    
    if (ret != 1 && connect(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr))==0) // try to kill it gracefully
    {
      const char * msg = "GET /kill HTTP/1.0\n\n";
      send(sockfd, msg, strlen(msg), 0);
      char buf[10];
      int s = 1;
      while (s > 0)
      {
        usleep(100000);
        s = recv(sockfd, &buf, 10, 0);
        if (s > 0)
        {
          if (strncmp(buf, "ok", 2) == 0)
          {
            int count = 10000;
            while (ret != 1 && count < 10000)
            {
              ret = mg_set_option(ctx, "ports", port);
              count ++;
            }
            break;
          }
        }
      }
      close(sockfd);
    }
    
    sleep(1);
    ret = mg_set_option(ctx, "ports", port);
  }
  
  mg_set_uri_callback(ctx, "/near", &near, NULL);
  mg_set_uri_callback(ctx, "/far", &far, NULL);
  mg_set_uri_callback(ctx, "/kill", &httpkill, NULL);
  
  fprintf(stderr, "boots in %f seconds\n", (float)(get_msec() - start) / 1000.0);
  printf("-------------------------------------------------------------------------\n");
  
  while (!received_kill)
  {
    usleep(999999); //pause();//sleep(10000);
    if (received_kill) { fprintf(stderr, "received kill\n"); }
  }
  mg_stop(ctx);
  
  return EXIT_SUCCESS;
}
