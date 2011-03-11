
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ftw.h>
#include <stdint.h>

#include <mysql.h>

#include "shapefile_src/shapefil.h"
#include "mongoose.h"

void list(struct mg_connection *conn, const struct mg_request_info *ri, void *data);
void fields(struct mg_connection *conn, const struct mg_request_info *ri, void *data);
void records(struct mg_connection *conn, const struct mg_request_info *ri, void *data);
void shapes(struct mg_connection *conn, const struct mg_request_info *ri, void *data);
void image(struct mg_connection *conn, const struct mg_request_info *ri, void *data);
void record_a_position(struct mg_connection *conn, const struct mg_request_info *ri, void *data);

const char *port = "2222";

int main(int argc, char **argv)
{
  struct mg_context *ctx = mg_start();
  mg_set_option(ctx, "dir_list", "no");  // Set document root
  int ret = 0;
  ret = mg_set_option(ctx, "ports", "2222");
  while (ret != 1)
  {
    sleep(1);
    ret = mg_set_option(ctx, "ports", "2222");
  }
  mg_set_uri_callback(ctx, "/", &list, NULL);
  mg_set_uri_callback(ctx, "/fields", &fields, NULL);
  mg_set_uri_callback(ctx, "/records", &records, NULL);
  mg_set_uri_callback(ctx, "/shapes", &shapes, NULL);
  mg_set_uri_callback(ctx, "/image", &image, NULL);
  mg_set_uri_callback(ctx, "/record_a_position", &record_a_position, NULL);
  
  printf("-------------------------------------------------------------------------\n");
  
  for (;;) sleep(10000);
  mg_stop(ctx);
  
}

char data_path[] = "/work/data";
struct mg_connection *dconn = NULL;
static int display_info(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
{
  if (fpath[ftwbuf->base] == '.') return 0;
  if (strcmp(fpath + strlen(fpath) - 4, ".shp") != 0) return 0;
  
  char file[200];
  strncpy(file, fpath + strlen(data_path), 200);
  file[strlen(file)-4] = 0;
  
  mg_printf(dconn, "<tr><td>%s</td>", file);
  mg_printf(dconn, "<td><a href='/fields?file=%s'>fields</a></td>", file);
  mg_printf(dconn, "<td><a href='/records?file=%s&id=0'>record 0</a></td>", file);
  mg_printf(dconn, "<td><a href='/shapes?file=%s&id=0'>shape 0</a></td>", file);
  mg_printf(dconn, "<td><a href='/image?file=%s&id=0'>image 0</a></td>", file);
  mg_printf(dconn, "<td><a href='/image?file=%s'>full image</a></td>", file);
  mg_printf(dconn, "</tr>\n");
  
  //mg_printf(dconn, "%s (%d) (%s)<br />\n", fpath + strlen(fpath) - 4);
  return 0;           /* To tell nftw() to continue */
}

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
  
  MYSQL mysql;
  //MYSQL_ROW row;
  
  if ((mysql_init(&mysql) == NULL)) { printf("mysql_init error\n"); }
  if (!mysql_real_connect(&mysql, "localhost", "root", "", "civicsets", 0, NULL, 0)) { printf("mysql_real_connect error\n"); }
  
  char query[1000];
  sprintf(query, "INSERT INTO points (created_at, recorded_at, source, lat, lon, altitude, speed, course, haccuracy, vaccuracy, heading) values (NOW(), %ld, '%s', %f, %f, %f, %f, %f, %f, %f, %f)", recorded_at, source_c, lat, lon, altitude, speed, course, haccuracy, vaccuracy, heading);
  mg_printf(conn, "%s", query);
  mysql_query(&mysql, query);
  mysql_close(&mysql);
  
  free(source_c);
  free(recorded_at_c);
  free(lat_c);
  free(lon_c);
}

void list(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  mg_printf(conn, "<h1>Available shapefiles</h1>\n");
  mg_printf(conn, "<table>\n");
  
  dconn = conn;
  int flags = FTW_DEPTH | FTW_PHYS;
  if (nftw(data_path, display_info, 20, flags) == -1) mg_printf(conn, "nftw FAILED\n");
  dconn = NULL;
  
  mg_printf(conn, "</table>\n");
}

void fields(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  char * file = mg_get_var(conn, "file");
  if (file == NULL) { mg_printf(conn, "You need to specify a file."); return; }
  
  char filename[100];
  sprintf(filename, "/work/data/%s", file);
  
  DBFHandle d = DBFOpen(filename, "rb");
  if (d == NULL) { mg_printf(conn, "DBFOpen error (%s)\n", filename); return; }
	
  int nRecordCount = DBFGetRecordCount(d);
  int nFieldCount = DBFGetFieldCount(d);
  
  mg_printf(conn, "{\n");
  mg_printf(conn, "  \"file\": \"%s\",\n", file);
  mg_printf(conn, "  \"num_records\": \"%d\",\n", nRecordCount);
  mg_printf(conn, "  \"fields\": {\n");
  for (int i = 0 ; i < nFieldCount ; i++)
  {
    char pszFieldName[12];
    int pnWidth; int pnDecimals;
    char type_names[5][20] = {"string", "integer", "double", "logical", "invalid"};
    
    DBFFieldType ft = DBFGetFieldInfo(d, i, pszFieldName, &pnWidth, &pnDecimals);
    mg_printf(conn, "    \"%d\": {\n", i);
    mg_printf(conn, "      \"name\":\"%s\",\n", pszFieldName);
    mg_printf(conn, "      \"type\":\"%s\",\n", type_names[ft]);
    if (pnWidth != 0) mg_printf(conn, "      \"width\":\"%d\",\n", pnWidth);
    if (pnDecimals != 0) mg_printf(conn, "      \"decimals\":\"%d\"\n", pnDecimals);
    mg_printf(conn, "    }%s\n", (i==nFieldCount-1)?"":",");
  }
  mg_printf(conn, "  }\n");
  mg_printf(conn, "}\n");
	
	if (d != NULL) DBFClose(d);
  free(file);
}

void records(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  char * file = mg_get_var(conn, "file");
  if (file == NULL) { mg_printf(conn, "You need to specify a file."); return; }
  
  char * id_c = mg_get_var(conn, "id");
  //if (id_c == NULL) { mg_printf(conn, "You need to specify an id."); return; }
  long id = (id_c != NULL) ? atoi(id_c) : -1;
  free(id_c);
  
  char filename[100];
  sprintf(filename, "/work/data/%s", file);
  
  DBFHandle d = DBFOpen(filename, "rb");
  if (d == NULL) { mg_printf(conn, "DBFOpen error (%s)\n", filename); return; }
	
	int nFieldCount = DBFGetFieldCount(d);
  int nRecordCount = DBFGetRecordCount(d);
  
  mg_printf(conn, "{\n");
  mg_printf(conn, "  \"file\": \"%s\",\n", file);
  
  if (id == -1)
  {
    mg_printf(conn, "  \"num_records\": \"%d\",\n", nRecordCount);
    mg_printf(conn, "  \"records\":\n  [\n");
  }
  else
    mg_printf(conn, "  \"record\":\n");
  
  for (int i = 0 ; i < nRecordCount ; i++)
  {
    if (id != -1 && id != i) continue;
    mg_printf(conn, "    {\n");
    mg_printf(conn, "      \"id\": \"%d\",\n", i);
    for (int j = 0 ; j < nFieldCount ; j++)
    {
      unsigned int k;
      char pszFieldName[12];
      int pnWidth;
      char *cStr;
    
      DBFFieldType ft = DBFGetFieldInfo(d, j, pszFieldName, &pnWidth, NULL);
      mg_printf(conn, "      \"%s\":", pszFieldName);
      switch (ft)
      {
        case FTString:
          cStr = (char *)DBFReadStringAttribute(d, i, j);
          for (k = 0 ; k < strlen(cStr) ; k++) if (cStr[k] == '"') cStr[k] = '\'';
          mg_printf(conn, "\"%s\"", cStr);
          break;
        case FTInteger:
          mg_printf(conn, "\"%d\"", DBFReadIntegerAttribute(d, i, j));
          break;
        case FTDouble:
          mg_printf(conn, "\"%f\"", DBFReadDoubleAttribute(d, i, j));
          break;
        case FTLogical:
          break;
        case FTInvalid:
          break;
      }
      mg_printf(conn, "%s\n", (j == nFieldCount-1)?"":",");
    }
    mg_printf(conn, "    }%s\n", (id != -1 || i == nRecordCount-1)?"":",");
  }
  if (id == -1)
    mg_printf(conn, "  ]\n");
  
  mg_printf(conn, "}\n");
	
	if (d != NULL) DBFClose(d);
  free(file);
}

void shapes(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  char * file = mg_get_var(conn, "file");
  if (file == NULL) { mg_printf(conn, "You need to specify a file."); return; }
  
  char * id_c = mg_get_var(conn, "id");
  if (id_c == NULL) { mg_printf(conn, "You need to specify an id."); return; }
  long id = atoi(id_c);
  free(id_c);
  
  char filename[100];
  sprintf(filename, "/work/data/%s", file);
  
	SHPHandle h = SHPOpen(filename, "rb");
  if (h == NULL) { mg_printf(conn, "SHPOpen error (%s)\n", filename); return; }
	
	SHPObject	*psShape = SHPReadObject(h, id);
  int j, iPart;
  
  mg_printf(conn, "{\n");
  mg_printf(conn, "  \"file\": \"%s\",\n", file);
  mg_printf(conn, "  \"record_id\": \"%d\",\n", id);
  mg_printf(conn, "  \"shape_parts\": {\n");
  for (iPart = 0 ; iPart < psShape->nParts ; iPart++)
  {
    int start = psShape->panPartStart[iPart];
    int end = psShape->nVertices;
    
    if (iPart != psShape->nParts - 1) end = psShape->panPartStart[iPart+1];
    
    mg_printf(conn, "    \"%d\":[", iPart);
    for (j = start ; j < end ; j++)
    {
      mg_printf(conn, "[%f,%f]%s", psShape->padfX[j], psShape->padfY[j], (j==end-1)?"":",");
    }
    mg_printf(conn, "]%s\n", (iPart==psShape->nParts-1)?"":",");
  }
  mg_printf(conn, "  }\n");
  mg_printf(conn, "}\n");
  
  if (psShape != NULL) SHPDestroyObject(psShape);
	if (h != NULL) SHPClose(h);
}
  
void image(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  char * file = mg_get_var(conn, "file");
  if (file == NULL) { mg_printf(conn, "You need to specify a file."); return; }
  
  char * row_id_c = mg_get_var(conn, "id");
  long row_id = (row_id_c == NULL) ? -1 : atoi(row_id_c);
  
  char * part_id_c = mg_get_var(conn, "part");
  long part_id = (part_id_c == NULL) ? -1 : atoi(part_id_c);
  
  char dbf_filename[200];
  sprintf(dbf_filename, "/work/data%s.dbf", file);
  
	SHPHandle h = SHPOpen(dbf_filename, "rb");
  if (h == NULL) { mg_printf(conn, "SHPOpen error (%s)\n", dbf_filename); return; }
  
  DBFHandle d = DBFOpen(dbf_filename, "rb");
  if (d == NULL) { mg_printf(conn, "DBFOpen error (%s)\n", dbf_filename); return; }
  
  //int nRecordCount = DBFGetRecordCount(d);
  
  mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
  
  mg_printf(conn, "<html>\n<head>\n<title>Civicsets.ca</title>\n</head>\n<body>\n<a href='/'>back to list</a><br />\n");
  
  char dir[] = ".";
  
  FILE * fp = NULL;
  char image_filename[300];
  sprintf(image_filename, "cache_images%s/%ld.%ld", file, row_id, part_id);
  
  char png_filename[350];
  sprintf(png_filename, "%s.png", image_filename);
  fp = fopen(png_filename, "r");
  if (fp == NULL)
  {
    char command[1000];
    sprintf(command, "mkdir -p cache_images%s", file);
    system(command);
    
    sprintf(command, "%s/read_shapefile %s %ld %ld | ", dir, dbf_filename, row_id, part_id);
    
    SHPObject	*psShape = SHPReadObject(h, 0);
    if(psShape->nSHPType == SHPT_POLYGON)
    {
      strcat(command, dir); strcat(command, "/tesselate | ");
    }
    strcat(command, dir); strcat(command, "/add_random_colors | ");
    strcat(command, dir); strcat(command, "/write_png "); strcat(command, image_filename); strcat(command, ".png");
    
    printf("%s\n", command);
    system(command);
    
  }
  else fclose(fp);
  
  mg_printf(conn, "<img src='%s.png' />\n", image_filename, image_filename);
  
  mg_printf(conn, "</body>\n</html>\n");
  
  if (h != NULL) SHPClose(h);
	if (d != NULL) DBFClose(d);
  free(row_id_c);
  free(part_id_c);
}