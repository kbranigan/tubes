
/*

make
./bin/read_mysql "SELECT x, y, unique_set_id, created_at, secsSinceReport from temp_nextbus.points where dirTag = '300_0_ba300' order by unique_set_id, created_at" \
| ./bin/align_points_to_line_strips -f data/ttc.300.shape.colors.b \
| ./bin/write_png

*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDINOUT_ARE_PIPED
#define SCHEME_FUNCTION align_points_to_line_strips
#include "scheme.h"

static const double DEG_TO_RAD = 0.017453292519943295769236907684886;
static const double EARTH_RADIUS_IN_METERS = 6372797.560856;

double distance_between(double from_lat, double from_lng, double to_lat, double to_lng)
{
  double latitudeArc  = (from_lat - to_lat) * DEG_TO_RAD;
  double longitudeArc = (from_lng - to_lng) * DEG_TO_RAD;
  double latitudeH = sin(latitudeArc * 0.5);
  latitudeH *= latitudeH;
  double lontitudeH = sin(longitudeArc * 0.5);
  lontitudeH *= lontitudeH;
  double tmp = cos(from_lat*DEG_TO_RAD) * cos(to_lat*DEG_TO_RAD);
  return EARTH_RADIUS_IN_METERS * (2.0 * asin(sqrt(latitudeH + tmp*lontitudeH)));
}

int align_points_to_line_strips(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char attribute[50] = "";
  
  int include_lines_in_output = 0;
  
  FILE * lines_fp = NULL;
  int c;
  while ((c = getopt(argc, argv, "a:f:i")) != -1)
  switch (c)
  {
    case 'a':
      strncpy(attribute, optarg, sizeof(attribute));
      break;
    case 'f':
      lines_fp = fopen(optarg, "r");
      break;
    case 'i':
      include_lines_in_output = 1;
      break;
    default:
      abort();
  }
  
  if (lines_fp == NULL)
  {
    fprintf(pipe_err, "Usage: %s -f [line strips file]\n", argv[0]);
    return 0;
  }
  
  int num_lines = 0;
  struct Shape ** lines = NULL;
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(lines_fp)))
  {
    num_lines++;
    lines = (struct Shape**)realloc(lines, sizeof(struct Shape*)*num_lines);
    lines[num_lines-1] = shape;
    
    float distance_along_line = 0;
    
    float * pv = NULL;
    int j;
    for (j = 0 ; j < shape->num_vertexs ; j++)
    {
      float * v = get_vertex(shape, 0, j);
      if (pv == NULL) { pv = v; continue; }
      
      distance_along_line += distance_between(pv[1], pv[0], v[1], v[0]);
      pv = v;
    }
    
    char temp[25];
    sprintf(temp, "%.5f", distance_along_line);
    set_attribute(shape, "length", temp);
    
    if (include_lines_in_output) // default 0
      write_shape(pipe_out, shape);
    
    //draw_marker(get_vertex(shape, 0, 0), 0.001);
    
    // these shapes are used later and will therefore be free'd later
  }
  
  if (num_lines == 0)
  {
    fprintf(pipe_err, "ERROR: %s: No lines provided in -f [line strips file]\n", argv[0]);
  }
  
  int shapes_removed = 0;
  int shapes_total = 0;
  
  int vertexs_removed = 0;
  int vertexs_total = 0;
  while ((shape = read_shape(pipe_in)))
  {
    shapes_total++;
    vertexs_total += shape->num_vertexs;
    
    struct VertexArray * va = get_or_add_array(shape, GL_VERTEX_ARRAY);
    
    int k;
    for (k = 0 ; k < shape->num_vertexs ; k++)
    {
      float * orig_v = get_vertex(shape, 0, k);
      
      float closest_dist = 10000;
      float closest_angle = 0;
      float * closest_v = NULL;
      float * closest_pv = NULL;

      float lat_diff, lng_diff, dist1_m, dist2_m, angle1_2, angle1_m, angle2_m, angle_m_1_2, angle_1_2_m, dist_g;
      
      int i,j;
      float * pv = NULL;
      for (i = 0 ; i < num_lines ; i++)
      {
        float distance_along_line = 0;
        struct Shape * line = lines[i];
        for (j = 0 ; j < line->num_vertexs ; j++)
        {
          float * v = get_vertex(line, 0, j);
          if (pv == NULL) { pv = v; continue; }
          
          lat_diff = v[1] - pv[1];
          lng_diff = v[0] - pv[0];
          angle1_2 = atan2(lng_diff, lat_diff);
          
          lat_diff = orig_v[1] - pv[1];
          lng_diff = orig_v[0] - pv[0];
          dist1_m = sqrt(lat_diff*lat_diff + lng_diff*lng_diff);
          angle1_m = atan2(lng_diff, lat_diff);
          
          lat_diff = orig_v[1] - v[1];
          lng_diff = orig_v[0] - v[0];
          dist2_m = sqrt(lat_diff*lat_diff + lng_diff*lng_diff);
          angle2_m = atan2(lng_diff, lat_diff);
          
          angle_m_1_2 = -1 * (angle1_m - angle1_2);
          angle_1_2_m = -1 * (angle1_2 - angle2_m);
          
          if (angle_m_1_2 > 3.141592654) angle_m_1_2 -= 6.283185308;
          if (angle_m_1_2 < -3.141592654) angle_m_1_2 += 6.283185308;
          
          if (angle_1_2_m > 3.141592654) angle_1_2_m -= 6.283185308;
          if (angle_1_2_m < -3.141592654) angle_1_2_m += 6.283185308;
          
          dist_g = sin(angle_m_1_2) * dist1_m; // length of perpendicular (law of sines)
          
          /*if (dist2_m < dist1_m && dist2_m < fabs(closest_dist))
          {
            closest_dist = dist2_m;
            closest_angle = angle2_m + 3.141592654;
            closest_pv = v;
            closest_v = v;
            draw_marker(v, 0.001);
          }
          
          if (dist1_m < dist2_m && dist1_m < fabs(closest_dist))
          {
            closest_dist = dist1_m;
            closest_angle = angle1_m + 3.141592654;
            closest_pv = pv;
            closest_v = pv;
            draw_marker(pv, 0.001);
          }*/
          
          if (!(angle_m_1_2 > 1.570796327 || angle_m_1_2 < -1.570796327 || (angle_1_2_m < 1.570796327 && angle_1_2_m > -1.570796327)))
          {
            if (fabs(dist_g) < fabs(closest_dist))
            {
              closest_dist = dist_g;
              closest_angle = angle1_2 + 1.570796327;
              closest_pv = pv;
              closest_v = v;
            }
          }
          
          pv = v;
        } // line each vertexs
        
        //if (shape->num_vertexs == 1)
        {
          distance_along_line = 0;
          pv = NULL;
          for (j = 0 ; j < line->num_vertexs ; j++)
          {
            float * v = get_vertex(line, 0, j);
            if (pv == NULL) { pv = v; continue; }
            
            if (v == closest_v)
            {
              float nv[3] = { orig_v[0] + sin(closest_angle)*closest_dist, orig_v[1] + cos(closest_angle)*closest_dist, 0 };
              distance_along_line += distance_between(pv[1], pv[0], nv[1], nv[0]);
              distance_along_line += distance_between(nv[1], nv[0], orig_v[1], orig_v[0]);
              break;
            }
            else
            {
              distance_along_line += distance_between(pv[1], pv[0], v[1], v[0]);
            }
            pv = v;
          }
          
          char temp[20];
          char temp2[20];
          sprintf(temp, "dist_line_%d", line->unique_set_id);
          sprintf(temp2, "%.5f", distance_along_line);
          set_attribute(shape, temp, temp2);
        }
        
      } // num_lines
      
      if (closest_dist < 0.0001)
      {
        if (shape->num_vertexs == 1)
        {
          char temp[20];
          sprintf(temp, "%.5f", distance_between(orig_v[1], orig_v[0], orig_v[1] + cos(closest_angle)*closest_dist, orig_v[0] + sin(closest_angle)*closest_dist));
          set_attribute(shape, "closest_dist", temp);
        }
        
        orig_v[1] = orig_v[1] + cos(closest_angle)*closest_dist;
        orig_v[0] = orig_v[0] + sin(closest_angle)*closest_dist;
      }
      else
      {
        k--;
        shape->num_vertexs--;
        memmove(orig_v, orig_v + va->num_dimensions, sizeof(float)*(shape->num_vertexs-k) * va->num_dimensions);
        vertexs_removed++;
      }
    } // shape vertexs
    
    /* manipulate data here if you like */
    if (shape->num_vertexs > 0)
      write_shape(pipe_out, shape);
    else
      shapes_removed++;
    free_shape(shape);
  }
  
  {
    int i;
    for (i = 0 ; i < num_lines ; i++)
      free_shape(lines[i]);
  }
  free(lines);
  //free(line_lengths);
  
  //if (shapes_removed > 0)
    fprintf(pipe_err, "%s: shapes removed = %d of %d\n", argv[0], shapes_removed, shapes_total);
  
  //if (vertexs_removed > 0)
    fprintf(pipe_err, "%s: vertexs removed = %d of %d\n", argv[0], vertexs_removed, vertexs_total);
}
