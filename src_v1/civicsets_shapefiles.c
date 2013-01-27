
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "mongoose.h"
#include "ext/shapefil.h"

char data_path[] = "/work/data";
struct mg_connection * dconn = NULL;
int general_count = 0;

int file_exists(const char * fmt, ...)
{
  char filename[1000];
  
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(filename, sizeof(filename), fmt, ap);
  va_end(ap);
  
  FILE * fp;
  fp = fopen(filename, "r");
  fclose(fp);
  return (fp != NULL);
}

static int display_info(const char * fpath, const struct stat * sb, int tflag, struct FTW * ftwbuf)
{
  if (fpath[ftwbuf->base] == '.') return 0;
  if (strcmp(fpath + strlen(fpath) - 4, ".dbf") != 0) return 0;
  
  char file[400];
  strncpy(file, fpath + strlen(data_path), 400);
  file[strlen(file)-4] = 0;
  
  DBFHandle dbf = DBFOpen(fpath, "rb");
  SHPHandle shp = SHPOpen(fpath, "rb");
  
  general_count++;
  
  char * formatc = mg_get_var(dconn, "format");
  int json = 0;
  if (formatc != NULL && strcmp(formatc, "json") == 0)
    json = 1;
  free(formatc);
  
  if (json)
  {
    mg_printf(dconn, "%s\n  {", ((general_count==1) ? "" : ","));
    mg_printf(dconn, "\"file\":\"%s\"", file);
    
    if (dbf) mg_printf(dconn, ", \"dbfRecords\":%d", dbf->nRecords);
    if (dbf) mg_printf(dconn, ", \"dbfFields\":%d", dbf->nFields);
    
    mg_printf(dconn, ", \"image 0\": \"/image?file=%s&id=0\"", file);
    mg_printf(dconn, ", \"full image\": \"/image?file=%s\"", file);
    mg_printf(dconn, "}");
  }
  else
  {
    mg_printf(dconn, "<tr>");
    mg_printf(dconn, "<td>%s</td>", file);
    if (dbf) mg_printf(dconn, "<td><a href='/shapefiles/fields?file=%s'>fields</a></td>", file);
    if (shp) mg_printf(dconn, "<td><a href='/shapefiles/png?file=%s'>image</a></td>\n", file);
    mg_printf(dconn, "</tr>");
  }
  
  if (shp) SHPClose(shp);
  if (dbf) DBFClose(dbf);
  
  return 0; // To tell nftw() to continue
}

void shapefiles_list(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  char * formatc = mg_get_var(conn, "format");
  int json = 0;
  if (formatc != NULL && strcmp(formatc, "json") == 0)
    json = 1;
  free(formatc);
  
  if (json)
  {
    mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/javascript\r\nConnection: close\r\n\r\n");
    mg_printf(conn, "[");
  }
  else
  {
    mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
    mg_printf(conn, "<table>");
  }
  
  general_count = 0;
  dconn = conn; // dconn used in display_info callback
  int flags = FTW_DEPTH | FTW_PHYS;
  if (nftw(data_path, display_info, 20, flags) == -1) mg_printf(conn, "nftw FAILED\n");
  dconn = NULL;
  
  if (json)
    mg_printf(conn, "\n]\n");
  else
    mg_printf(conn, "</table>");
}

void shapefiles_png(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  char * file = mg_get_var(conn, "file");
  if (file == NULL) { mg_printf(conn, "You need to specify a file."); return; }
  
  char * row_id_c = mg_get_var(conn, "id");
  long row_id = (row_id_c == NULL) ? -1 : atoi(row_id_c);
  free(row_id_c);
  
  char * part_id_c = mg_get_var(conn, "part");
  long part_id = (part_id_c == NULL) ? -1 : atoi(part_id_c);
  free(part_id_c);
  
  char dbf_filename[200];
  sprintf(dbf_filename, "/work/data%s.dbf", file);
  
  DBFHandle dbf = DBFOpen(dbf_filename, "rb");
  if (dbf == NULL) { mg_printf(conn, "DBFOpen error (%s)\n", dbf_filename); return; }
  
  SHPHandle shp = SHPOpen(dbf_filename, "rb");
  if (shp == NULL) { mg_printf(conn, "SHPOpen error (%s)\n", dbf_filename); return; }
  
  mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
  
  mg_printf(conn, "<html>\n<head>\n<title>Civicsets.ca</title>\n</head>\n<body>\n<a href='/shapefiles'>back to list</a><br />\n");
  
  char * buf;
  char * cwd;
  long cwd_length = pathconf(".", _PC_PATH_MAX);
  
  if ((buf = (char *)malloc((size_t)cwd_length)) != NULL)
    cwd = getcwd(buf, (size_t)cwd_length);
  
  FILE * fp = NULL;
  
  char png_filename[550];
  sprintf(png_filename, "%s/cache_images%s/%ld.%ld.png", cwd, file, row_id, part_id);
  fp = fopen(png_filename, "r");
  sprintf(png_filename, "/cache_images%s/%ld.%ld.png", file, row_id, part_id);
  if (fp == NULL)
  {
    char command[1500];
    sprintf(command, "mkdir -p %s/cache_images%s", cwd, file);
    system(command);
    
    sprintf(command, "%s/bin/read_shapefile -a 0 -f \"%s\" -r \"%ld\" -p \"%ld\" | ", cwd, dbf_filename, row_id, part_id);
    
    SHPObject * psShape = SHPReadObject(shp, 0);
    if (psShape->nSHPType == SHPT_POLYGON)
    {
      strcat(command, cwd); strcat(command, "/bin/tesselate | ");
    }
    strcat(command, cwd); strcat(command, "/bin/add_random_colors | ");
    strcat(command, cwd); strcat(command, "/bin/write_png -w 500 -f ");
    strcat(command, cwd); strcat(command, png_filename);
    
    printf("%s\n", command);
    system(command);
  }
  else fclose(fp);
  
  char pszFieldName[12];
  int pnWidth; int pnDecimals;
  char type_names[5][20] = {"string", "integer", "double", "logical", "invalid"};
  
  if (row_id != -1)
  {
    mg_printf(conn, "<table>\n");
    int i = 0;
    for (i = 0 ; i < dbf->nFields ; i++)
    {
      DBFFieldType ft = DBFGetFieldInfo(dbf, i, pszFieldName, &pnWidth, &pnDecimals);
      mg_printf(conn, "<tr><td>%s</td><td>%s(%d)</td><td>%s</td></tr>\n", pszFieldName, type_names[ft], pnWidth, (char *)DBFReadStringAttribute(dbf, row_id, i));
    }
    mg_printf(conn, "</table>\n");
  }
  
  mg_printf(conn, "<img src='%s' />\n", png_filename);
  
  mg_printf(conn, "</body>\n</html>\n");
  
  if (dbf != NULL) DBFClose(dbf);
  if (shp != NULL) SHPClose(shp);
}

void shapefiles_fields(struct mg_connection *conn, const struct mg_request_info *ri, void *data)
{
  char * file = mg_get_var(conn, "file");
  if (file == NULL) { mg_printf(conn, "You need to specify a file."); return; }
  
  char filename[100];
  sprintf(filename, "/work/data/%s", file);
  
  DBFHandle dbf = DBFOpen(filename, "rb");
  if (dbf == NULL) { mg_printf(conn, "DBFOpen error (%s)\n", filename); return; }
  
  int nRecordCount = DBFGetRecordCount(dbf);
  int nFieldCount = DBFGetFieldCount(dbf);
  
  mg_printf(conn, "{\n");
  mg_printf(conn, "  \"file\": \"%s\",\n", file);
  mg_printf(conn, "  \"num_records\": \"%d\",\n", nRecordCount);
  mg_printf(conn, "  \"fields\": {\n");
  for (int i = 0 ; i < nFieldCount ; i++)
  {
    char pszFieldName[12];
    int pnWidth; int pnDecimals;
    char type_names[5][20] = {"string", "integer", "double", "logical", "invalid"};
    
    DBFFieldType ft = DBFGetFieldInfo(dbf, i, pszFieldName, &pnWidth, &pnDecimals);
    mg_printf(conn, "    \"%d\": {\n", i);
    mg_printf(conn, "      \"name\":\"%s\",\n", pszFieldName);
    if (pnWidth != 0) mg_printf(conn, "      \"width\":\"%d\",\n", pnWidth);
    if (pnDecimals != 0) mg_printf(conn, "      \"decimals\":\"%d\",\n", pnDecimals);
    mg_printf(conn, "      \"type\":\"%s\"\n", type_names[ft]);
    mg_printf(conn, "    }%s\n", (i==nFieldCount-1)?"":",");
  }
  mg_printf(conn, "  }\n");
  mg_printf(conn, "}\n");
  
  if (dbf != NULL) DBFClose(dbf);
  free(file);
}
