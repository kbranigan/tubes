
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/block.h"
#include "../src/block_kdtree.h"

// edge graph
struct Vertex {
  double x;
  double y;
  char lf_name[42];
  struct Vertex ** vout;
  int num_vout;
};
struct Vertex * vertices = NULL;
int num_vertices = 0;

void add_vout(struct Vertex * v, struct Vertex * vo)
{
  v->vout = (struct Vertex**)realloc(v->vout, sizeof(struct Vertex*)*(v->num_vout+1));
  v->vout[v->num_vout++] = vo;
}

struct Vertex get_nearest_edge_vertex(void * kdtree, float x, float y, char * lf_name)
{
  //int num_v = 0;
  struct kdtree_results v = { 0, NULL };
  double range = 0.002;
  //void ** v = NULL;
  while (v.count < 20)
  {
    range *= 2.0;
    v = search_kdtree_find_within_range(kdtree, x, y, range);
    //fprintf(stderr, "range = %f (num_v = %d)\n", range, v.count);
    //ids = nearest_osm_ids(y_t, x_t, range, &num_ids); // ids must be free'd
    //if (v.count < 3 && v.count != 0) free(v.row_ids);
    if (range > 10000) { fprintf(stderr, "DYING, DIDN'T FIND SHIT FUCK\n"); exit(0); }
  }
  
  float closest_dist = 10000;
  float closest_angle = 0;
  struct Vertex * closest_vertex1 = NULL;
  struct Vertex * closest_vertex2 = NULL;
  
  struct Vertex * prev_vertex1 = NULL;
  
  float y_diff, x_diff, dist1_2, dist1_m, dist2_m, angle1_2, angle1_m, angle2_m, angle_m_1_2, angle_1_2_m, dist_g;
  
  int i,j;
  for (i = 0 ; i < v.count ; i++)
  {
    struct Vertex * vertex1 = &vertices[v.row_ids[i]];
    //if (lf_name == NULL || lf_name[0] == 0) continue;
    if (lf_name != NULL && strcmp(vertex1->lf_name, lf_name) != 0) continue;
    //fprintf(stderr, "%d: %s %s %d %d\n", i, vertex1->lf_name, lf_name, strcmp(vertex1->lf_name, lf_name), vertex1->num_vout);
    
    for (j = 0 ; j < vertex1->num_vout ; j++)
    {
      struct Vertex * vertex2 = vertex1->vout[j];
      if (vertex2 == prev_vertex1 || vertex2 == vertex1) continue;
      y_diff = vertex2->y - vertex1->y;
      x_diff = vertex2->x - vertex1->x;
      dist1_2 = sqrt(y_diff*y_diff + x_diff*x_diff);
      angle1_2 = atan2(x_diff, y_diff);
      
      y_diff = y - vertex1->y;
      x_diff = x - vertex1->x;
      dist1_m = sqrt(y_diff*y_diff + x_diff*x_diff);
      angle1_m = atan2(x_diff, y_diff);
      
      y_diff = y - vertex2->y;
      x_diff = x - vertex2->x;
      dist2_m = sqrt(y_diff*y_diff + x_diff*x_diff);
      angle2_m = atan2(x_diff, y_diff);
      
      if (dist2_m < fabs(closest_dist))
      {
        closest_dist = dist2_m;
        closest_angle = angle2_m+3.141592654;
        closest_vertex1 = vertex2;
        closest_vertex2 = vertex2;
      }
      
      if (dist1_m < fabs(closest_dist))
      {
        closest_dist = dist1_m;
        closest_angle = angle1_m+3.141592654;
        closest_vertex1 = vertex1;
        closest_vertex2 = vertex1;
      }
      
      angle_m_1_2 = -1 * (angle1_m - angle1_2);
      angle_1_2_m = -1 * (angle1_2 - angle2_m);
      
      if (angle_m_1_2 > 3.141592654) angle_m_1_2 -= 6.283185308;
      if (angle_m_1_2 < -3.141592654) angle_m_1_2 += 6.283185308;
      
      if (angle_1_2_m > 3.141592654) angle_1_2_m -= 6.283185308;
      if (angle_1_2_m < -3.141592654) angle_1_2_m += 6.283185308;
      
      dist_g = sin(angle_m_1_2) * dist1_m; // length of perpendicular (law of sines)
      
      if (angle_m_1_2 > 1.570796327 || angle_m_1_2 < -1.570796327 || (angle_1_2_m < 1.570796327 && angle_1_2_m > -1.570796327)) continue;
      
      if (fabs(dist_g) < fabs(closest_dist))
      {
        closest_dist = dist_g;
        closest_angle = angle1_2+1.570796327;
        closest_vertex1 = vertex1;
        closest_vertex2 = vertex2;
      }
    }
    //fprintf(stderr, "closest_dist = %f\n", closest_dist);
    prev_vertex1 = vertex1;
  }
  
  free(v.row_ids);
  
  struct Vertex ret;
  memset(&ret, 0, sizeof(struct Vertex));
  ret.x = x + sin(closest_angle)*closest_dist;
  ret.y = y + cos(closest_angle)*closest_dist;
  if (closest_vertex1 != NULL) add_vout(&ret, closest_vertex1);
  if (closest_vertex2 != NULL) add_vout(&ret, closest_vertex2);
  
  if (closest_dist == 10000 && closest_angle == 0 && lf_name != NULL && lf_name[0] != 0)
  {
    ret.x = x;
    ret.y = y;
    return ret;
    //ret = get_nearest_edge_vertex(x, y, NULL); // try again without a street name :(
  }
  else if (closest_dist == 10000 && closest_angle == 0 && (lf_name == NULL || lf_name[0] == 0))
  {
    ret.x = x;
    ret.y = y;
    return ret;
  }
  
  return ret;
}

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  assert_stdin_is_piped();
  assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
  struct Block * addresses = read_block(stdin);
  
  FILE * centerline_fp = NULL;
  
  float scale = 1.0;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"centerline", required_argument, 0, 'c'},
      {"scale", required_argument, 0, 's'},
      //{"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "c:s:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'c': centerline_fp = fopen(optarg, "r"); break;
      //case 'a': addresses_fp = fopen(optarg, "r"); break;
      case 's': scale = atof(optarg); break;
      default: abort();
    }
  }
  
  if (centerline_fp == NULL) { fprintf(stderr, "centerline isn't provided or is invalid\n"); return EXIT_FAILURE; }
  //if (addresses_fp == NULL) { fprintf(stderr, "addresses isn't provided or is invalid\n"); return EXIT_FAILURE; }
  
  struct Block * centerline = read_block(centerline_fp); fclose(centerline_fp);
  //struct Block * addresses = read_block(addresses_fp); fclose(addresses_fp);
  
  //addresses = set_num_rows(addresses, 6);// kbfu
  
  /*struct Block * edges = new_block();
  edges = add_int_column(edges, const_cast<char *>("shape_row_id"));
  edges = add_int_column(edges, const_cast<char *>("shape_part_type"));
  edges = add_double_column(edges, const_cast<char *>("x"));
  edges = add_double_column(edges, const_cast<char *>("y"));*/
  
  //addresses = add_double_column(addresses, const_cast<char *>("street_left_x")); blank_column_values(addresses, const_cast<char *>("street_left_x"));
  //addresses = add_double_column(addresses, const_cast<char *>("street_left_y")); blank_column_values(addresses, const_cast<char *>("street_left_y"));
  //addresses = add_double_column(addresses, const_cast<char *>("street_right_x")); blank_column_values(addresses, const_cast<char *>("street_right_x"));
  //addresses = add_double_column(addresses, const_cast<char *>("street_right_y")); blank_column_values(addresses, const_cast<char *>("street_right_y"));
  
  //edges = set_num_rows(edges, addresses->num_rows*2);
  
  void * kdtree = create_kdtree_for_block(centerline);
  
  int x_column_id = get_column_id_by_name(addresses, const_cast<char *>("x"));
  int y_column_id = get_column_id_by_name(addresses, const_cast<char *>("y"));
  int radius_column_id = get_column_id_by_name(addresses, const_cast<char *>("radius"));
  int lf_name_column_id = get_column_id_by_name(addresses, const_cast<char *>("LF_NAME"));
  //int street_left_x_column_id = get_column_id_by_name(addresses, const_cast<char *>("street_left_x"));
  //int street_left_y_column_id = get_column_id_by_name(addresses, const_cast<char *>("street_left_y"));
  //int street_right_x_column_id = get_column_id_by_name(addresses, const_cast<char *>("street_right_x"));
  //int street_right_y_column_id = get_column_id_by_name(addresses, const_cast<char *>("street_right_y"));
  
  int centerline_shape_row_id_column_id = get_column_id_by_name(centerline, const_cast<char *>("shape_row_id"));
  if (centerline_shape_row_id_column_id == -1) centerline_shape_row_id_column_id = get_column_id_by_name(centerline, const_cast<char *>("row_id"));
  int centerline_x_column_id = get_column_id_by_name_or_exit(centerline, const_cast<char *>("x"));
  int centerline_y_column_id = get_column_id_by_name_or_exit(centerline, const_cast<char *>("y"));
  int centerline_lf_name_column_id = get_column_id_by_name_or_exit(centerline, const_cast<char *>("LF_NAME"));
  
  num_vertices = centerline->num_rows;
  vertices = (struct Vertex*)realloc(vertices, sizeof(struct Vertex)*num_vertices);
  memset(vertices, 0, sizeof(struct Vertex)*num_vertices);
  int row_id, prev_shape_row_id = -1;
  for (row_id = 0 ; row_id < centerline->num_rows ; row_id++)
  {
    double x = get_cell_as_double(centerline, row_id, centerline_x_column_id);
    double y = get_cell_as_double(centerline, row_id, centerline_y_column_id);
    int32_t shape_row_id = get_cell_as_int32(centerline, row_id, centerline_shape_row_id_column_id);
    char * lf_name = (char*)get_cell(centerline, row_id, centerline_lf_name_column_id);
    
    vertices[row_id].x = x;
    vertices[row_id].y = y;
    strcpy(vertices[row_id].lf_name, lf_name);
    
    if (prev_shape_row_id == shape_row_id)
    {
      add_vout(&vertices[row_id-1], &vertices[row_id]);
      add_vout(&vertices[row_id], &vertices[row_id-1]);
    }
    prev_shape_row_id = shape_row_id;
  }
  
  fprintf(stderr, "centerline fully indexed\n");
  
  int bad_distances = 0, double_bad_distances = 0;
  for (row_id = 0 ; row_id < addresses->num_rows ; row_id++)
  {
    double x = get_cell_as_double(addresses, row_id, x_column_id);
    double y = get_cell_as_double(addresses, row_id, y_column_id);
    double radius = get_cell_as_double(addresses, row_id, radius_column_id);
    if (radius < 0.0001) radius = 0.0001;
    
    char * lf_name = (char*)get_cell(addresses, row_id, lf_name_column_id);
    struct Vertex v = get_nearest_edge_vertex(kdtree, x, y, lf_name);
    
    double dx = x - v.x;
    double dy = y - v.y;
    
    double angle = atan2(dy, dx);
    double distance = sqrt(dx*dx + dy*dy);
    
    if (distance < 0.0001 || distance > 0.1)
    {
      bad_distances++;
      struct Vertex v = get_nearest_edge_vertex(kdtree, x, y, NULL); // try again without the street name
      
      dx = x - v.x;
      dy = y - v.y;
      
      angle = atan2(dy, dx);
      distance = sqrt(dx*dx + dy*dy);
      if (distance < 0.0001 || distance > 0.1)
      {
        double_bad_distances++;
      }
      /*fprintf(stderr, "row_id = %d\n", row_id);
      fprintf(stderr, "%s\n", (char*)get_cell(addresses, row_id, 3));
      fprintf(stderr, "lf_name = %s\n", lf_name);
      fprintf(stderr, "radius = %f\n", radius);
      fprintf(stderr, "distance = %f\n", distance);
      return EXIT_FAILURE;*/
    }
    
    //set_cell_from_int(edges, row_id*2+1, 0, row_id);
    //set_cell_from_int(edges, row_id*2+1, 1, 5); // GL_LINE_STRIP
    
    double road_width = 0.0001;
    
    if (lf_name != NULL)
    {
      if (strcmp(lf_name, "SPADINA AVE")==0) road_width *= 2;
      else if (strcmp(lf_name, "UNIVERSITY AVE")==0) road_width *= 2;
      else if (strcmp(lf_name, "ST CLAIR AVE")==0) road_width *= 2;
    }
    
    if (v.num_vout == 2)
    {
      //struct Vertex * v1 = v.vout[0];
      //struct Vertex * v2 = v.vout[1];
      set_cell_from_double(addresses, row_id, x_column_id, v.x);
      set_cell_from_double(addresses, row_id, y_column_id, v.y);
      //set_cell_from_double(addresses, row_id, street_left_x_column_id, x - cos(angle)*(distance-road_width)*scale + cos(angle-3.14159265/2.0)*(radius/2.0));
      //set_cell_from_double(addresses, row_id, street_left_y_column_id, y - sin(angle)*(distance-road_width)*scale + sin(angle-3.14159265/2.0)*(radius/2.0));
      //set_cell_from_double(addresses, row_id, street_right_x_column_id, x - cos(angle)*(distance-road_width)*scale - cos(angle-3.14159265/2.0)*(radius/2.0));
      //set_cell_from_double(addresses, row_id, street_right_y_column_id, y - sin(angle)*(distance-road_width)*scale - sin(angle-3.14159265/2.0)*(radius/2.0));
    }
    else
    {
      set_cell_from_double(addresses, row_id, x_column_id, v.x);
      set_cell_from_double(addresses, row_id, y_column_id, v.y);
      //set_cell_from_double(addresses, row_id, street_left_x_column_id, x);
      //set_cell_from_double(addresses, row_id, street_left_y_column_id, y);
      //set_cell_from_double(addresses, row_id, street_right_x_column_id, x);// - cos(angle)*distance*scale);
      //set_cell_from_double(addresses, row_id, street_right_y_column_id, y);// - sin(angle)*distance*scale);
    }
    
    if (row_id % 20000 == 0) fprintf(stderr, "%d/%d done (%.1f%%)\n", row_id, addresses->num_rows, 100.0 * row_id / (float)addresses->num_rows);
    //break;
  }
  if (bad_distances > 0) fprintf(stderr, "there were %d addresses where the distance just doesn't seem right.\n", bad_distances);
  if (double_bad_distances > 0) fprintf(stderr, "of those bad distance addresses, %d still failed the test after blanking the street name\n", double_bad_distances);
  
  write_block(stdout, addresses);
  free_block(addresses);
}

