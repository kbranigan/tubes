
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/block.h"
#include "../src/block_hashtable.h"

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  assert_stdin_is_piped();
  assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
  static int wday = 0;
  static int debug = 0;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"wday", required_argument, 0, 'w'},
      {"value", required_argument, 0, 'v'},
      {"operator", required_argument, 0, 'o'},
      //{"add", no_argument, &output_header, 1},
      {"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "w:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'w': wday = atoi(optarg); break;
      default: abort();
    }
  }
  
  struct Block * addresses = read_block(stdin);
  int32_t num_all_tickets_column_id = get_column_id_by_name(addresses, "num_all_tickets");
  assert(num_all_tickets_column_id >= 0);
  //fprintf(stderr, "num_all_tickets_column_id = %d\n", num_all_tickets_column_id);
  
  addresses = add_float_column(addresses, "red");   blank_column_values(addresses, "red");
  addresses = add_float_column(addresses, "green"); blank_column_values(addresses, "green");
  addresses = add_float_column(addresses, "blue");  blank_column_values(addresses, "blue");
  addresses = add_float_column(addresses, "alpha"); blank_column_values(addresses, "alpha");
  
  int32_t wday_0_column_id = get_column_id_by_name(addresses, "wday_0"); //assumes wday_0 to wday_6 are all adjacent
  int32_t wday_0_hh_0_column_id = get_column_id_by_name(addresses, "wday_0_hh_0"); //assumes wday_0_hh_0 to wday_6_hh_2 are all adjacent
  
  //wday_0_hh_0
  
  struct Attribute * attr = get_attribute_by_name(addresses, "num_tickets_total");
  int32_t num_tickets_total = *(int32_t*)attribute_get_value(attr);
  float average_num_tickets = num_tickets_total / (float)addresses->num_rows;
  
  int row_id;
  for (row_id = 0 ; row_id < addresses->num_rows ; row_id++)
  {
    float num_all_tickets = get_cell_as_float(addresses, row_id, num_all_tickets_column_id);
    float num_wday_tickets = get_cell_as_float(addresses, row_id, wday_0_column_id + wday);
    set_rgba(
      addresses,
      row_id,
      0.0, 0.0, 0.0,
      0.8 * (num_wday_tickets / average_num_tickets) + 0.2
    );
    //fprintf(stderr, "%f / %f\n", num_wday_tickets, num_all_tickets);
    //if (row_id > 10) break;
  }
  
  write_block(stdout, addresses);
  free_block(addresses);
}
