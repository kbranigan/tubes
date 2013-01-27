
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/block.h"
#include "../src/block_hashtable.h"

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

int is_dumbass_infraction(int32_t infraction_code)
{
  if (infraction_code == 1 || 
      infraction_code == 207 || 
      infraction_code == 210 || 
      infraction_code == 312)
    return 1;
  else
    return 0;
}

int is_ignored_infraction(int32_t infraction_code)
{
  if (infraction_code == 5 || 
      infraction_code == 9 || 
      infraction_code == 264)
    return 1;
  else
    return 0;
}

int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  //assert_stdin_is_piped();
  assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
  //FILE * tickets_fp = NULL;
  FILE * addresses_fp = NULL;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      //{"tickets", required_argument, 0, 'c'},
      {"addresses", required_argument, 0, 'a'},
      //{"debug", no_argument, &debug, 1},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "t:a:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      //case 't': tickets_fp = fopen(optarg, "r"); break;
      case 'a': addresses_fp = fopen(optarg, "r"); break;
      default: abort();
    }
  }
  
  //if (tickets_fp == NULL) { fprintf(stderr, "tickets isn't provided or is invalid\n"); return EXIT_FAILURE; }
  if (addresses_fp == NULL) { fprintf(stderr, "addresses isn't provided or is invalid\n"); return EXIT_FAILURE; }
  
  //struct Block * centerline = read_block(centerline_fp); fclose(centerline_fp);
  struct Block * addresses = read_block(addresses_fp); fclose(addresses_fp);
  
  //char * addresses_filename = "data/toronto.addresses.with.full_address.smaller.block";
  //char * addresses_filename = "toronto_addresses_and_tickets_join.source.data.block";
  
  if (get_column_id_by_name(addresses, "num_all_tickets") == -1)
  {
    fprintf(stderr, "you need to add a whole bunch of columns to addresses file for this to work.\n");
    
    addresses = add_int_column(addresses, "num_all_tickets"); blank_column_values(addresses, "num_all_tickets");
    addresses = add_int_column(addresses, "total_fined"); blank_column_values(addresses, "total_fined");
    
    addresses = add_int_column(addresses, "num_dumbass_tickets"); blank_column_values(addresses, "num_dumbass_tickets");
    addresses = add_int_column(addresses, "num_ignored_tickets"); blank_column_values(addresses, "num_ignored_tickets");
    
    addresses = add_int_attribute(addresses, "num_ticket_years", 0);
    addresses = add_int_attribute(addresses, "num_tickets_total", 0);
    
    int wday;
    char addresses_wday_column_name[7][100];
    for (wday = 0 ; wday < 7 ; wday++)
    {
      sprintf(addresses_wday_column_name[wday], "wday_%d", wday);
      addresses = add_int_column(addresses, addresses_wday_column_name[wday]);
      blank_column_values(addresses, addresses_wday_column_name[wday]);
    }
    
    /*int hhly;
    for (wday = 0 ; wday < 7 ; wday++)
    {
      for (hhly = 0 ; hhly < 24 ; hhly++) // kbfu - 24 = half hour
      {
        char temp[100];
        sprintf(temp, "wday_%d_hh_%d", wday, hhly);
        //addresses = add_long_column(addresses, temp); // kbfu
        //blank_column_values(addresses, temp);
      }
    }*/
    
    fprintf(stderr, "fields were added to addresses block, and thats what you get. Give me that file back (as the new addresses file) and I'll continue.\n");
    write_block(stdout, addresses);
    free_block(addresses);
    return EXIT_FAILURE;
  }
  
  void * addy_hash = create_hashtable_on_column(addresses, "FULL_ADDRESS");
  void * name_hash = create_hashtable_on_column(addresses, "NAME");
  
  int32_t addresses_num_all_tickets_column_id     = get_column_id_by_name(addresses, "num_all_tickets");
  int32_t addresses_num_dumbass_tickets_column_id = get_column_id_by_name(addresses, "num_dumbass_tickets");
  int32_t addresses_num_ignored_tickets_column_id = get_column_id_by_name(addresses, "num_ignored_tickets");
  int32_t addresses_total_fined_column_id         = get_column_id_by_name(addresses, "total_fined");
  
  int wday;
  int32_t addresses_wday_column_id[7];
  char addresses_wday_column_name[7][20];
  for (wday = 0 ; wday < 7 ; wday++)
  {
    sprintf(addresses_wday_column_name[wday], "wday_%d", wday);
    addresses_wday_column_id[wday] = get_column_id_by_name(addresses, addresses_wday_column_name[wday]);
  }
  
  int32_t addresses_wday_hhly_column_id_start = get_column_id_by_name(addresses, "wday_0_hh_0");
  
  int32_t num_tickets_total = 0;
  
  int year = 2008;
  do
  {
    char temp[100];
    sprintf(temp, "data/tickets.%d.block", year);
    FILE * fp = fopen(temp, "r");
    if (fp == NULL) break;
    fprintf(stderr, "loading: %s\n", temp);
    struct Block * ticket_block = read_block(fp);
    fclose(fp);
    
    //int32_t tag_number_masked_column_id  = get_column_id_by_name(ticket_block, "tag_number_masked");
    int32_t location1_column_id          = get_column_id_by_name(ticket_block, "location1");
    int32_t location2_column_id          = get_column_id_by_name(ticket_block, "location2");
    int32_t date_of_infraction_column_id = get_column_id_by_name(ticket_block, "date_of_infraction");
    int32_t infraction_code_column_id    = get_column_id_by_name(ticket_block, "infraction_code");
    int32_t set_fine_amount_column_id    = get_column_id_by_name(ticket_block, "set_fine_amount");
    int32_t time_of_infraction_column_id = get_column_id_by_name(ticket_block, "time_of_infraction");
    
    int tickets_with_no_location2 = 0;
    int tickets_with_bad_location2 = 0;
    
    int ticket_index;
    for (ticket_index = 0 ; ticket_index < ticket_block->num_rows ; ticket_index++)
    {
      //char * ticket_tag_number_masked   = (char*)get_cell(ticket_block, ticket_index, tag_number_masked_column_id);
      char *  ticket_location1          = (char*)get_cell(ticket_block, ticket_index, location1_column_id);
      char *  ticket_location2          = (char*)get_cell(ticket_block, ticket_index, location2_column_id);
      int32_t ticket_date_of_infraction = *(int32_t*)get_cell(ticket_block, ticket_index, date_of_infraction_column_id);
      int32_t ticket_infraction_code    = *(int32_t*)get_cell(ticket_block, ticket_index, infraction_code_column_id);
      int32_t ticket_set_fine_amount    = *(int32_t*)get_cell(ticket_block, ticket_index, set_fine_amount_column_id);
      int32_t ticket_time_of_infraction = *(int32_t*)get_cell(ticket_block, ticket_index, time_of_infraction_column_id);
      
      if (strlen(ticket_location2) == 0) { tickets_with_no_location2++; continue; }
      //if (strcmp(ticket_location2, "5 CAMDEN ST")!=0) continue; //kbfu
      int32_t address_row_id = search_hashtable_for_string(addy_hash, ticket_location2);
      if (address_row_id == -1) address_row_id = search_hashtable_for_string(name_hash, ticket_location2);
      if (address_row_id == -1) { tickets_with_bad_location2++; continue; }
      
      int32_t * addresses_num_all_tickets     = (int32_t*)get_cell(addresses, address_row_id, addresses_num_all_tickets_column_id);
      int32_t * addresses_num_dumbass_tickets = (int32_t*)get_cell(addresses, address_row_id, addresses_num_dumbass_tickets_column_id);
      int32_t * addresses_num_ignored_tickets = (int32_t*)get_cell(addresses, address_row_id, addresses_num_ignored_tickets_column_id);
      int32_t * addresses_total_fined         = (int32_t*)get_cell(addresses, address_row_id, addresses_total_fined_column_id);
      
      int32_t * addresses_wday[7];
      for (wday = 0 ; wday < 7 ; wday++)
        addresses_wday[wday] = (int32_t*)get_cell(addresses, address_row_id, addresses_wday_column_id[wday]);
      
      (*addresses_total_fined) += ticket_set_fine_amount;
      
      (*addresses_num_all_tickets)++;
      if (is_dumbass_infraction(ticket_infraction_code)) (*addresses_num_dumbass_tickets)++;
      if (is_ignored_infraction(ticket_infraction_code)) (*addresses_num_ignored_tickets)++;
      
      wday = get_wday(ticket_date_of_infraction);
      (*addresses_wday[wday])++;
      
      if (ticket_time_of_infraction >= 0 && ticket_time_of_infraction <= 2400)
      {
        int32_t hour = floor(ticket_time_of_infraction / 100);
        int32_t min = ticket_time_of_infraction - hour * 100;
        int32_t min_in_day = hour*60 + min;
        //int32_t hhly = floor(min_in_day / (1440.0/(float)DAY_DIVISION));
        //if (hhly < 0 || hhly > DAY_DIVISION) { fprintf(stderr, "bad hhly\n"); exit(0); }
        //int32_t * hhly_cell = (int32_t*)get_cell(addresses, address_row_id, addresses_wday_hhly_column_id_start + hhly + wday*DAY_DIVISION);
        //(*hhly_cell)++;
      }
      else { fprintf(stderr, "error with time_of_infraction %d\n", ticket_time_of_infraction); exit(0); }
      
      num_tickets_total++;
      if (num_tickets_total%500000==0) fprintf(stderr, "num_tickets_total = %d\n", num_tickets_total);
    }
    fprintf(stderr, "tickets in year %d with no location2: %d\n", year, tickets_with_no_location2);
    fprintf(stderr, "tickets in year %d with bad location2: %d\n", year, tickets_with_bad_location2);
    
    free_block(ticket_block);
    
    year++;
    break; // kbfu, only 2008 for you
  }
  while (1);

  fprintf(stderr, "num_tickets_total = %d\n", num_tickets_total);
  
  struct Attribute * num_tickets_total_attribute = get_attribute_by_name(addresses, "num_tickets_total");
  attribute_set_value(num_tickets_total_attribute, &num_tickets_total);
  //struct Attribute * num_ticket_years_attribute = get_attribute_by_name(addresses, "num_ticket_years");
  //attribute_set_value(num_ticket_years_attribute, &num_ticket_years);
  
  free_hashtable(addy_hash);
  free_hashtable(name_hash);
  
  write_block(stdout, addresses);
  free_block(addresses);
}
