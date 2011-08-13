
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "shapefile_src/shapefil.h"

#define SCHEME_CREATE_MAIN
#define SCHEME_ASSERT_STDOUT_IS_PIPED
#define SCHEME_FUNCTION join_attributes_from_dbf
#include "scheme.h"

int join_attributes_from_dbf(int argc, char ** argv, FILE * pipe_in, FILE * pipe_out, FILE * pipe_err)
{
  char filename[300] = "";
  char left_join_field[50] = "";
  char right_join_field[50] = "";
  char join_type[50] = "left";
  int c;
  while ((c = getopt(argc, argv, "f:l:r:j:")) != -1)
  switch (c)
  {
    case 'f':
      strncpy(filename, optarg, sizeof(filename));
      break;
    case 'l':
      strncpy(left_join_field, optarg, sizeof(left_join_field));
      break;
    case 'r':
      strncpy(right_join_field, optarg, sizeof(right_join_field));
      break;
    case 'j':
      strncpy(join_type, optarg, sizeof(join_type));
      break;
    default:
      abort();
  }
  
  if (strlen(filename) == 0)
  {
    fprintf(stderr, "%s: must specify a shapefile using -f [filename.dbf]\n", argv[0]);
    exit(1);
  }
  
  FILE * fp = fopen(filename, "r");
  if (!fp)
  {
    fprintf(stderr, "%s: error reading shapefile: %s\n", argv[0], filename);
    exit(1);
  }
  
  DBFHandle d = DBFOpen(filename, "rb");
  if (d == NULL) { fprintf(stderr, "DBFOpen error (%s.dbf)\n", filename); exit(1); }
	
  long nRecordCount = DBFGetRecordCount(d);
  long nFieldCount = DBFGetFieldCount(d);
  
  long t=0;
  long i;
  
  int num_strings = 0;
  char ** strings = malloc(sizeof(char*)*num_strings);
  for (i = 0 ; i < nFieldCount ; i++)
  {
    strings[i] = malloc(20);
    int value_length;
    DBFFieldType field_type = DBFGetFieldInfo(d, i, strings[i], &value_length, NULL);
  }
  write_string_table(pipe_out, num_strings, strings);
  for (i = 0 ; i < nFieldCount ; i++)
    free(strings[i]);
  free(strings);
  
  for (i = 0 ; i < nRecordCount ; i++)
  {
  }
  DBFClose(d);
}
