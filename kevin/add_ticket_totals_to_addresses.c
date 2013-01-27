
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "../src/block.h"
//#include "../src/block_hashtable.h"

unsigned int get_msec(void)
{
  static struct timeval timeval, first_timeval;
  gettimeofday(&timeval, 0);
  if(first_timeval.tv_sec == 0) { first_timeval = timeval; return 0; }
  return (timeval.tv_sec - first_timeval.tv_sec) * 1000 + (timeval.tv_usec - first_timeval.tv_usec) / 1000;
}

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

int is_meter_infraction(int32_t infraction_code)
{
  if (0
      || infraction_code == 1   // PARK FAIL TO DEPOSIT FEE METER
      || infraction_code == 207 // PARK FAIL TO DEP. FEE MACHINE
      || infraction_code == 210 // PARK FAIL TO DISPLAY RECEIPT
      || infraction_code == 312 // PARKING MACH-NOT USED/NO FEE  
      )
    return 1;
  else
    return 0;
}

int should_ignore_infraction(int32_t infraction_code)
{
  if (is_meter_infraction(infraction_code)) return 0;
  
	return 0;
	
  if (0
       || infraction_code == 3   // PARK/LEAVE ON PRIVATE PROPERTY
       || infraction_code == 5   // PARK-HWY PROHIB TIME/DAYS
       || infraction_code == 8   // STAND VEH HWY PRO TIMES/DAYS
       || infraction_code == 9   // STOP HWY PROHIBITED TIME/DAY
    // || infraction_code == 14  // PARK OBSTRUCT DRIVEWAY/LANEWAY
    // || infraction_code == 15  // PARK 3 M OF FIRE HYDRANT
    // || infraction_code == 26  // PARK - ON BOULEVARD
       || infraction_code == 29  // PARK PROHIBITED TIME NO PERMIT
    // || infraction_code == 30  // STOP ON SIDEWALK
    // || infraction_code == 48  // FAIL TO PARK/STOP PAR TO CURB
       || infraction_code == 264 // STAND VEH - PROHIBITED TIME
    // || infraction_code == 337 // PARK - ON BOULEVARD
     )
    return 0;
  
  return 1; // should ignore
}

int add_usleep = 0;

int main(int argc, char ** argv)
{
  unsigned int start = get_msec();
  
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  assert_stdin_is_piped();
  assert_stdout_is_piped();
  //assert_stdin_or_out_is_piped();
  
	static char ticket_filenames_all[1000] = "";
  //FILE * tickets_fp = NULL;
  //FILE * addresses_fp = NULL;
  
  static time_t now_t = 0;
  
  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"tickets", required_argument, 0, 't'},
      //{"addresses", required_argument, 0, 'a'},
      //{"debug", no_argument, &debug, 1},
      {"sleep", no_argument, &add_usleep, 1},
      {"now", required_argument, 0, 'n'},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "n:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 't': strncpy(ticket_filenames_all, optarg, sizeof(ticket_filenames_all)); break; //tickets_fp = fopen(optarg, "r"); break;
      //case 'a': addresses_fp = fopen(optarg, "r"); break;
      case 'n': now_t = atol(optarg); break;
      default: abort();
    }
  }
  
  char ticket_filenames_copy[1000] = "";
  strncpy(ticket_filenames_copy, ticket_filenames_copy, sizeof(ticket_filenames_copy));
  
  int num_ticket_filenames = 0;
  char ** ticket_filenames = NULL;
  char * pch = strtok(ticket_filenames_all, ",");
  while (pch != NULL)
  {
    num_ticket_filenames++;
    ticket_filenames = realloc(ticket_filenames, sizeof(char*)*num_ticket_filenames);
    ticket_filenames[num_ticket_filenames-1] = pch;
    pch = strtok(NULL, ",");
  }
  
  struct Block * addresses = read_block(stdin);
  
  if (get_column_id_by_name(addresses, "num_AT_tickets") == -1)
  {
    //addresses = add_int32_column_and_blank(addresses, "num_all_tickets");
    addresses = add_int32_column_and_blank(addresses, "num_AT_tickets");
    addresses = add_int32_column_and_blank(addresses, "num_AT_tickets_within_1_hour");
    addresses = add_int32_column_and_blank(addresses, "num_AT_tickets_within_3_hours");
    addresses = add_int32_column_and_blank(addresses, "num_AT_tickets_within_6_hours");
    
    addresses = add_int32_column_and_blank(addresses, "num_AT_meter_tickets");
    addresses = add_int32_column_and_blank(addresses, "num_AT_meter_tickets_within_1_hour");
    addresses = add_int32_column_and_blank(addresses, "num_AT_meter_tickets_within_3_hours");
    addresses = add_int32_column_and_blank(addresses, "num_AT_meter_tickets_within_6_hours");
    
    addresses = add_int32_column_and_blank(addresses, "num_OP_tickets");
    addresses = add_int32_column_and_blank(addresses, "num_OP_tickets_within_1_hour");
    addresses = add_int32_column_and_blank(addresses, "num_OP_tickets_within_3_hours");
    addresses = add_int32_column_and_blank(addresses, "num_OP_tickets_within_6_hours");
    
    addresses = add_int32_column_and_blank(addresses, "num_OP_meter_tickets");
    addresses = add_int32_column_and_blank(addresses, "num_OP_meter_tickets_within_1_hour");
    addresses = add_int32_column_and_blank(addresses, "num_OP_meter_tickets_within_3_hours");
    addresses = add_int32_column_and_blank(addresses, "num_OP_meter_tickets_within_6_hours");
    
    addresses = add_int64_attribute(addresses, "tickets_vs_now", 0);
    addresses = add_int32_attribute(addresses, "num_ticket_years", 0);
    addresses = add_int32_attribute(addresses, "num_tickets_total", 0);
    
    fprintf(stderr, "fields were added to addresses block.\n");
  }
  
  int32_t addresses_num_AT_tickets_column_id                       = get_column_id_by_name_or_exit(addresses, "num_AT_tickets");
  int32_t addresses_num_AT_tickets_within_1_hour_column_id         = get_column_id_by_name_or_exit(addresses, "num_AT_tickets_within_1_hour");
  int32_t addresses_num_AT_tickets_within_3_hour_column_id         = get_column_id_by_name_or_exit(addresses, "num_AT_tickets_within_3_hours");
  int32_t addresses_num_AT_tickets_within_6_hour_column_id         = get_column_id_by_name_or_exit(addresses, "num_AT_tickets_within_6_hours");
  int32_t addresses_num_AT_meter_tickets_column_id                 = get_column_id_by_name_or_exit(addresses, "num_AT_meter_tickets");
  int32_t addresses_num_AT_meter_tickets_within_1_hour_column_id   = get_column_id_by_name_or_exit(addresses, "num_AT_meter_tickets_within_1_hour");
  int32_t addresses_num_AT_meter_tickets_within_3_hour_column_id   = get_column_id_by_name_or_exit(addresses, "num_AT_meter_tickets_within_3_hours");
  int32_t addresses_num_AT_meter_tickets_within_6_hour_column_id   = get_column_id_by_name_or_exit(addresses, "num_AT_meter_tickets_within_6_hours");
  
  int32_t addresses_num_OP_tickets_column_id                       = get_column_id_by_name_or_exit(addresses, "num_OP_tickets");
  int32_t addresses_num_OP_tickets_within_1_hour_column_id         = get_column_id_by_name_or_exit(addresses, "num_OP_tickets_within_1_hour");
  int32_t addresses_num_OP_tickets_within_3_hour_column_id         = get_column_id_by_name_or_exit(addresses, "num_OP_tickets_within_3_hours");
  int32_t addresses_num_OP_tickets_within_6_hour_column_id         = get_column_id_by_name_or_exit(addresses, "num_OP_tickets_within_6_hours");
  int32_t addresses_num_OP_meter_tickets_column_id                 = get_column_id_by_name_or_exit(addresses, "num_OP_meter_tickets");
  int32_t addresses_num_OP_meter_tickets_within_1_hour_column_id   = get_column_id_by_name_or_exit(addresses, "num_OP_meter_tickets_within_1_hour");
  int32_t addresses_num_OP_meter_tickets_within_3_hour_column_id   = get_column_id_by_name_or_exit(addresses, "num_OP_meter_tickets_within_3_hours");
  int32_t addresses_num_OP_meter_tickets_within_6_hour_column_id   = get_column_id_by_name_or_exit(addresses, "num_OP_meter_tickets_within_6_hours");
  
  int now_wday;
  int now_in_minutes;
  {
    if (now_t == 0) now_t = time(NULL);
    
    struct tm * now = localtime(&now_t);
    now_wday = now->tm_wday;
    now_in_minutes = now->tm_hour*60.0 + now->tm_min;
    //now_in_minutes = (int)(floor(now_in_minutes / 30.0) * 30.0);
    
    fprintf(stderr, "%02d:%02d (%d) (%ld)\n", now->tm_hour, now->tm_min, now_in_minutes, now_t);
  }
  
  //now_in_minutes = 800; // 22:00 // KBFU
  //fprintf(stderr, "now_in_minutes set to 0 // KBFU\n");
  
  /*int wday;
  int32_t addresses_wday_column_id[7];
  char addresses_wday_column_name[7][20];
  for (wday = 0 ; wday < 7 ; wday++)
  {
    sprintf(addresses_wday_column_name[wday], "wday_%d", wday);
    addresses_wday_column_id[wday] = get_column_id_by_name(addresses, addresses_wday_column_name[wday]);
  }*/
  
  //int32_t addresses_wday_hhly_column_id_start = get_column_id_by_name(addresses, "wday_0_hh_0");
  
	int32_t num_tickets_total = 0;
  //int32_t num_ticket_years = 0;
  //int year = 2008;
	
	int ticket_filename_index = 0;
	for (ticket_filename_index = 0 ; ticket_filename_index < num_ticket_filenames ; ticket_filename_index++)
  {
    //char temp[100];
    //sprintf(temp, "data/tickets.%d.block", year);
		
		char * ticket_filename = ticket_filenames[ticket_filename_index];
		
    FILE * fp = fopen(ticket_filename, "r");
    if (fp == NULL) break;
    fprintf(stderr, "loading: %s\n", ticket_filename);
    struct Block * ticket_block = read_block(fp);
    fclose(fp);
    
    int32_t ticket_date_of_infraction_column_id    = get_column_id_by_name_or_exit(ticket_block, "date_of_infraction");
    int32_t ticket_infraction_code_column_id       = get_column_id_by_name_or_exit(ticket_block, "infraction_code");
    int32_t ticket_set_fine_amount_column_id       = get_column_id_by_name_or_exit(ticket_block, "set_fine_amount");
    int32_t ticket_time_of_infraction_column_id    = get_column_id_by_name_or_exit(ticket_block, "time_of_infraction");
    int32_t ticket_ticket_address_row_id_column_id = get_column_id_by_name_or_exit(ticket_block, "toronto_address_id");
    int32_t ticket_is_opp_column_id                = get_column_id_by_name_or_exit(ticket_block, "is_opp");
    
    int ticket_index;
    for (ticket_index = 0 ; ticket_index < ticket_block->num_rows ; ticket_index++)
    {
      int32_t ticket_date_of_infraction = get_cell_as_int32(ticket_block, ticket_index, ticket_date_of_infraction_column_id);
      int32_t ticket_infraction_code    = get_cell_as_int32(ticket_block, ticket_index, ticket_infraction_code_column_id);
      int32_t ticket_set_fine_amount    = get_cell_as_int32(ticket_block, ticket_index, ticket_set_fine_amount_column_id);
      int32_t ticket_time_of_infraction = get_cell_as_int32(ticket_block, ticket_index, ticket_time_of_infraction_column_id);
      int32_t ticket_address_row_id     = get_cell_as_int32(ticket_block, ticket_index, ticket_ticket_address_row_id_column_id);
      int32_t ticket_is_opp             = get_cell_as_int32(ticket_block, ticket_index, ticket_is_opp_column_id);
      
      //if (ticket_address_row_id != 20) continue; // kbfu // 49 CAMDEN ST
      //if (ticket_address_row_id != 28) continue; // kbfu // 50 CAMDEN ST
      
      //if (ticket_infraction_code != 3) continue; // kbfu
      
      if (should_ignore_infraction(ticket_infraction_code)) continue;
      //if (ticket_is_opp == 0) continue; kbfu
      
      //fprintf(stderr, "%d %04d %3d %d\n", ticket_date_of_infraction, ticket_time_of_infraction, ticket_infraction_code, ticket_is_opp);
      
      if (ticket_address_row_id > addresses->num_rows) { fprintf(stderr, "encountered invalid address_id in tickets file - perhaps tickets file needs to be regenerated\n"); break; }
      
      //int32_t * addresses_num_all_tickets     = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_all_tickets_column_id);
      //int32_t * addresses_num_dumbass_tickets = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_dumbass_tickets_column_id);
      //int32_t * addresses_num_ignored_tickets = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_ignored_tickets_column_id);
      //int32_t * addresses_total_fined         = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_total_fined_column_id);
      
      //int32_t * addresses_num_dumbass_tickets = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_dumbass_tickets_column_id);
      //int32_t * addresses_num_ignored_tickets = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_ignored_tickets_column_id);
      //int32_t * addresses_total_fined         = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_total_fined_column_id);
      
      int32_t * addresses_num_AT_tickets                     = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_AT_tickets_column_id);
      int32_t * addresses_num_AT_tickets_within_1_hour       = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_AT_tickets_within_1_hour_column_id);
      int32_t * addresses_num_AT_tickets_within_3_hour       = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_AT_tickets_within_3_hour_column_id);
      int32_t * addresses_num_AT_tickets_within_6_hour       = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_AT_tickets_within_6_hour_column_id);
      int32_t * addresses_num_AT_meter_tickets               = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_AT_meter_tickets_column_id);
      int32_t * addresses_num_AT_meter_tickets_within_1_hour = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_AT_meter_tickets_within_1_hour_column_id);
      int32_t * addresses_num_AT_meter_tickets_within_3_hour = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_AT_meter_tickets_within_3_hour_column_id);
      int32_t * addresses_num_AT_meter_tickets_within_6_hour = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_AT_meter_tickets_within_6_hour_column_id);
      
      int32_t * addresses_num_OP_tickets                     = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_OP_tickets_column_id);
      int32_t * addresses_num_OP_tickets_within_1_hour       = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_OP_tickets_within_1_hour_column_id);
      int32_t * addresses_num_OP_tickets_within_3_hour       = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_OP_tickets_within_3_hour_column_id);
      int32_t * addresses_num_OP_tickets_within_6_hour       = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_OP_tickets_within_6_hour_column_id);
      int32_t * addresses_num_OP_meter_tickets               = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_OP_meter_tickets_column_id);
      int32_t * addresses_num_OP_meter_tickets_within_1_hour = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_OP_meter_tickets_within_1_hour_column_id);
      int32_t * addresses_num_OP_meter_tickets_within_3_hour = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_OP_meter_tickets_within_3_hour_column_id);
      int32_t * addresses_num_OP_meter_tickets_within_6_hour = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_num_OP_meter_tickets_within_6_hour_column_id);
      
      //(*addresses_total_fined) += ticket_set_fine_amount;
      //(*addresses_num_all_tickets)++;
      
      if (ticket_is_opp != 1)
      {
        if (is_meter_infraction(ticket_infraction_code)) (*addresses_num_AT_meter_tickets)++;
        (*addresses_num_AT_tickets)++;
      }
      else
      {
        if (is_meter_infraction(ticket_infraction_code)) (*addresses_num_OP_meter_tickets)++;
        (*addresses_num_OP_tickets)++;
      }
      
      //if (is_dumbass_infraction(ticket_infraction_code)) (*addresses_num_dumbass_tickets)++;
      //if (is_ignored_infraction(ticket_infraction_code)) (*addresses_num_ignored_tickets)++;
      
      int ticket_wday = get_wday(ticket_date_of_infraction);
      
      if (ticket_time_of_infraction >= 0 && ticket_time_of_infraction <= 2400)
      {
        int32_t hour = floor(ticket_time_of_infraction / 100);
        int32_t min = ticket_time_of_infraction - hour * 100;
        int32_t min_in_day = hour*60 + min;
        
        int minute_windows[3] = { 60, 180, 360 };
        int num_minute_frames = 3;
        
        int mf;
        for (mf = 0 ; mf < num_minute_frames ; mf++)
        {
          if ((now_in_minutes+minute_windows[mf] <= 1440 && 
              (ticket_wday == now_wday && min_in_day > now_in_minutes && min_in_day < now_in_minutes+minute_windows[mf])) ||
              (now_in_minutes+minute_windows[mf] > 1440 &&
              (ticket_wday == now_wday+1 || (ticket_wday == 0 && now_wday == 6)) && 
                min_in_day > (now_in_minutes-1440) && min_in_day < (now_in_minutes-1440+minute_windows[mf])))
          {
            if (ticket_is_opp == 1)
            {
              if (is_meter_infraction(ticket_infraction_code))
              {
                if      (mf == 0) *addresses_num_OP_meter_tickets_within_1_hour++;
                else if (mf == 1) *addresses_num_OP_meter_tickets_within_3_hour++;
                else if (mf == 2) *addresses_num_OP_meter_tickets_within_6_hour++;
              }
              else
              {
                if      (mf == 0) *addresses_num_OP_tickets_within_1_hour++;
                else if (mf == 1) *addresses_num_OP_tickets_within_3_hour++;
                else if (mf == 2) *addresses_num_OP_tickets_within_6_hour++;
              }
            }
            else
            {
              if (is_meter_infraction(ticket_infraction_code))
              {
                if      (mf == 0) *addresses_num_AT_meter_tickets_within_1_hour++;
                else if (mf == 1) *addresses_num_AT_meter_tickets_within_3_hour++;
                else if (mf == 2) *addresses_num_AT_meter_tickets_within_6_hour++;
              }
              else
              {
                if      (mf == 0) *addresses_num_AT_tickets_within_1_hour++;
                else if (mf == 1) *addresses_num_AT_tickets_within_3_hour++;
                else if (mf == 2) *addresses_num_AT_tickets_within_6_hour++;
              }
            }
          }
        }
        
        /*
        if (ticket_wday == now_wday && min_in_day > now_in_minutes && min_in_day < now_in_minutes + 180)
        {
          if (strcmp(ticket_location1, "OP")==0) (*addresses_num_OP_tickets_within_3_hour)++;
          else (*addresses_num_AT_tickets_within_3_hour)++;
        }
        if (ticket_wday == now_wday && min_in_day > now_in_minutes && min_in_day < now_in_minutes + 360)
        {
          if (strcmp(ticket_location1, "OP")==0) (*addresses_num_OP_tickets_within_6_hour)++;
          else (*addresses_num_AT_tickets_within_6_hour)++;
        }*/
        
        
        //int32_t hhly = floor(min_in_day / (1440.0/(float)DAY_DIVISION));
        //if (hhly < 0 || hhly > DAY_DIVISION) { fprintf(stderr, "bad hhly\n"); exit(0); }
        //int32_t * hhly_cell = (int32_t*)get_cell(addresses, ticket_address_row_id, addresses_wday_hhly_column_id_start + hhly + wday*DAY_DIVISION);
        //(*hhly_cell)++;
      }
      else
      {
        fprintf(stderr, "error with time_of_infraction %d (%s, row %d %d)\n", ticket_time_of_infraction, ticket_filename, ticket_index, ticket_date_of_infraction);
        int32_t j = 0;
        fprintf(stderr, "ticket_block->num_columns = %d\n", ticket_block->num_columns);
        for (j = 0 ; j < ticket_block->num_columns ; j++)
        {
          struct Column * column = get_column(ticket_block, j);
          if (column->type == TYPE_INT)
            fprintf(stderr, "%s: %d\n", column_get_name(column), get_cell_as_int32(ticket_block, ticket_index, j));
          else if (column->type == TYPE_FLOAT)
            fprintf(stderr, "%s: %f\n", column_get_name(column), get_cell_as_double(ticket_block, ticket_index, j));
          else if (column->type == TYPE_CHAR)
            fprintf(stderr, "%s: %s\n", column_get_name(column), (char*)get_cell(ticket_block, ticket_index, j));
        }
        exit(0);
      }
      
      num_tickets_total++;
      if (add_usleep && num_tickets_total%5000==0) usleep(50000); // just to slow it down, dont want this taking 100% cpu after all
      if (num_tickets_total%500000==0) fprintf(stderr, "num_tickets_total = %d (%.5f seconds running)\n", num_tickets_total, (float)(get_msec() - start) / 1000.0);
    }
    //fprintf(stderr, "tickets in year %d with no location2: %d\n", year, tickets_with_no_location2);
    //fprintf(stderr, "tickets in year %d with bad location2: %d\n", year, tickets_with_bad_location2);
    
    free_block(ticket_block);
    
    //break; // kbfu, only 2008 for you
  }
  
  fprintf(stderr, "num_tickets_total = %d\n", num_tickets_total);
  
  struct Attribute * tickets_vs_now_attribute = get_attribute_by_name(addresses, "tickets_vs_now");
  attribute_set_value(tickets_vs_now_attribute, &now_t);
  
  struct Attribute * num_tickets_total_attribute = get_attribute_by_name(addresses, "num_tickets_total");
  attribute_set_value(num_tickets_total_attribute, &num_tickets_total);
  struct Attribute * num_ticket_years_attribute = get_attribute_by_name(addresses, "num_ticket_years");
  attribute_set_value(num_ticket_years_attribute, &num_ticket_filenames);
  
  //free_hashtable(addy_hash);
  
  write_block(stdout, addresses);
  free_block(addresses);
}
