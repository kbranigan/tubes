
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDOUT_IS_PIPED
#define SCHEME_FUNCTION read_dem

#include "scheme.h"

FILE * pFile = NULL;
long lSize;
char * buffer = NULL;
size_t result;

void get_chars(int count)
{
  buffer = (char*)realloc(buffer, count+1);
  
  result = fread(buffer, 1, count, pFile);
  buffer[count] = '\0';
}

int read_dem(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
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
  
  pFile = fopen(filename, "rb");
  if (pFile == NULL) { fputs("File error", stderr); exit(1); }
  
  fseek (pFile, 0, SEEK_END);
  lSize = ftell (pFile);
  rewind (pFile);
  
  get_chars(40+60+9); // junk
  get_chars(4); int lng_deg = atoi(buffer);
  get_chars(2); int lng_min = atoi(buffer);
  get_chars(7); float lng_sec = atof(buffer);
  get_chars(4); int lat_deg = atoi(buffer);
  get_chars(2); int lat_min = atoi(buffer);
  get_chars(7); float lat_sec = atof(buffer);
  
  float lng = (fabs(lng_deg) + lng_min/60.0 + lng_sec/3600.0) * (lng_deg < 0 ? -1 : 1);
  float lat = (fabs(lat_deg) + lat_min/60.0 + lat_sec/3600.0) * (lng_deg < 0 ? -1 : 1);
  //printf("%d %d %f (%f)\n", lng_deg, lng_min, lng_sec, lng);
  //printf("%d %d %f (%f)\n", lat_deg, lat_min, lat_sec, lat);
  
  get_chars(1+1+3+4+6+6+6+6+15*24+6+6+6); // junk
  
  get_chars(24); float sw_lng = atof(buffer) / 3600.00;
  get_chars(24); float sw_lat = atof(buffer) / 3600.00;
  get_chars(24); float nw_lng = atof(buffer) / 3600.00;
  get_chars(24); float nw_lat = atof(buffer) / 3600.00;
  get_chars(24); float ne_lng = atof(buffer) / 3600.00;
  get_chars(24); float ne_lat = atof(buffer) / 3600.00;
  get_chars(24); float se_lng = atof(buffer) / 3600.00;
  get_chars(24); float se_lat = atof(buffer) / 3600.00;
  //printf("%f %f\n", sw_lng, sw_lat);
  //printf("%f %f\n", nw_lng, nw_lat);
  //printf("%f %f\n", ne_lng, ne_lat);
  //printf("%f %f\n", se_lng, se_lat);
  
  get_chars(24); float min_elev = atof(buffer);
  get_chars(24); float max_elev = atof(buffer);
  //printf("%f %f\n", min_elev, max_elev);  // min/max
  
  get_chars(24+6);
  get_chars(12); float x_res = atof(buffer);
  get_chars(12); float y_res = atof(buffer);
  get_chars(12); float z_res = atof(buffer);
  //printf("%f %f %f\n", x_res, y_res, z_res);
  //  +2*6+5+
  //  1+5+1+4+4+1+1+2+
  //  2+2+4+4+4*2+7); // junk
  
  rewind(pFile);
  
  get_chars(1024); // Record type A
  
  int type_b_header_size = 2*6 + 2*6 + 2*24 + 24 + 2*24;
  int type_b_data_size = ceil((6*1201+type_b_header_size)/1024.0)*1024 - type_b_header_size;
  
  int col_id;
  for (col_id = 0 ; col_id < 1201 ; col_id++)
  {
    get_chars(6);
    int row = atoi(buffer);
    assert(row == 1);
    get_chars(6);
    int col = atoi(buffer);
    
    get_chars(6);
    int rn = atoi(buffer); // 1201 for canada
    assert(rn == 1201);
    get_chars(6);
    int n = atoi(buffer); // 1
    assert(n == 1);
    
    get_chars(24);
    float lng = atof(buffer) / 3600.00; // long in arc seconds
    get_chars(24);
    float lat = atof(buffer) / 3600.00; // lat in arc seconds
    //printf("%d %f %f\n", col, lng, lat);
    
    get_chars(24); // always 0.0
    
    get_chars(24);
    float min = atof(buffer); // min elevation
    get_chars(24);
    float max = atof(buffer); // max elevation
    
    float elevation_data[1201];
    
    int char_count = 0;
    int row_id = 0;
    while (row_id < 1201)
    {
      get_chars(6); //short alt = atoi(buffer);
      char_count += 6;
      elevation_data[row_id] = atof(buffer);
      
      struct Shape * shape = new_shape();
      shape->vertex_arrays[0].num_dimensions = 3;
      float v[3]= { lat+(row_id*y_res/3600.00), lng, elevation_data[row_id] };
      append_vertex(shape, v);
      write_shape(pipe_out, shape);
      free_shape(shape);
      
      if (elevation_data[row_id] == -32767) elevation_data[row_id] = 75; // kbfu, applies to toronto data only, USA is invalid but it's all water anyway
      
      row_id ++;
      if (((row_id - 146) % 170 == 0 && row_id != 0) || row_id == 146) { get_chars(4); char_count += 4; } // junk
    }
    if (type_b_data_size > char_count)
      get_chars(type_b_data_size - char_count); // junk
    
  }
  //get_chars(type_b_header_size);
  //printf("%s\n", buffer);
  //get_chars(ceil((6*1201+type_b_header_size)/1024.0)*1024 - type_b_header_size);
  //printf("%s------------\n", buffer);
  
  //char temp[100];
  //sprintf(temp, "cp %s 030/", fn);
  //printf("%s\n", temp);
  //system(temp);

  fclose(pFile);
  free(buffer);
}
