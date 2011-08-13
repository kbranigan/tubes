
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
  FILE * lines_fp = NULL;
  int c;
  while ((c = getopt(argc, argv, "a:f:")) != -1)
  switch (c)
  {
    case 'a':
      strncpy(attribute, optarg, sizeof(attribute));
      break;
    case 'f':
      lines_fp = fopen(optarg, "r");
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
  double * line_lengths = NULL;
  
  struct Shape * shape = NULL;
  while ((shape = read_shape(lines_fp)))
  {
    num_lines++;
    lines = (struct Shape**)realloc(lines, sizeof(struct Shape*)*num_lines);
    line_lengths = (double*)realloc(line_lengths, sizeof(double)*num_lines);
    lines[num_lines-1] = shape;
    line_lengths[num_lines-1] = 0;
    int i;
    float * pv = NULL;
    for (i = 0 ; i < shape->num_vertexs ; i++)
    {
      float * v = get_vertex(shape, 0, i);
      if (pv != NULL)
        line_lengths[num_lines-1] += distance_between(v[1], v[0], pv[1], pv[0]);
      pv = v;
    }
    write_shape(pipe_out, shape);
  }
  
  if (num_lines == 0)
  {
    fprintf(pipe_err, "ERROR: %s: No lines provided in -f [line strips file]\n", argv[0]);
  }
  
  int removed = 0;
  while ((shape = read_shape(pipe_in)))
  {
    struct Shape * line = NULL;
    {
      int i;
      for (i = 0 ; i < num_lines ; i++)
      {
        if (attribute == NULL && line->unique_set_id == shape->unique_set_id)
        {
          line = lines[i];
          break;
        }
        line = NULL;
      }
    }
    line = lines[0];
    
    //shape->gl_type = GL_POINTS;
    struct VertexArray * va = get_or_add_array(shape, GL_VERTEX_ARRAY);
    
    int k;
    for (k = 0 ; k < shape->num_vertexs ; k++)
    {
      float * orig_v = get_vertex(shape, 0, k);
      
      float closest_dist = 10000;
      float closest_angle = 0;
      float * closest_v = NULL;
      float * closest_pv = NULL;

      float lat_diff, lng_diff, dist1_2, dist1_m, dist2_m, angle1_2, angle1_m, angle2_m, angle_m_1_2, angle_1_2_m, dist_g;
      
      int i,j;
      float * pv = NULL;
      for (i = 0 ; i < num_lines ; i++)
      {
        struct Shape * line = lines[i];
        for (j = 0 ; j < line->num_vertexs ; j++)
        {
          float * v = get_vertex(line, 0, j);
          if (pv == NULL) { pv = v; continue; }
          
          lat_diff = v[1] - pv[1];
          lng_diff = v[0] - pv[0];
          dist1_2 = sqrt(lat_diff*lat_diff + lng_diff*lng_diff);
          angle1_2 = atan2(lng_diff, lat_diff);
          
          lat_diff = orig_v[1] - pv[1];
          lng_diff = orig_v[0] - pv[0];
          dist1_m = sqrt(lat_diff*lat_diff + lng_diff*lng_diff);
          angle1_m = atan2(lng_diff, lat_diff);
          
          lat_diff = orig_v[1] - v[1];
          lng_diff = orig_v[0] - v[0];
          dist2_m = sqrt(lat_diff*lat_diff + lng_diff*lng_diff);
          angle2_m = atan2(lng_diff, lat_diff);
          
          if (dist2_m < fabs(closest_dist))
          {
            closest_dist = dist2_m;
            closest_angle = angle2_m+3.141592654;
            closest_pv = v;
            closest_v = v;
          }
          
          if (dist1_m < fabs(closest_dist))
          {
            closest_dist = dist1_m;
            closest_angle = angle1_m+3.141592654;
            closest_pv = pv;
            closest_v = pv;
          }
          
          angle_m_1_2 = -1 * (angle1_m - angle1_2);
          angle_1_2_m = -1 * (angle1_2 - angle2_m);
          
          if (angle_m_1_2 > 3.141592654) angle_m_1_2 -= 6.283185308;
          if (angle_m_1_2 < -3.141592654) angle_m_1_2 += 6.283185308;
          
          if (angle_1_2_m > 3.141592654) angle_1_2_m -= 6.283185308;
          if (angle_1_2_m < -3.141592654) angle_1_2_m += 6.283185308;
          
          dist_g = sin(angle_m_1_2) * dist1_m; // length of perpendicular (law of sines)
          
          if (!(angle_m_1_2 > 1.570796327 || angle_m_1_2 < -1.570796327 || (angle_1_2_m < 1.570796327 && angle_1_2_m > -1.570796327)))
          {
            if (fabs(dist_g) < fabs(closest_dist))
            {
              closest_dist = dist_g;
              closest_angle = angle1_2+1.570796327;
              closest_pv = pv;
              closest_v = v;
            }
          }
          
          pv = v;
        } // line vertexs
      }
      
      if (closest_dist < 0.0001)
      {
        orig_v[1] = orig_v[1] + cos(closest_angle)*closest_dist;
        orig_v[0] = orig_v[0] + sin(closest_angle)*closest_dist;
      }
      else
      {
        k--;
        shape->num_vertexs--;
        memmove(orig_v, orig_v + va->num_dimensions, sizeof(float)*(shape->num_vertexs-k) * va->num_dimensions);
        removed++;
      }
    } // shape vertexs
    
    
    /* manipulate data here if you like */
    write_shape(pipe_out, shape);
    free_shape(shape);
  }
  fprintf(pipe_err, "removed = %d\n", removed);
}
