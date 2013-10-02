
#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/block.h"

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  assert_stdin_is_piped();
  //assert_stdout_is_piped();
  
  static char addresses_table[1000] = ""; //addresses_july2013";
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"addresses_table", required_argument, 0, 'a'},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "a:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'a': strncpy(addresses_table, optarg, sizeof(addresses_table)); break;
      default: abort();
    }
  }
  
  if (strlen(addresses_table) == 0) { fprintf(stderr, "--addresses_table=addresses_july2013 or similar\n"); return EXIT_FAILURE; }
  
	MYSQL mysql;
	MYSQL_RES * res;
	MYSQL_ROW row;
	
	if ((mysql_init(&mysql) == NULL)) { fprintf(stderr, "ERROR: mysql_init() error\n"); return 0; }
	if (!mysql_real_connect(&mysql, "localhost", "root", "", "mb", 0, NULL, 0))
	{
		fprintf(stderr, "ERROR: mysql_real_connect error (%s)\n", mysql_error(&mysql));
	}

  struct Block * ticket_block = NULL;
  while ((ticket_block = read_block(stdin)))
  {
    int32_t is_opp_column_id             = get_column_id_by_name_or_exit(ticket_block, "is_opp");
    int32_t location2_column_id          = get_column_id_by_name_or_exit(ticket_block, "location2");

    char query[1000] = "";
		sprintf(query, "UPDATE %s SET num_opp_tickets = 0, num_at_tickets = 0", addresses_table);
    //fprintf(stderr, "%s\n", query);
		if (mysql_query(&mysql, query) != 0)
		{
			fprintf(stderr, "SQL ERROR: %s\n%s\n", query, mysql_error(&mysql));
			break;
		}

    int row_id = 0;
    for (row_id = 0 ; row_id < ticket_block->num_rows ; row_id++)
    {
    	int is_opp 						= get_cell_as_int32(ticket_block, row_id, is_opp_column_id);
    	char * full_address 	= (char*)get_cell(ticket_block, row_id, location2_column_id);
    	int i;
    	for (i = 0 ; i < strlen(full_address) ; i++)
    		if (full_address[i] == '"')
    			full_address[i] == '\'';
    	
    	if (is_opp)
    	  sprintf(query, "UPDATE %s SET num_opp_tickets = num_opp_tickets + 1 WHERE FULL_ADDRESS = \"%s\"", addresses_table, full_address);
		  else
		    sprintf(query, "UPDATE %s SET num_at_tickets = num_at_tickets + 1 WHERE FULL_ADDRESS = \"%s\"", addresses_table, full_address);
		  
	    //fprintf(stderr, "%s\n", query);
			if (row_id % 10000 == 0) fprintf(stderr, ".", query);
			if (mysql_query(&mysql, query) != 0)
			{
				fprintf(stderr, "SQL ERROR: %s\n%s\n", query, mysql_error(&mysql));
				//break;
			}
			//if (row_id > 10) break;
		}
  }
}


