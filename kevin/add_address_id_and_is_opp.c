
#include <math.h>
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
  
  FILE * addresses_fp = NULL;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"addresses", required_argument, 0, 'a'},
      //{"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "a:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'a': addresses_fp = fopen(optarg, "r"); break;
      default: abort();
    }
  }
  
  if (addresses_fp == NULL) { fprintf(stderr, "addresses file isn't provided or is invalid\n"); return EXIT_FAILURE; }
  
  struct Block * addresses = read_block(addresses_fp);
  void * addy_hash = create_hashtable_on_column(addresses, "FULL_ADDRESS");
  
  int tickets_with_bad_address = 0;
  
  struct Block * ticket_block = NULL;
  while ((ticket_block = read_block(stdin)))
  {
    ticket_block = add_command(ticket_block, argc, argv);
    
    ticket_block = add_int32_column_and_blank(ticket_block, "toronto_address_id");
    ticket_block = add_int32_column_and_blank(ticket_block, "is_opp");
    
    int32_t toronto_address_id_column_id = get_column_id_by_name_or_exit(ticket_block, "toronto_address_id");
    int32_t is_opp_column_id             = get_column_id_by_name_or_exit(ticket_block, "is_opp");
    int32_t location1_column_id          = get_column_id_by_name_or_exit(ticket_block, "location1");
    int32_t location2_column_id          = get_column_id_by_name_or_exit(ticket_block, "location2");
    
    int ticket_index;
    for (ticket_index = 0 ; ticket_index < ticket_block->num_rows ; ticket_index++)
    {
      char * ticket_location2 = (char*)get_cell(ticket_block, ticket_index, location2_column_id);
      int32_t address_row_id = search_hashtable_for_string(addy_hash, ticket_location2);
      if (address_row_id == -1) { tickets_with_bad_address++; continue; }
      
      int32_t is_opp = (strcmp((char*)get_cell(ticket_block, ticket_index, location1_column_id), "OP") == 0 || 
                        strcmp((char*)get_cell(ticket_block, ticket_index, location1_column_id), "OPP") == 0);
      
      set_cell_from_int32(ticket_block, ticket_index, toronto_address_id_column_id, address_row_id);
      set_cell_from_int32(ticket_block, ticket_index, is_opp_column_id, is_opp);
    }
    fprintf(stderr, "of %d tickets, %d have unknown addresses\n", ticket_block->num_rows, tickets_with_bad_address);
    
    write_block(stdout, ticket_block);
    free_block(ticket_block);
    
    /*
    //date_and_minute =   (date_of_infraction-20000000)*10000 + time_of_infraction
    block = add_int32_column_and_blank(block, "date_and_minute");
    //combined_cell   =   is_opp*1000000 + infraction_code*1000 + set_fine_amount
    block = add_int32_column_and_blank(block, "combined_cell");
    
    int32_t date_of_infraction_column_id = get_column_id_by_name_or_exit(block, "date_of_infraction");
    int32_t time_of_infraction_column_id = get_column_id_by_name_or_exit(block, "time_of_infraction");
    
    int32_t location1_column_id = get_column_id_by_name_or_exit(block, "location1");
    int32_t location2_column_id = get_column_id_by_name_or_exit(block, "location2");
    int32_t infraction_code_column_id = get_column_id_by_name_or_exit(block, "infraction_code");
    int32_t set_fine_amount_column_id = get_column_id_by_name_or_exit(block, "set_fine_amount");
    
    int32_t date_and_minute_column_id = get_column_id_by_name_or_exit(block, "date_and_minute");
    int32_t combined_cell_column_id = get_column_id_by_name_or_exit(block, "combined_cell");
    
    int row_id = 0;
    for (row_id = 0 ; row_id < block->num_rows ; row_id++)
    {
      int32_t date_of_infraction = get_cell_as_int32(block, row_id, date_of_infraction_column_id);
      int32_t time_of_infraction = get_cell_as_int32(block, row_id, time_of_infraction_column_id);
      
      int32_t date_and_minute = ((date_of_infraction-20000000)*10000) + time_of_infraction;
      set_cell_from_int32(block, row_id, date_and_minute_column_id, date_and_minute);
      
      int32_t infraction_code = get_cell_as_int32(block, row_id, infraction_code_column_id);
      int32_t set_fine_amount = get_cell_as_int32(block, row_id, set_fine_amount_column_id);
      char * location1 = (char*)get_cell(block, row_id, location1_column_id);
      char * location2 = (char*)get_cell(block, row_id, location2_column_id);
      
      int32_t combined_cell = infraction_code*1000 + set_fine_amount;
      if (strcmp(location1, "OPP")==0 || strcmp(location1, "OP")==0)
        combined_cell += 1000000;
      
      set_cell_from_int32(block, row_id, combined_cell_column_id, combined_cell);
    }
    write_block(stdout, block);
    free_block(block);
    */
  }
  
  free_hashtable(addy_hash);
}

