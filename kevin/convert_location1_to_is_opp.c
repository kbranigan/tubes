
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/block.h"

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  assert_stdin_is_piped();
  assert_stdout_is_piped();
  
  int tickets_which_are_opp = 0;

  struct Block * ticket_block = NULL;
  while ((ticket_block = read_block(stdin)))
  {
    ticket_block = add_command(ticket_block, argc, argv);
    
    if (get_column_id_by_name(ticket_block, "is_opp") == -1) {
      ticket_block = add_int32_column_and_blank(ticket_block, "is_opp");
    }
    
    int32_t is_opp_column_id             = get_column_id_by_name_or_exit(ticket_block, "is_opp");
    int32_t location1_column_id          = get_column_id_by_name_or_exit(ticket_block, "location1");
    
    int ticket_index;
    for (ticket_index = 0 ; ticket_index < ticket_block->num_rows ; ticket_index++)
    {
      int32_t is_opp = (strcmp((char*)get_cell(ticket_block, ticket_index, location1_column_id), "OP") == 0 || 
                        strcmp((char*)get_cell(ticket_block, ticket_index, location1_column_id), "OPP") == 0);
      
      if (is_opp) {
        tickets_which_are_opp++;
      }

      set_cell_from_int32(ticket_block, ticket_index, is_opp_column_id, is_opp);
    }
    fprintf(stderr, "of %d tickets, %d are issued opposite their given address\n", ticket_block->num_rows, tickets_which_are_opp);
    
    write_block(stdout, ticket_block);
    free_block(ticket_block);
  }
}

