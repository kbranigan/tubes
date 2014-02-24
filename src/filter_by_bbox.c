
#include "block.h"
#include <math.h>

enum OPERATOR {
  OPERATOR_DELETE = 1,
  OPERATOR_PASS = 2
};

char operator_names[3][40] = {
  "unknown", "delete", "pass"
};

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);

  assert_stdin_is_piped();
  assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();

  static char bbox_char[1000] = "";
  static int debug = 0;

  double bbox[2][2] = {0,0,0,0};

  struct Params * params = NULL;
  params = add_string_param(params, "bbox", 'b', bbox_char, 1);
  params = add_flag_param(params, "debug", 'd', &debug, 0);
  eval_params(params, argc, argv);

  char * p = bbox_char;
  char * bbox_ptr = bbox_char;
  p = strsep(&bbox_ptr, ",");
  if (p == NULL) { fprintf(stderr, "ERROR: Usage: %s --bbox=\"0,0,1,1\" (min_x,max_x,min_y,max_y)", argv[0]); return 0; }
  bbox[0][0] = atof(p);
  p = strsep(&bbox_ptr, ",");
  if (p == NULL) { fprintf(stderr, "ERROR: Usage: %s --bbox=\"0,0,1,1\" (min_x,max_x,min_y,max_y)", argv[0]); return 0; }
  bbox[0][1] = atof(p);
  p = strsep(&bbox_ptr, ",");
  if (p == NULL) { fprintf(stderr, "ERROR: Usage: %s --bbox=\"0,0,1,1\" (min_x,max_x,min_y,max_y)", argv[0]); return 0; }
  bbox[1][0] = atof(p);
  p = strsep(&bbox_ptr, ",");
  if (p == NULL) { fprintf(stderr, "ERROR: Usage: %s --bbox=\"0,0,1,1\" (min_x,max_x,min_y,max_y)", argv[0]); return 0; }
  bbox[1][1] = atof(p);

  int i, j;
  struct Block * block = NULL;
  while ((block = read_block(stdin))) {

    struct Block * newblock = new_block();
    newblock = copy_all_attributes(newblock, block);
    newblock = copy_all_columns(newblock, block);

    int x_cid = get_column_id_by_name_or_exit(block, "x");
    int y_cid = get_column_id_by_name_or_exit(block, "y");

    unsigned row_id = 0;
    for (row_id = 0 ; row_id < block->num_rows ; row_id++)
    {
      double x = get_cell_as_double(block, row_id, x_cid);
      double y = get_cell_as_double(block, row_id, y_cid);
      //fprintf(stderr, "%f,%f\n", x, y);
      //fprintf(stderr, "%f,%f,%f,%f\n", bbox[0][0], bbox[0][1], bbox[1][0], bbox[1][1]);
      if (x >= bbox[0][0] && x <= bbox[0][1] &&
          y >= bbox[1][0] && y <= bbox[1][1])
      {
        newblock = add_row(newblock);
        memcpy(get_row(newblock, newblock->num_rows-1), get_row(block, row_id), block->row_bsize);
      }
      //break;
    }
    //newblock->num_rows = new_num_rows;
    //newblock = set_num_rows(newblock, newblock->num_rows);

    //int newblock_row_id = 0;

    write_block(stdout, newblock);
    free_block(newblock);
    free_block(block);
  }
}
