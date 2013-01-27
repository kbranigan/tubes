
#include "block.h"
#include <time.h>
#include <math.h>

time_t now_temp = 0;
struct tm * timeinfo_temp;
int get_wday(int date)
{
  int year = floor(date / 10000);
  int month = floor((date - year*10000) / 100);
  int day = date - month*100 - year*10000;
  
  if (now_temp == 0)
  {
    time(&now_temp);
    timeinfo_temp = localtime(&now_temp);
  }
  
  timeinfo_temp->tm_year = year - 1900;
  timeinfo_temp->tm_mon = month - 1;
  timeinfo_temp->tm_mday = day;
  mktime(timeinfo_temp);
  
  return timeinfo_temp->tm_wday;
}

int main(int argc, char ** argv)
{

  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  assert_stdin_is_piped();
  assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
  static char column_name[1000] = "date_of_infraction";
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"column", required_argument, 0, 'c'},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "c:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'c': strncpy(column_name, optarg, sizeof(column_name)); break;
      default: abort();
    }
  }
  
  struct Block * block = NULL;
  while ((block = read_block(stdin)))
  {
		if (get_column_id_by_name(block, "wday")==-1)
			block = add_int32_column(block, "wday");
		
    int32_t date_column_id = get_column_id_by_name_or_exit(block, column_name);
		int32_t wday_column_id = get_column_id_by_name_or_exit(block, "wday");
		
		int row_id;
		for (row_id = 0 ; row_id < block->num_rows ; row_id++)
		{
	    int32_t date = get_cell_as_int32(block, row_id, date_column_id);
			set_cell_from_int32(block, row_id, wday_column_id, get_wday(date));
		}
		
    write_block(stdout, block);
    free_block(block);
  }
}
