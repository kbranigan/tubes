
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h> // for gethostbyname
#include <mysql.h>
#include "../ext/kdtree.hpp"
#include "../ext/mongoose.h"

struct duplet 
{
  typedef double value_type;
  
  inline value_type operator[](int const N) const { return d[N]; }
  inline bool operator==(duplet const& other) const { return this->d[0] == other.d[0] && this->d[1] == other.d[1]; }
  inline bool operator!=(duplet const& other) const { return this->d[0] != other.d[0] || this->d[1] != other.d[1]; }
  //friend std::ostream & operator<<(std::ostream & o, duplet const& d) { return o << "(" << d[0] << "," << d[1] << ")"; }
  
  double distance_to(struct duplet const& x) const
  {
    fprintf(stderr, "distance_to\n");
    double dx = d[0] - x.d[0];
    double dy = d[1] - x.d[1];
    double dist = std::sqrt(dx*dx + dy*dy);
    fprintf(stderr, "dist = %f (%f %f)\n", dist, d[0], dy);
    return dist;
  }
  
  value_type d[2];
  int32_t row_id;
  //void * data;
};

typedef KDTree::KDTree<2, duplet, std::pointer_to_binary_function<duplet,int,double> > duplet_tree_type;
inline double return_dup(duplet d, int k) { return d[k]; }

duplet_tree_type * kdtree = new duplet_tree_type(std::ptr_fun(return_dup));

int received_kill = 0;
const char * port = "4512";

MYSQL mysql;
MYSQL_RES * res;
MYSQL_ROW row;

struct sPoint {
	double x;
	double y;
};

struct sAddressLine {
	int num_points;
	struct sPoint * points;
	int32_t geo_id, arc_side, sequence;
	time_t last_update_ts;
	struct sTicketData * ticket_data;
	int32_t num_ticket_data;
};

struct sTicketData {
	int32_t infraction_code;
	int32_t tickets_all_day;
	char tickets_next_3_hours[7]; // only 6 characters stored
};

struct sAddressLine * address_lines = NULL;
int num_address_lines = 0;

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
	struct tm * timeinfo = localtime(&now_t);
	int string_offset = timeinfo->tm_hour*2 + (int)floor(timeinfo->tm_min / 30.0);
	int i, j, k, l;
	
	char * temp = NULL;
	temp = mg_get_var(conn, "lat"); if (temp != NULL) { lat = atof(temp); mg_free(temp); } else if (range==0) { mg_printf(conn, "{\"error\":\"lat is required\"}"); return; }
	temp = mg_get_var(conn, "lng"); if (temp != NULL) { lng = atof(temp); mg_free(temp); } else if (range==0) { mg_printf(conn, "{\"error\":\"lng is required\"}"); return; }
	
	fprintf(stderr, "%s %f %f %f\n", ri->uri, lat, lng, range);
	
	char time_ranges[3][15] = { "", "", "" };
	
	sprintf(time_ranges[0], "7pm");
	sprintf(time_ranges[1], "8pm");
	sprintf(time_ranges[2], "2am");
	
	mg_printf(conn, "{\n	\"years\":[2008,2011],\n	\"api_version\":\"2\"");
	mg_printf(conn, ",\n	\"params\": {\n		\"lat\":%lf,\n		\"lng\":%lf", lat, lng);
	mg_printf(conn, ",\n		\"payment_options\":[\"Free\", \"Meter\", \"Lots\"]");
	mg_printf(conn, ",\n		\"time_ranges\":[\"%s\", \"%s\", \"%s\"", time_ranges[0], time_ranges[1], time_ranges[2]);
	mg_printf(conn, "]\n	},\n	\"address_ranges\": [");
	
	duplet d = { {lng, lat}, 0 };
	std::vector<duplet> v;
	kdtree->find_within_range(d, range/1000, std::back_inserter(v));
	
	int * ids = NULL;
	int num_ids = 0;
	
	for (std::vector<duplet>::iterator it = v.begin() ; it != v.end() ; it++) {
		int id = it->row_id;
		
		int found = 0;
		for (i = 0 ; i < num_ids ; i++) {
			if (ids[i] == id) {
				found = 1;
				break;
			}
		}
		
		if (found == 0) {
			num_ids++;
			ids = (int*)realloc(ids, num_ids*sizeof(int));
			ids[num_ids-1] = id;
		}
	}
	
	{
		//int i;
		//for (i = 0 ; i < num_ids; i ++)
		//	fprintf(stderr, "%d: %d\n", i, ids[i]);
	}
	
	//fprintf(stderr, "num_address_lines = %d, num_ids = %d\n", num_address_lines, num_ids);
	int count = 0;
	//for (i = 0 ; i < num_address_lines ; i++)
	//if (0)
	//for (std::vector<duplet>::iterator it = v.begin() ; it != v.end() ; it++)
	
	int index;
	for (index = 0 ; index < num_ids ; index++) {
		int i = ids[index];//it->row_id;
		//fprintf(stderr, "i:%d\n", i);
		mg_printf(conn, "%s\n		{\"id\":\"%d-%d\"", (count==0?"":","), address_lines[i].arc_side, address_lines[i].geo_id);
		//if (FULL_ADDRESS != NULL) mg_printf(conn, ", \"name\":\"%s\"", FULL_ADDRESS);
		mg_printf(conn, ", \"polyline\":[");
		
		/*j = i;
		while (j < num_address_lines)
		{
			//mg_printf(conn, "%s[%f,%f]", (j==i?"":","), address_lines[j].y, address_lines[j].x);
			j++;
			if (j == num_address_lines) break;
			if (address_lines[j].geo_id != address_lines[i].geo_id) break;
			if (address_lines[j].arc_side != address_lines[i].arc_side) break;
		}*/
		
		for (j = 0 ; j < address_lines[i].num_points ; j++) {
			mg_printf(conn, "%s[%f,%f]", (j==0?"":","), address_lines[i].points[j].y, address_lines[i].points[j].x);
		}
		
		mg_printf(conn, "]");
		
		float ha = rand()/(float)RAND_MAX;

		float redgreen[3] = { ha, 1.0 - ha, 0.5 };

		int total_tickets = 0;
		int total_meter_tickets = 0;
		
		int ever_never_stop = 0;          // 31, 134
		int current_never_stop = 0;
		int ever_prohibited_time = 0;     // 5, 6, 8, 9, 28, 29, 106, 137, 139, 209, 263, 264, 266
		int current_prohibited_time = 0;
		int ever_metered = 0;             // 1, 40, 41, 42, 44, 45, 46? , 115?, 122?, 207, 210, 312
		int current_metered = 0;
		
		//fprintf(stderr, "last_update_ts = %ld\n", address_lines[i].last_update_ts);
		
		if (address_lines[i].last_update_ts < now_t - 5) //60*5) // more then 5 minutes old
		{
			address_lines[i].last_update_ts = now_t;
			
			address_lines[i].num_ticket_data = 0;
			if (address_lines[i].ticket_data != NULL) free(address_lines[i].ticket_data);
			address_lines[i].ticket_data = NULL;
			char query[200];
			sprintf(query, "SELECT infraction_code, tickets_all_day, tickets_half_hourly, SUBSTRING(tickets_half_hourly, %d, 6) "
				"FROM mb.ticket_day_infractions_for_geo_id WHERE geo_id = %d AND is_opp = %d AND wday = %d", string_offset+1, address_lines[i].geo_id, address_lines[i].arc_side, timeinfo->tm_wday);
			//fprintf(stderr, "%s\n", query);
			if (mysql_query(&mysql, query)==0)
			{
				res = mysql_store_result(&mysql);
				address_lines[i].num_ticket_data = mysql_num_rows(res);
				address_lines[i].ticket_data = (struct sTicketData*)calloc(address_lines[i].num_ticket_data, sizeof(struct sTicketData));
				//fprintf(stderr, "address_lines[i].num_ticket_data = %d\n", address_lines[i].num_ticket_data);
				
				int r = 0;
				while ((row = mysql_fetch_row(res)))
				{
					struct sTicketData * ticket_data = &address_lines[i].ticket_data[r];
					//fprintf(stderr, "offset = %d\n", offset);
					
					ticket_data->infraction_code = atoi(row[0]);
					ticket_data->tickets_all_day = atoi(row[1]);
					strncpy(ticket_data->tickets_next_3_hours, row[3], 6); // you'll have to verify this one
					
					//fprintf(stderr, "%03d '%s'\n", address_lines[i].ticket_data[r].infraction_code, address_lines[i].ticket_data[r].tickets_next_3_hours);
					r++;
				}
				mysql_free_result(res);
			}
		}
		
		int r = 0;
		for (r = 0 ; r < address_lines[i].num_ticket_data ; r++)
		{
			struct sTicketData * ticket_data = &address_lines[i].ticket_data[r];
			total_tickets += ticket_data->tickets_all_day;
			if (ticket_data->infraction_code == 1 || ticket_data->infraction_code == 207 || 
					ticket_data->infraction_code == 210 || ticket_data->infraction_code == 312)
			{
				total_meter_tickets += ticket_data->tickets_all_day;
			}
		}
		
		if ((float)total_meter_tickets > (float)total_tickets * 0.05) ever_metered = 1;
		
		if (ever_metered) redgreen[0] = 0;
		
		mg_printf(conn, ", \"tickets\":[%.2f,%.2f,%.2f], \"total\":%d, \"total_meter\":%d}", 
			redgreen[0], redgreen[1], redgreen[2], total_tickets, total_meter_tickets);
			//rand()/(float)RAND_MAX, rand()/(float)RAND_MAX, rand()/(float)RAND_MAX, 50, 56);
			//redgreen[0], redgreen[1], redgreen[2], num_tickets[AT_OP], num_meter_tickets[AT_OP]);
		
		count++;
	}
	mg_printf(conn, "\n	]");
	
	mg_printf(conn, ",\n	\"bbox\":[[%f,%f],[%f,%f]]", lat-0.0021, lng-0.0021, lat+0.0021, lng+0.0021);
	mg_printf(conn, ",\n	\"exec_time\":%.5f\n}", (float)(get_msec() - start) / 1000.0);
	
	free(ids);
}
/*
void details(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
	char * temp = NULL;
	temp = mg_get_var(conn, "geo_id");
	int geo_id = (temp==NULL) ? 0 : atoi(temp);
	free(temp);
	
	mg_printf(conn, "[");
	char query[1000] = "";
	sprintf(query,	"SELECT is_opp, wday, infraction_code, tickets_all_day, tickets_half_hourly FROM ticket_day_infractions_for_geo_id "
									"WHERE geo_id = %d ORDER BY is_opp, wday, infraction_code, tickets_all_day, tickets_half_hourly", geo_id);
	//mg_printf(conn, "%s\n", query);
	if (mysql_query(&mysql, query)==0) {
		res = mysql_store_result(&mysql);
		num_address_lines = mysql_num_rows(res);
		int i = 0;
		while ((row = mysql_fetch_row(res))) {
			char thh_temp[100] = "";
			int j=0,k=0;
			for (j = 0 ; j < strlen(row[4]) ; j++) {
				if (row[4][j] == '"' || row[4][j] == '\\' || row[4][j] == '/') {
					thh_temp[k++] = '\\';
				}
				thh_temp[k++] = row[4][j];
			}
			thh_temp[99] = 0;
			mg_printf(conn, "%s\n  [%s,%s,%3s,%3s,\"%s\"]", (i!=0 ? "," : ""), row[0], row[1], row[2], row[3], thh_temp);
			i++;
		}
		mysql_free_result(res);
	}
	mg_printf(conn, "\n]");
}

void break_down_infraction_code(struct mg_connection *conn, const struct mg_request_info *ri, void *data) {
	char * temp = NULL;
	temp = mg_get_var(conn, "infraction_code");
	int infraction_code = (temp==NULL) ? 0 : atoi(temp);
	free(temp);
	mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
	if (infraction_code == 0) {
		mg_printf(conn, "infraction_code needed\n");
		return;
	}
	
	FILE * fp = NULL;
	char filename[100] = "";
	sprintf(filename, "cache_images/infractions/%d-0.png", infraction_code);
	fp = fopen(filename, "r");
	if (fp == NULL) {
		int wday;
		for (wday = 0 ; wday < 7 ; wday++) {
			sprintf(filename, "cache_images/infractions/%d-%d.png", infraction_code, wday);
			fp = fopen(filename, "r");
			if (fp == NULL) {
				char cmd[1000] = "";
				sprintf(cmd, "./bin/read_mysql -q "
												"\"SELECT *, count/7.0 AS alpha, sum_tickets_all_day/1000.0 AS red FROM "
													"(SELECT id, "
														"(SELECT COUNT(*) FROM mb.ticket_day_infractions_for_geo_id t "
															"WHERE t.geo_id = al.geo_id "
																"AND ((al.arc_side = 'L' AND t.is_opp = 0) OR (al.arc_side = 'R' AND t.is_opp = 1)) "
																"AND t.infraction_code = %d) AS count, "
														"(SELECT SUM(tickets_all_day) FROM mb.ticket_day_infractions_for_geo_id t "
															"WHERE t.geo_id = al.geo_id "
																"AND ((al.arc_side = 'L' AND t.is_opp = 0) OR (al.arc_side = 'R' AND t.is_opp = 1)) "
																"AND t.infraction_code = %d) AS sum_tickets_all_day, "
														"0 AS green, "
														"0 AS blue, "
														"x, y, (geo_id * if(arc_side='L', -1, 1)) AS shape_row_id, 0 AS shape_part_id, 5 AS shape_part_type "
															"FROM mb.address_lines al "
																//"WHERE geo_id = 7930734 "
																"ORDER BY geo_id, arc_side, id) e"
												"\" | ./bin/png --filename=%s", 
												infraction_code, 
												infraction_code, 
												filename);
				fprintf(stderr, "%s\n", cmd);
				system(cmd);
			} else {
				fclose(fp);
			}
			break;
		}
	} else {
		fclose(fp);
	}
	
	int wday;
	for (wday = 0 ; wday < 7 ; wday++) {
		mg_printf(conn, "%d<img src='cache_images/infractions/%d-%d.png' /><br />", wday, infraction_code, wday);
		break;
	}
}

void httpkill(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
	//mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: application/javascript\r\nConnection: close\r\n\r\n");
	mg_printf(conn, "ok");
	received_kill = 1;
}
*/
int main(int argc, char ** argv)
{
	unsigned int start = get_msec();
	
	if ((mysql_init(&mysql) == NULL)) { fprintf(stderr, "ERROR: mysql_init() error\n"); return 0; }
	if (!mysql_real_connect(&mysql, "localhost", "root", "", "mb", 0, NULL, 0))
	{
		fprintf(stderr, "ERROR: mysql_real_connect error (%s)\n", mysql_error(&mysql));
	}
	
	//if (mysql_query(&mysql, "SELECT x, y, sequence, geo_id, arc_side='L' FROM address_lines WHERE geo_id IN (1143060,1143217,1143508,1143623,1144803,1144964,1145067,1145132,1146178,1146213,1146443,1146522,1146556,1146683,1146728,1146770,1146853,1146976,1146998,1147027,2821086,3150314,5999130,7569522,7569576,7569605,7929431,7929478,7929486,7929605,7929919,7930184,7930461,7930588,7930670,7930734,7930769,7930850,7930872,7930873,8033769,8418213,8492130,8677044,10222906,10223745,10223817,10223853,10223870,10223904,10223924,10223941,10458361,10494161,10494181,10494196,10512735,10513373,10513396,10513444,10864275,10864288,10906331,12763884,12763900,14024762,14024763,14024764,14024849,14024850,14024868,14024869,14024988,14024989,14025205,14025206,14025244,14025245,14025275,14025276,14025283,14025284,14025347,14025348,14025408,14025409,14025410,14025469,14025535,14025536,14025543,14025544,14025568,14025569,14025583,14025584,14035996,14035997,14035998,14036014,14036064,14036065,14073511,14073512,14614667,14661238,14671468,14671590,14671591,14673213,14673508,14673518,14673703,14673704,14673829,14673830,20006289,20006295,20034983,20035030,20110574,20110595,20110596,20141140,30022476) ORDER BY geo_id, arc_side, sequence")==0)
	//if (mysql_query(&mysql, "SELECT x, y, sequence, geo_id, arc_side='L' FROM address_lines WHERE geo_id IN (7930734) ORDER BY geo_id, arc_side, sequence")==0)
	if (mysql_query(&mysql, "SELECT x, y, sequence, geo_id, arc_side='L' FROM address_lines ORDER BY geo_id, arc_side, sequence")==0)
	{
		res = mysql_store_result(&mysql);
		//num_address_lines = mysql_num_rows(res);
		//fprintf(stderr, "num_address_lines = %d\n", num_address_lines);
		//address_lines = (struct sAddressLine*)calloc(num_address_lines, sizeof(struct sAddressLine));
		
		while ((row = mysql_fetch_row(res)))
		{
			int geo_id   = atoi(row[3]);
			int arc_side = atoi(row[4]);
			
			if (num_address_lines == 0 || address_lines[num_address_lines-1].geo_id != geo_id || address_lines[num_address_lines-1].arc_side != arc_side) {
				num_address_lines++;
				address_lines = (struct sAddressLine*)realloc(address_lines, num_address_lines*sizeof(struct sAddressLine));
				memset(&address_lines[num_address_lines-1], 0, sizeof(struct sAddressLine));
				address_lines[num_address_lines-1].sequence = atoi(row[2]);
				address_lines[num_address_lines-1].geo_id   = geo_id;
				address_lines[num_address_lines-1].arc_side = arc_side;
				address_lines[num_address_lines-1].last_update_ts = 0;
				address_lines[num_address_lines-1].num_ticket_data = 0;
				address_lines[num_address_lines-1].ticket_data = NULL;
			}
			
			double x = atof(row[0]);
			double y = atof(row[1]);
			
			address_lines[num_address_lines-1].num_points++;
			address_lines[num_address_lines-1].points = (struct sPoint*)realloc(address_lines[num_address_lines-1].points, sizeof(struct sPoint)*address_lines[num_address_lines-1].num_points);
			address_lines[num_address_lines-1].points[address_lines[num_address_lines-1].num_points-1].x = x;
			address_lines[num_address_lines-1].points[address_lines[num_address_lines-1].num_points-1].y = y;
			
			duplet d = { {x, y}, num_address_lines-1 };
			kdtree->insert(d);
		}
		fprintf(stderr, "num_address_lines = %d\n", num_address_lines);
		
		kdtree->optimize();
		fprintf(stderr, "kdtree->size() = %d\n", (int)kdtree->size());
		
		mysql_free_result(res);
	}
	else
	{
		fprintf(stderr, "ERROR: mysql_query() error %s\n", mysql_error(&mysql));
	}
	
	struct mg_context *ctx = mg_start();
	mg_set_option(ctx, "dir_list", "no"); // Set document root
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
	
	fprintf(stderr, "boots in %f seconds\n", (float)(get_msec() - start) / 1000.0);
	printf("-------------------------------------------------------------------------\n");
	
	mg_set_uri_callback(ctx, "/near", &near, NULL);
	//mg_set_uri_callback(ctx, "/kill", &httpkill, NULL);
	//mg_set_uri_callback(ctx, "/details", &details, NULL);
	//mg_set_uri_callback(ctx, "/break_down_infraction_code", &break_down_infraction_code, NULL);
	
	while (!received_kill)
	{
		usleep(999999); //pause();//sleep(10000);
		if (received_kill) { fprintf(stderr, "received kill\n"); }
	}
	mg_stop(ctx);
	
	return EXIT_SUCCESS;
}