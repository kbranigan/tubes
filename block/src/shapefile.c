
#include <stdio.h>
#include <stdlib.h>

#include "../ext/shapefil.h"

#include "block.h"

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  //assert_stdin_is_piped();
  assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
  static long specific_row_id = -1;
  static long specific_part_id = -1;
  static char filename[1000] = "";
  
  int number_of_rows = 0;
  
  static int debug = 0;
  
  char * allcolumns = NULL;
  int num_specific_columns = 0;
  char ** specific_columns = NULL;
  
  int include_z = 0;
  int include_m = 0;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"row_id", required_argument, 0, 'r'},
      {"part_id", required_argument, 0, 'p'},
      {"filename", required_argument, 0, 'f'},
      {"columns", required_argument, 0, 'c'},
      {"number_of_rows", required_argument, 0, 'n'},
      {"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "r:p:f:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'r': specific_row_id = atoi(optarg); break;
      case 'p': specific_part_id = atoi(optarg); break;
      case 'f': strncpy(filename, optarg, sizeof(filename)); break;
      case 'c': { allcolumns = malloc(strlen(optarg)+1); strncpy(allcolumns, optarg, strlen(optarg)); break; }
      case 'n': number_of_rows = atoi(optarg); break;
      default: abort();
    }
  }
  
  if (filename[0] == 0 && argc == 2 && argv[1] != NULL)
    strncpy(filename, argv[1], sizeof(filename));
  
  if (filename[0] == 0)
  {
    fprintf(stderr, "ERROR: Usage: %s -f [file_name]\n", argv[0]);
    return -1;
  }
  
  FILE * fp = filename[0] == 0 ? stdin : fopen(filename, "r");
  
  if (fp == NULL)
  {
    fprintf(stderr, "ERROR: file '%s' couldn't be opened for reading.\n", filename);
  }
  
  DBFHandle dbf = DBFOpen(filename, "rb");
  SHPHandle shp = SHPOpen(filename, "rb");
  
  if (dbf == NULL && shp == NULL)
  {
    fprintf(stderr, "%s: Error, neither dbf or shp file found\n", argv[0]);
    return EXIT_FAILURE;
  }
  
  long nRecordCount = dbf ? DBFGetRecordCount(dbf) : 0;
  long nFieldCount = dbf ? DBFGetFieldCount(dbf) : 0;
  
  struct Block * block = new_block();
  block = add_command(block, argc, argv);
  
  if (allcolumns)
  {
    block = add_string_attribute(block, "specific columns", allcolumns);
    char * ptr = strtok(allcolumns, ",");
    while (ptr != NULL)
    {
      num_specific_columns++;
      specific_columns = realloc(specific_columns, sizeof(char*)*num_specific_columns);
      specific_columns[num_specific_columns-1] = malloc(strlen(ptr)+1);
      strncpy(specific_columns[num_specific_columns-1], ptr, strlen(ptr));
      specific_columns[num_specific_columns-1][strlen(ptr)] = 0;
      ptr = strtok(NULL, ",");
    }
    free(allcolumns);
  }
  
  double min_x = 180, max_x = -180;
  double min_y = 90, max_y = -90;
  
  block = add_double_attribute(block, "min_x", min_x);
  block = add_double_attribute(block, "max_x", max_x);
  block = add_double_attribute(block, "min_y", min_y);
  block = add_double_attribute(block, "max_y", max_y);
  
  block = add_int_column(block, "shape_row_id");
  block = add_int_column(block, "shape_part_id");
  block = add_int_column(block, "shape_part_type");
  block = add_double_column(block, "x");
  block = add_double_column(block, "y");
  if (include_z) block = add_double_column(block, "z");
  if (include_m) block = add_double_column(block, "m");
  
  long column_id;
  for (column_id = 0 ; column_id < nFieldCount ; column_id++)
  {
    char name[20];
    int value_length;
    DBFFieldType field_type = DBFGetFieldInfo(dbf, column_id, name, &value_length, NULL);
    
    int32_t max_string_length = 0;
    if (field_type == FTString)
    {
      int shape_row_id = 0;
      for (shape_row_id = 0 ; shape_row_id < nRecordCount ; shape_row_id++)
      {
        char * value = (char*)DBFReadStringAttribute(dbf, shape_row_id, column_id);
        if (strlen(value) > max_string_length) max_string_length = strlen(value);
      }
    }
    
    int found = 0;
    if (num_specific_columns > 0)
    {
      int specific_column_id;
      for (specific_column_id = 0 ; specific_column_id < num_specific_columns ; specific_column_id++)
        if (strcmp(specific_columns[specific_column_id], name)==0) found = 1;
      if (found == 0) continue;
    }
    
    if (field_type == FTString)      block = add_string_column_with_length(block, name, max_string_length);
    else if (field_type = FTInteger) block = add_int_column(block, name);
    else if (field_type = FTDouble)  block = add_double_column(block, name);
    else { fprintf(stderr, "%s: unknown field type (%d) encountered\n", argv[0], field_type); abort(); }
  }
  
  int shape_row_id = 0;
  int num_rows = 0;
  for (shape_row_id = 0 ; shape_row_id < nRecordCount ; shape_row_id++)
  {
    SHPObject * shape = SHPReadObject(shp, shape_row_id);
    num_rows += shape->nVertices;
    SHPDestroyObject(shape);
  }
  
  if (number_of_rows != 0) num_rows = number_of_rows;
  
  block = set_num_rows(block, num_rows);
  
  //block = column_string_set_length(block, find_column_id_by_name(block, "ADDRESS"), 10);
  
  int row_id = 0;
  for (shape_row_id = 0 ; shape_row_id < nRecordCount ; shape_row_id++)
  {
    if (specific_row_id != -1 && shape_row_id != specific_row_id) continue;
    
    SHPObject * shape = SHPReadObject(shp, shape_row_id);
    
    if (debug) fprintf(stderr, "shape->nParts = %d\n", shape->nParts);
    int part_id = 0;
    for (part_id = 0 ; part_id < shape->nParts || (shape->nParts==0 && part_id==0) ; part_id++)
    {
      int part_start = shape->nParts==0 ? 0 : shape->panPartStart[part_id];
      int part_end = (shape->nParts==0 || part_id+1 == shape->nParts) ? shape->nVertices : shape->panPartStart[part_id+1];
      int part_type = shape->nParts==0 ? SHPT_POINT : shape->panPartType[part_id];
      
      if (debug) fprintf(stderr, "  part(%d) = %d to %d\n", part_id, part_start, part_end);
      int point_id = 0;
      for (point_id = part_start ; point_id < part_end ; point_id++)
      {
        if (specific_part_id != -1 && part_id != specific_part_id) continue;
        
        int field_id = 0;
        
        set_cell(block, row_id, field_id++, &shape_row_id);
        set_cell(block, row_id, field_id++, &part_id);
        set_cell(block, row_id, field_id++, &part_type);
        
        double x = shape->padfX[point_id]; set_cell(block, row_id, field_id++, &x);
        double y = shape->padfY[point_id]; set_cell(block, row_id, field_id++, &y);
        if (include_z) { double z = shape->padfZ[point_id]; set_cell(block, row_id, field_id++, &z); }
        if (include_m) { double m = shape->padfM[point_id]; set_cell(block, row_id, field_id++, &m); }
        
        if (x > max_x) max_x = x; if (x < min_x) min_x = x;
        if (y > max_y) max_y = y; if (y < min_y) min_y = y;
        
        for (column_id = 0 ; column_id < nFieldCount ; column_id++)
        {
          char name[20];
          char value[1000];
          int value_length;
          DBFFieldType field_type = DBFGetFieldInfo(dbf, column_id, name, &value_length, NULL);
          
          int found = 0;
          if (num_specific_columns > 0)
          {
            int specific_column_id;
            for (specific_column_id = 0 ; specific_column_id < num_specific_columns ; specific_column_id++)
              if (strcmp(specific_columns[specific_column_id], name)==0)
                found = 1;
            if (found == 0)
              continue;
          }
          
          switch (field_type) {
            case FTString: {
              void * value = (void*)DBFReadStringAttribute(dbf, shape_row_id, column_id);
              set_cell(block, row_id, field_id++, value);
              break;
            }
            case FTInteger: {
              int32_t value = DBFReadIntegerAttribute(dbf, shape_row_id, column_id);
              set_cell(block, row_id, field_id++, &value);
              break;
            }
            case FTDouble: {
              double value = DBFReadDoubleAttribute(dbf, shape_row_id, column_id);
              set_cell(block, row_id, field_id++, &value);
              break;
            }
            default: {
              fprintf(stderr, "unknown shapefile field type\n");
              abort();
            }
          }
        }
        row_id++;
        if (number_of_rows != 0 && row_id >= number_of_rows) break;
      }
      if (number_of_rows != 0 && row_id >= number_of_rows) break;
    }
    SHPDestroyObject(shape);
    if (number_of_rows != 0 && row_id >= number_of_rows) break;
  }
  
  memcpy((void*)attribute_get_value(get_attribute_by_name(block, "min_x")), &min_x, sizeof(double));
  memcpy((void*)attribute_get_value(get_attribute_by_name(block, "max_x")), &max_x, sizeof(double));
  memcpy((void*)attribute_get_value(get_attribute_by_name(block, "min_y")), &min_y, sizeof(double));
  memcpy((void*)attribute_get_value(get_attribute_by_name(block, "max_y")), &max_y, sizeof(double));
  
  write_block(stdout, block);
  fprintf(stderr, "%s: Finished.  %d rows, %d columns read from '%s', enjoy!\n", argv[0], block->num_rows, block->num_columns, filename);
  free_block(block);
  
  if (dbf) DBFClose(dbf);
  if (shp) SHPClose(shp);
  
  
  if (num_specific_columns > 0)
  {
    while (num_specific_columns > 0)
    {
      free(specific_columns[num_specific_columns-1]);
      num_specific_columns--;
    }
    free(specific_columns);
  }
}
