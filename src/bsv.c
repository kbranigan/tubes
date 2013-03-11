
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>


#define UTF_8        1
#define UTF_16_BIG   2
#define UTF_16_SMALL 3

#define EXTRACT_HEADER 1
#define EXTRACT_DATA 2

#define ROWS_TO_EVALUATE_FOR_HEADER_SEARCH 10

//#define HEADER_SEARCH_DEBUG

struct sCounts {
  int num_characters;
  char * characters;
  int * charcounts;
};

struct sNotices {
  int num_notices;
  char ** notices;
};

struct sRow {
  int num_fields;
  char ** fields;
};

int number_of_utf8_extended_characters = 0;

int getcharacter(FILE * fp, int encoding)
{
  int c;
  if (encoding == UTF_8)
  {
    c = fgetc(fp);
    /*
    don't really need to do any of this, leaving this here so we know we aren't doing it
    if (c > 127)
    {
      if (c >= 192) // 2 bytes
      {
        //number_of_utf8_extended_characters ++;
        fgetc(fp);
      }
      else if (c >= 224) // 3 bytes
      {
        fgetc(fp);
        fgetc(fp);
      }
      else if (c >= 240) // 4 bytes
      {
        fgetc(fp);
        fgetc(fp);
        fgetc(fp);
      }
      else
      {
        fprintf(stderr, "%d\n", c);
        number_of_utf8_extended_characters ++;
      }
    }*/
  }
  else if (encoding == UTF_16_BIG)
    c = fgetc(fp) * 255 + fgetc(fp);
  else if (encoding == UTF_16_SMALL)
    c = fgetc(fp) + fgetc(fp) * 255;
  else
    c = fgetc(fp);
  
  return c;
}

char * _csv_get_line(FILE * fp, int * length_p, int encoding)
{
  /*
  this function returns a single plain text line.
  it does no parsing and might not be a complete csv line due to embedded carriage returns and line feeds.
  csv_get_line() calls this function until a complete row is achieved
  */
  
  char * line = NULL;
  int alloc_size = 0;
  int length = 0;
  int c;
  while (!feof(fp))
  {
    c = getcharacter(fp, encoding);
    
    if (c == 0x0D)
    {
      fpos_t pos;
      fgetpos(fp, &pos);
      
      int c2 = getcharacter(fp, encoding);
      
      if (c2 != 0x0A) fsetpos(fp, &pos); // get the next char - if it's 0x0A skip it, otherwise rewind once
      break;
    }
    else if (c == 0x0A) break;    // end of the line
    else if (c == 0x00) continue; // ignore null characters, some files prepend lots of them for fun
    else if (c != EOF)
    {
      length++;
      if (length >= alloc_size)
      {
        alloc_size += 1000;
        line = realloc(line, (sizeof(char) * alloc_size) + 1);
      }
      line[length-1] = c;
    }
  }
  if (line != NULL)
    line[length] = 0;
  
  *length_p = length;
  return line;
}

int get_character_distribution(struct sCounts * counts, char * string, char * allowed)
{
  if (string == NULL) return 0;
  if (counts == NULL) return 0;
  long l = strlen(string);
  long al = allowed == NULL ? 0 : strlen(allowed);
  int i,j;
  
  for (i = 0 ; i < l ; i++)
  {
    int found_index = -1;
    for (j = 0 ; j < al ; j++)
      if (string[i] == allowed[j])
        { found_index = j; break; }
    
    if (al > 0 && found_index == -1) continue;
    
    found_index = -1;
    for (j = 0 ; j < counts->num_characters ; j++)
      if (string[i] == counts->characters[j])
        { found_index = j; break; }
    
    if (found_index != -1 && found_index < counts->num_characters)
      counts->charcounts[found_index]++;
    else
    {
      char temp[2] = { string[i], 0 };
      counts->num_characters++;
      counts->characters = (char *)realloc(counts->characters, sizeof(char)*counts->num_characters);
      counts->characters[counts->num_characters-1] = string[i];
      counts->charcounts = (int *)realloc(counts->charcounts, sizeof(int)*counts->num_characters);
      counts->charcounts[counts->num_characters-1] = 1;
    }
  }
  return counts->num_characters;
}

void free_character_distribution(struct sCounts * counts)
{
  free(counts->characters);
  counts->characters = NULL;
  free(counts->charcounts);
  counts->charcounts = NULL;
  counts->num_characters = 0;
}

char get_peak_character(struct sCounts * counts)
{
  if (counts == NULL) return ',';
  int max_count = -1;
  int max_index = -1;
  int i;
  for (i = 0 ; i < counts->num_characters ; i++)
    if (counts->charcounts[i] > max_count)
    {
      max_count = counts->charcounts[i];
      max_index = i;
    }
  if (max_index != -1) return counts->characters[max_index];
  else return ',';
}

int get_bsv_row(struct sRow * row, char * string, char delimiter, int quote)
{
  if (row == NULL || string == NULL || delimiter == 0) return 0;
  
  // quote == 1 if this is a continued row
  int alloc_length = 0;
  int field_length = 0;
  row->num_fields++;
  row->fields = (char**)realloc(row->fields, sizeof(char*)*row->num_fields);
  row->fields[row->num_fields-1] = NULL;
  char * p = string;
  while (*p != 0)
  {
    if (*p == '"' && quote == 0 && field_length == 0 && (p == string || *(p-1) != '"'))
      quote = 1;
    else if (*p == '"' && quote == 1 && (*(p+1) == delimiter || *(p+1) == 0))
      quote = 0;
    else if (*p == '"' && *(p+1) == '"')
    {
      // skip one of the quotes
    }
    else if ((quote == 1 && *p == delimiter) || *p != delimiter)
    {
      field_length++;
      if (field_length >= alloc_length)
      {
        alloc_length += 10;
        row->fields[row->num_fields-1] = (char*)realloc(row->fields[row->num_fields-1], sizeof(char)*alloc_length);
      }
      row->fields[row->num_fields-1][field_length-1] = *p;
    }
    else if (*p == delimiter)
    {
      if (field_length > 0) // append NULL
      {
        field_length++;
        if (field_length >= alloc_length)
        {
          alloc_length += 10;
          row->fields[row->num_fields-1] = (char*)realloc(row->fields[row->num_fields-1], sizeof(char)*alloc_length);
        }
        row->fields[row->num_fields-1][field_length-1] = 0;
      }
      
      quote = 0;
      field_length = 0;
      alloc_length = 0;
      row->num_fields++;
      row->fields = (char**)realloc(row->fields, sizeof(char*)*row->num_fields);
      row->fields[row->num_fields-1] = NULL;
    }
    
    p++;
  }
  
  if (field_length > 0) // append NULL
  {
    field_length++;
    if (field_length >= alloc_length)
    {
      alloc_length += 10;
      row->fields[row->num_fields-1] = (char*)realloc(row->fields[row->num_fields-1], sizeof(char)*alloc_length);
    }
    row->fields[row->num_fields-1][field_length-1] = 0;
  }
  
  return quote;
}

void free_row(struct sRow * row)
{
  if (row == NULL) return;
  int i;
  for (i = 0 ; i < row->num_fields ; i++)
    free(row->fields[i]);
  free(row->fields);
  row->num_fields = 0;
  row->fields = NULL;
}

char * csv_get_line(FILE * fp, int * length_p, int encoding)
{
  /*
  
  This is a terrible hack at getting lines of content that span multiple lines.
  I definitely disagree that this should be allowed, but alas, csv standard it is.
  Calling get_bsv_row and get_character_distribution on every single row totally sucks.
  
  */
  
  int length = 0;
  char * line = _csv_get_line(fp, &length, encoding);
  
  char acceptable_delimiters[100] = ",|\t;";
  
  struct sCounts counts = { 0, NULL, NULL };
  get_character_distribution(&counts, line, acceptable_delimiters);
  int delimiter = get_peak_character(&counts);
  free_character_distribution(&counts);
  
  struct sRow row = { 0, NULL };
  
  int append_next_line_required = get_bsv_row(&row, line, delimiter, 0); // returns 1 if it the \n encountered wasn't properly quoted.
  free_row(&row);
  
  while (append_next_line_required)
  {
    fseek(fp, -2, SEEK_CUR);
    int c1 = getcharacter(fp, encoding);
    int c2 = getcharacter(fp, encoding);
    
    int next_length = 0;
    char * next_line = _csv_get_line(fp, &next_length, encoding);
    
    length += next_length;
    
    if (c1 == 0x0D && c2 == 0x0A) length += 2;
    else if (c2 == 0x0D || c2 == 0x0A) length += 1;
    
    line = realloc(line, length + 1);
    
    if (next_line != NULL)
      memcpy(line + (length - next_length), next_line, next_length);
    
    line[length] = 0;
    
    if (c1 == 0x0D && c2 == 0x0A) { line[length - next_length - 2] = 0x0D; line[length - next_length - 1] = 0x0A; }
    else if (c2 == 0x0D) line[length - next_length - 1] = 0x0D;
    else if (c2 == 0x0A) line[length - next_length - 1] = 0x0A;
    
    if (next_line != NULL)
    {
      append_next_line_required = get_bsv_row(&row, next_line, delimiter, 1);
      free_row(&row);
      free(next_line);
    }
    else if (feof(fp))
      append_next_line_required = 0;
  }
  
  *length_p = length;
  
  return line;
}

int add_notice(struct sNotices * notices, const char * fmt, ...)
{
  if (notices == NULL) return 0;
  
  notices->num_notices++;
  notices->notices = (char**)realloc(notices->notices, sizeof(char*)*notices->num_notices);
  
  char buf[1000];
  va_list ap;
  va_start(ap, fmt);
  (void) vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  
  notices->notices[notices->num_notices-1] = malloc(strlen(buf)+1);
  strncpy(notices->notices[notices->num_notices-1], buf, strlen(buf)+1);
  
  return notices->num_notices;
}

void free_notices(struct sNotices * notices)
{
  int i;
  for (i = 0 ; i < notices->num_notices ; i++)
    free(notices->notices[i]);
  free(notices->notices);
  notices->num_notices = 0;
  notices->notices = NULL;
}


struct Block * bsv(char * filename, int instruction)
{
  //const char * filename = "../colors_for_streets.csv";
  //int instruction = EXTRACT_HEADER;
  //int instruction = EXTRACT_DATA;
  
  struct Block * block = new_block();
  
  clock_t start;
  start = clock();
  
  char acceptable_delimiters[100] = ",|\t;";
  
  char delimiter = 0;
  int header_line_number = -1;
  
  int i;
  
  struct sNotices notices = { 0, NULL };
  struct sNotices errors = { 0, NULL };
  
  FILE * fp = fopen(filename, "rb");
  
  if (fp == NULL) { add_notice(&errors, "Could not open file"); fprintf(stderr, "Could not open file\n"); return 0; }
  
  int c1 = fgetc(fp);
  int c2 = fgetc(fp);
  
  int encoding = UTF_8;
  
  if ((c1 == 0xFE && c2 == 0xFF))
  {
    add_notice(&notices, "UTF-16 small endian byte order mark encountered");
    encoding = UTF_16_BIG;
  }
  else if ((c1 == 0xFF && c2 == 0xFE))
  {
    add_notice(&notices, "UTF-16 small endian byte order mark encountered");
    encoding = UTF_16_SMALL;
  }
  else
    rewind(fp);
  
  int length = 0;
  char * line = NULL;
  
  struct tRowStat {
    int line_number;
    int num_fields;
    int num_null_fields;
    int total_alpha;
    int total_numeric;
    int total_characters;
    int delimiter;
  };
  
  struct tRowStat * row_stats = NULL;
  int num_row_stats = 0;
  
  int header_has_trailing_delimiter = 0;
  int num_lines_total = 0, num_blank_lines = 0, num_comment_lines = 0, num_content_rows = 0, number_of_incorrect_number_of_fields = 0;
  while (!feof(fp))
  {
    line = csv_get_line(fp, &length, encoding);
    
    num_lines_total++;
    if (line == NULL || length == 0 || line[0] == 0 || line[0] == 0x0A || line[0] == 0x0D) { num_blank_lines++; free(line); continue; }
    
    // need some row_stats before knowing where the header is and where the content begins
    if (num_row_stats < ROWS_TO_EVALUATE_FOR_HEADER_SEARCH)
    {
      num_row_stats++;
      row_stats = realloc(row_stats, sizeof(struct tRowStat)*num_row_stats);
      struct tRowStat * row_stat = &row_stats[num_row_stats-1];
      
      row_stat->line_number = num_lines_total - 1;
      
      struct sCounts delimiter_counts = { 0, NULL, NULL };
      get_character_distribution(&delimiter_counts, line, acceptable_delimiters);
      row_stat->delimiter = get_peak_character(&delimiter_counts);
      //if (delimiter_counts.num_characters == 0) add_notice(&notices, "Encountered a row with no valid delimiters: '%s'", line);
      free_character_distribution(&delimiter_counts);
      
      struct sRow header_row = { 0, NULL };
      get_bsv_row(&header_row, line, row_stat->delimiter, 0);
      
      row_stat->num_fields = header_row.num_fields;
      
      row_stat->num_null_fields = 0;
      for (i = 0 ; i < header_row.num_fields ; i++)
        if (header_row.fields[i] == NULL || strlen(header_row.fields[i]) == 0)
          row_stat->num_null_fields++;
      
      int is_comment = 0;
      for (i = 0 ; i < header_row.num_fields ; i++)
      {
        if (header_row.fields[i] != NULL)
        {
          unsigned int j;
          for (j = 0 ; j < strlen(header_row.fields[i]) ; j++)
          {
            if (header_row.fields[i][j] == ' ') continue;
            else if (header_row.fields[i][j] == '#') { is_comment = 1; break; }
          }
          if (is_comment == 1) break;
        }
      }
      
      if (is_comment == 1 && header_row.num_fields == 1)
      {
        num_comment_lines++;
        free_row(&header_row);
        free(line);
        continue;
      }
      
      free_row(&header_row);
      
      row_stat->total_alpha = 0;
      row_stat->total_characters = 0;
      struct sCounts counts = { 0, NULL, NULL };
      get_character_distribution(&counts, line, NULL);
      for (i = 0 ; i < counts.num_characters ; i++)
      {
        if ((counts.characters[i] >= 'A' && counts.characters[i] <= 'Z') || (counts.characters[i] >= 'a' && counts.characters[i] <= 'z'))
          row_stat->total_alpha += counts.charcounts[i];
        
        if ((counts.characters[i] >= '0' && counts.characters[i] <= '9') || counts.characters[i] == '$')
          row_stat->total_numeric += counts.charcounts[i];
      }
      
      for (i = 0 ; i < counts.num_characters ; i++)
        if (counts.characters[i] != row_stat->delimiter && counts.characters[i] != '"')
          row_stat->total_characters += counts.charcounts[i];
      
      free_character_distribution(&counts);
    }
    free(line);
    if (num_lines_total > ROWS_TO_EVALUATE_FOR_HEADER_SEARCH) break;
  }
  
  int min_num_null_fields = 100000;
  int max_num_fields = 0;
  
  // update header_line_number to the first line with the maximum number of fields and minimum number of null fields
  int k;
  for (k = 0 ; k < num_row_stats ; k++)
  {
    #ifdef HEADER_SEARCH_DEBUG
    char temp[2] = { row_stats[k].delimiter, 0 };
    fprintf(stderr, "row_stats[%d].line_number = %3d   .num_null_fields = %2d   .num_fields = %2d  .delimiter = %s\n", k, row_stats[k].line_number, row_stats[k].num_null_fields, row_stats[k].num_fields, temp);
    #endif
    if (row_stats[k].num_null_fields < min_num_null_fields) min_num_null_fields = row_stats[k].num_null_fields;
    if (row_stats[k].num_fields > max_num_fields) max_num_fields = row_stats[k].num_fields;
  }
  
  for (k = 0 ; k < num_row_stats ; k++)
  {
    if (row_stats[k].num_null_fields == min_num_null_fields && row_stats[k].num_fields == max_num_fields)
    {
      header_line_number = row_stats[k].line_number;
      break;
    }
  }
  
  if (header_line_number == -1)
  for (k = 0 ; k < num_row_stats ; k++)
  {
    if (row_stats[k].num_null_fields == min_num_null_fields && row_stats[k].num_fields == max_num_fields)
    {
      header_line_number = row_stats[k].line_number;
      break;
    }
  }
  
  #ifdef HEADER_SEARCH_DEBUG
  fprintf(stderr, "header_line_number = %d\n", header_line_number);
  fprintf(stderr, "max_num_fields = %d\n", max_num_fields);
  fprintf(stderr, "min_num_null_fields = %d\n", min_num_null_fields);
  //fprintf(stderr, "min_num_null_fields = %d\n", min_num_null_fields);
  #endif
  
  rewind(fp);
  if (encoding == UTF_16_BIG || encoding == UTF_16_SMALL) { fgetc(fp); fgetc(fp); }
  
  for (k = 0 ; k < header_line_number ; k++)
  {
    line = csv_get_line(fp, &length, encoding);
    free(line);
  }
  
  line = csv_get_line(fp, &length, encoding);
  
  struct sRow header_row = { 0, NULL };
  get_bsv_row(&header_row, line, row_stats[header_line_number].delimiter, 0);
  
  add_notice((header_row.num_fields == 0) ? &errors : &notices, "Header has %d fields", header_row.num_fields);
  
  free(line);
  
  if (instruction == EXTRACT_HEADER)
  {
    if (header_line_number == -1) return 0;
    
    int j;
    for (j = 0 ; j < header_row.num_fields ; j++)
    {
      printf("%s%s", header_row.fields[j], (j == header_row.num_fields - 1) ? "\n" : "|");
    }
    
    return 0;
  }
  else if (instruction == EXTRACT_DATA)
  {
    int i;
    for(i = 0 ; i < header_row.num_fields ; i++)
    {
      block = add_string_column_with_length(block, header_row.fields[i], 60); // major kbfu oh well
    }
    
    int32_t * max_column_widths = malloc(sizeof(int32_t)*header_row.num_fields);
    memset(max_column_widths, 0, sizeof(int32_t)*header_row.num_fields);
    
    while (!feof(fp))
    {
      line = csv_get_line(fp, &length, encoding);
      
      if (line == NULL || length == 0 || line[0] == 0 || line[0] == 0x0A || line[0] == 0x0D) { num_blank_lines++; free(line); continue; }
      
      num_content_rows++;
      struct sRow row = { 0, NULL };
      get_bsv_row(&row, line, row_stats[0].delimiter, 0);
      
      if (instruction == EXTRACT_DATA)
      {
        block = add_row(block);
        int j;
        for (j = 0 ; j < row.num_fields && j < block->num_columns ; j++)
        {
          if (row.fields[j] != NULL)
          {
            if (strlen(row.fields[j]) > 60)
            {
              fprintf(stderr, "strlen(row[%d].fields[%d]) = %ld\n", block->num_rows, j, (long)strlen(row.fields[j]));
              return block;
            }
            if (strlen(row.fields[j]) > max_column_widths[j])
              max_column_widths[j] = strlen(row.fields[j]);
            
            set_cell(block, block->num_rows-1, j, row.fields[j]);
          }
          else
            set_cell(block, block->num_rows-1, j, NULL);
        }
        if (block->num_rows % 100000 == 0) fprintf(stderr, "%d rows\n", block->num_rows);
      }
      
      free(line);
      free_row(&row);
    }
    
    struct Block * condensed_block = new_block();
    condensed_block = add_string_attribute(condensed_block, "source", filename);
    for (i = 0 ; i < block->num_columns ; i++)
    {
      condensed_block = add_string_column_with_length(condensed_block, column_get_name(get_column(block, i)), max_column_widths[i]);
    }
    
    condensed_block = set_num_rows(condensed_block, block->num_rows);
    
    for (i = 0 ; i < block->num_rows ; i++)
    {
      int j;
      for (j = 0 ; j < block->num_columns ; j++)
      {
        set_cell(condensed_block, i, j, get_cell(block, i, j));
      }
    }
    
    free_block(block);
    block = condensed_block;
    free(max_column_widths);
  }
  
  free_row(&header_row);
  
  num_row_stats = 0;
  free(row_stats);
  
  free_notices(&notices);
  
  fclose(fp);
  
  return block;
}
