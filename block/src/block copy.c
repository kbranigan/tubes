
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <signal.h>
#include <execinfo.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <inttypes.h>

#include "block.h"
#include "std_helpers.h" // includes C source actually

static char ** running_command = NULL;
void block_segfault_handler(int sig)
{
  fprintf(stderr, "\nSIGSEGV (segfault) in : ");
  if (running_command != NULL)
  {
    int i = 0;
    while (running_command[i] != NULL)
      fprintf(stderr, "%s ", running_command[i++]);
  }
  fprintf(stderr, "\n");
  
  void * array[32];
  size_t size = backtrace(array, 32);
  
  char ** strings = backtrace_symbols(array, size);
  
  int i;
  for (i = 0 ; i < size ; i++) fprintf(stderr, "%s\n", strings[i]);
  
  fprintf(stderr, "\n\n");
  exit(EXIT_FAILURE);
}

void setup_segfault_handling(char ** command)
{
  running_command = command;
  signal(SIGSEGV, block_segfault_handler);
}

char block_type_names[6][20] = {
  "unknown", "int", "long", "float", "double", "string"
};

size_t block_header_size()
{
  return sizeof(int32_t)*7;
}

struct Block * new_block()
{
  if (sizeof(struct Block) != block_header_size()) { fprintf(stderr, "sizeof(struct Block) is the wrong size (%ld) - should be %ld, perhaps padding or memory alignment works differently for your machine?\n", sizeof(struct Block), block_header_size()); exit(1); }
  
  struct Block * block = (struct Block*)malloc(sizeof(struct Block));
  memset(block, 0, sizeof(struct Block));
  return block;
}

void realloc_block(struct Block ** pBlock)
{
  *pBlock = (struct Block *)realloc(*pBlock, sizeof(struct Block) + (*pBlock)->attributes_bsize + (*pBlock)->columns_bsize + (*pBlock)->data_bsize);
}

void set_num_rows(struct Block ** pBlock, int32_t num_rows)
{
  (*pBlock)->num_rows = num_rows;
  (*pBlock)->data_bsize = (*pBlock)->row_bsize * (*pBlock)->num_rows;
  realloc_block(pBlock);
}

struct Block * read_block(FILE * fp)
{
  if (fp == NULL) { fprintf(stderr, "trying to read from NULL file pointer\n"); return 0; }
  
  float header;
  if (fread(&header, sizeof(header), 1, fp) != 1) return NULL;
  if (isfinite(header)) { fprintf(stderr, "header (%f) is finite (it's suppose to be infinite, either no data or invalid data was read)\n", header); return NULL; }
  
  int32_t is_block;
  if (fread(&is_block, sizeof(is_block), 1, fp) != 1) { fprintf(stderr, "fread block is_block failed.\n"); return NULL; }
  if (is_block != 1) { fprintf(stderr, "is_block != 1\n"); return NULL; }
  
  struct Block * block = (struct Block*)malloc(sizeof(struct Block));
  if (fread(block, sizeof(struct Block), 1, fp) != 1) { fprintf(stderr, "fread block failed (error 1)\n"); return NULL; }
  realloc_block(&block);
  
  if (fread(&block[1], block->attributes_bsize + block->columns_bsize + block->data_bsize, 1, fp) != 1) { fprintf(stderr, "fread block failed (error 2)\n"); return NULL; }
  
  return block;
}

void write_block(FILE * fp, struct Block * block)
{
  if (block == NULL) { fprintf(stderr, "write_block passed null block.\n"); return; }
  
  float inf = INFINITY;
  if (fwrite(&inf, sizeof(inf), 1, fp) != 1) { fprintf(stderr, "fwrite block header failed.\n"); return; }
  
  int32_t is_block = 1;
  if (fwrite(&is_block, sizeof(is_block), 1, fp) != 1) { fprintf(stderr, "fwrite block is_block failed.\n"); return; }
  if (fwrite(block, sizeof(struct Block) + block->attributes_bsize + block->columns_bsize + block->data_bsize, 1, fp) != 1) { fprintf(stderr, "fwrite block failed.\n"); return; }
}

void free_block(struct Block * block)
{
  free(block);
}

struct Block * add_command(struct Block * block, int argc, char ** argv)
{
  if (block == NULL || argc == 0 || argv == NULL) return block;
  
  int length = 100;
  char * full_command = malloc(length);
  sprintf(full_command, "");
  int i = 0;
  for (i = 0 ; i < argc ; i++)
  {
    if (strlen(full_command) + strlen(argv[i]) + 2 > length) {
      length += 100;
      full_command = realloc(full_command, length);
    }
    strcat(full_command, argv[i]);
    if (i < argc-1) strcat(full_command, " ");
  }
  //block = 
  add_string_attribute(&block, "command", full_command);
  free(full_command);
  return block;
}

int32_t memory_pad(int32_t i)
{
  return ceil(i / 4.0) * 4.0;
}

int32_t get_type_size(int32_t type)
{
  if (type == INT_TYPE)         return sizeof(int32_t);
  else if (type == LONG_TYPE)   return sizeof(long);
  else if (type == FLOAT_TYPE)  return sizeof(float);
  else if (type == DOUBLE_TYPE) return sizeof(double);
  else if (type >= 5)           return memory_pad(type);
  else fprintf(stderr, "column_get_cell_size invalid type '%d'\n", type);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

int32_t * get_attribute_offsets(struct Block * block) {
  return (int32_t*)((char*)block + sizeof(struct Block));
}

struct Attribute * get_first_attribute(struct Block * block) {
  return (block == NULL) ? NULL : (struct Attribute *)((char*)block + sizeof(struct Block) + sizeof(int32_t)*block->num_attributes);
}

struct Attribute * get_next_attribute(struct Block * block, struct Attribute * attribute) {
  return (block == NULL || attribute == NULL) ? NULL : (struct Attribute *)((char*)attribute + sizeof(struct Attribute) + attribute->name_length + attribute->value_length);
}

struct Attribute * get_attribute(struct Block * block, int32_t attribute_id)
{
  if (block == NULL || attribute_id < 0 || attribute_id > block->num_attributes)
  {
    fprintf(stderr, "get_attribute(%ld, %d) FAILED\n", (long int)block, attribute_id);
    return NULL;
  }
  
  int32_t * attribute_offsets = get_attribute_offsets(block);
  return (struct Attribute*)((char*)block + sizeof(struct Block) + attribute_offsets[attribute_id]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline int32_t * get_column_offsets(const struct Block * block) {
  return (int32_t*)((char*)block + sizeof(struct Block) + block->attributes_bsize);
}

inline int32_t * get_cell_offsets(const struct Block * block) {
  return (int32_t*)((char*)block + sizeof(struct Block) + block->attributes_bsize + sizeof(int32_t)*block->num_columns);
}

inline struct Column * get_first_column(const struct Block * block) {
  return (block == NULL) ? NULL : (struct Column *)((char*)block + sizeof(struct Block) + block->attributes_bsize + (sizeof(int32_t)+sizeof(int32_t))*block->num_columns);
}

inline struct Column * get_next_column(const struct Block * block, const struct Column * column) {
  return (block == NULL || column == NULL) ? NULL : (struct Column *)((char*)column + sizeof(struct Column) + column->name_length);
}

struct Column * get_column(struct Block * block, int32_t column_id)
{
  if (block == NULL || column_id < 0 || column_id > block->num_columns)
  {
    fprintf(stderr, "get_column(%ld, %d) FAILED\n", (long int)block, column_id);
    return NULL;
  }
  
  int32_t * column_offsets = get_column_offsets(block);
  return (struct Column*)((char*)block + sizeof(struct Block) + block->attributes_bsize + column_offsets[column_id]);
  
  int i = 0;
  struct Column * column = get_first_column(block);
  for (i = 0 ; i != column_id ; i++)
    column = get_next_column(block, column);
  
  return column;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void * get_row(struct Block * block, int32_t row_id)
{
  return (void*)&block[1] + block->attributes_bsize + block->columns_bsize + block->row_bsize * row_id;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

int32_t get_row_bsize_from_columns(struct Block * block)
{
  int32_t single_row_bsize = 0;
  int32_t column_id;
  for (column_id = 0 ; column_id < block->num_columns ; column_id++)
  {
    struct Column * column = get_column(block, column_id);
    single_row_bsize += get_type_size(column->type);
  }
  
  return single_row_bsize;
}

int32_t find_column_id_by_name(struct Block * block, char * column_name)
{
  int32_t column_id;
  for (column_id = 0 ; column_id < block->num_columns ; column_id++)
  {
    struct Column * column = get_column(block, column_id);
    if (strcmp(column_name, column_get_name(column))==0) return column_id;
  }
  return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

char * attribute_get_name(struct Attribute * attribute)  { return (char*)attribute + sizeof(struct Attribute); }
char * attribute_get_value(struct Attribute * attribute) { return (char*)attribute + sizeof(struct Attribute) + attribute->name_length; }
void attribute_set_name(struct Attribute * attribute, char * name)   { strncpy((char*)attribute + sizeof(struct Attribute), name, attribute->name_length); }
void attribute_set_value(struct Attribute * attribute, char * value) { strncpy((char*)attribute + sizeof(struct Attribute) + attribute->name_length, value, attribute->value_length); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void _add_attribute(struct Block ** pBlock, int32_t type, char * name, void * value)
{
  if (pBlock == NULL || *pBlock == NULL) { fprintf(stderr, "add_attribute called on a NULL block\n"); return; }
  
  int32_t name_length = memory_pad(strlen(name)+1);
  int32_t value_length = type==STRING_TYPE ? memory_pad(strlen(value)+1) : get_type_size(type);
  
  int32_t old_attributes_bsize = (*pBlock)->attributes_bsize;
  int32_t single_attribute_bsize = sizeof(struct Attribute) + name_length + value_length;
  
  (*pBlock)->attributes_bsize += 
              single_attribute_bsize + 
              sizeof(int32_t); // attributes_offsets
  (*pBlock)->num_attributes ++;
  realloc_block(pBlock);
  
  // move content after attributes, making room for the additional attribute
  if ((*pBlock)->num_columns > 0 || (*pBlock)->num_rows > 0)
  {
    memmove((char*)(*pBlock) + sizeof(struct Block) + old_attributes_bsize + single_attribute_bsize + sizeof(int32_t),
            (char*)(*pBlock) + sizeof(struct Block) + old_attributes_bsize,
            (*pBlock)->columns_bsize + (*pBlock)->data_bsize);
  }
  
  // shift attributes over so there is room for the attribute_offset
  if (old_attributes_bsize > 0)
  {
    memmove((char*)(*pBlock) + sizeof(struct Block) + sizeof(int32_t),
            (char*)(*pBlock) + sizeof(struct Block),
            old_attributes_bsize);
  }
  
  int32_t * attribute_offsets = get_attribute_offsets(*pBlock);
  
  int i;
  struct Attribute * attribute = get_first_attribute(*pBlock);
  for (i = 0 ; i < (*pBlock)->num_attributes - 1 ; i++)
  {
    attribute_offsets[i] = (int32_t)((char*)attribute - (char*)(*pBlock) - sizeof(struct Block));
    attribute = get_next_attribute(*pBlock, attribute);
  }
  
  attribute->type = type;
  attribute->name_length = name_length;
  attribute->value_length = value_length;
  attribute_set_name(attribute, name);
  attribute_set_value(attribute, value);
  attribute_offsets[(*pBlock)->num_attributes - 1] = (int32_t)((char*)attribute - (char*)(*pBlock) - sizeof(struct Block));
}

void add_int_attribute(struct Block ** pBlock, char * name, int32_t value)   { _add_attribute(pBlock, INT_TYPE, name, &value); }
void add_long_attribute(struct Block ** pBlock, char * name, long value)     { _add_attribute(pBlock, LONG_TYPE, name, &value); }
void add_float_attribute(struct Block ** pBlock, char * name, float value)   { _add_attribute(pBlock, FLOAT_TYPE, name, &value); }
void add_double_attribute(struct Block ** pBlock, char * name, double value) { _add_attribute(pBlock, DOUBLE_TYPE, name, &value); }
void add_string_attribute(struct Block ** pBlock, char * name, char * value) { _add_attribute(pBlock, STRING_TYPE, name, value); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////

char * column_get_name(struct Column * column)  { return (char*)column + sizeof(struct Column); }
void column_set_name(struct Column * column, char * name) { strncpy(column_get_name(column), name, column->name_length); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void _add_column(struct Block ** pBlock, int32_t type, char * name)
{
  if (pBlock == NULL || *pBlock == NULL) { fprintf(stderr, "add_field called on a NULL block\n"); return; }
  
  int32_t name_length = memory_pad(strlen(name)+1);
  int32_t old_columns_bsize = (*pBlock)->columns_bsize;
  int32_t old_data_bsize = (*pBlock)->data_bsize;
  int32_t old_row_bsize = (*pBlock)->row_bsize;
  
  (*pBlock)->row_bsize += get_type_size(type);
  
  int32_t single_column_bsize = sizeof(struct Column) + name_length;
  
  (*pBlock)->columns_bsize += 
              single_column_bsize + 
              sizeof(int32_t) + // column_offsets
              sizeof(int32_t); // cell_offsets
  (*pBlock)->num_columns++;
  set_num_rows(pBlock, (*pBlock)->num_rows);
  realloc_block(pBlock);
  
  // kbfu - interpolation of all rows of data, not easy
  if ((*pBlock)->num_rows > 0)
  {
    int row_id;
    for (row_id = (*pBlock)->num_rows - 1 ; row_id >= 0 ; row_id--)
    {
      memmove(
        (char*)(*pBlock) + sizeof(struct Block) + (*pBlock)->attributes_bsize + (*pBlock)->columns_bsize + (*pBlock)->row_bsize*row_id,
        (char*)(*pBlock) + sizeof(struct Block) + (*pBlock)->attributes_bsize + old_columns_bsize + old_row_bsize*row_id,
        old_row_bsize);
    }
  }
  
  // shift attributes over so there is room for the attribute_offsets and cell_offsets
  if (old_columns_bsize > 0)
  {
    memmove((char*)(*pBlock) + sizeof(struct Block) + (*pBlock)->attributes_bsize + sizeof(int32_t) + sizeof(int32_t),
            (char*)(*pBlock) + sizeof(struct Block) + (*pBlock)->attributes_bsize,
            old_columns_bsize);
  }
  
  int32_t * column_offsets = get_column_offsets(*pBlock);
  int32_t * cell_offsets = get_cell_offsets(*pBlock);
  cell_offsets[0] = 0;
  
  int o = 0;
  int i;
  struct Column * column = get_first_column(*pBlock);
  for (i = 0 ; i < (*pBlock)->num_columns - 1 ; i++)
  {
    cell_offsets[i] = o;
    o += get_type_size(column->type);
    column_offsets[i] = (int32_t)((char*)column - (char*)(*pBlock) - sizeof(struct Block) - (*pBlock)->attributes_bsize);
    column = get_next_column(*pBlock, column);
  }
  
  column->type = type;
  column->name_length = name_length;
  column_set_name(column, name);
  column_offsets[(*pBlock)->num_columns - 1] = (int32_t)((char*)column - (char*)(*pBlock) - sizeof(struct Block) - (*pBlock)->attributes_bsize);
  if ((*pBlock)->num_columns != 1) cell_offsets[(*pBlock)->num_columns - 1] = o;
  
  void * m = malloc(get_type_size(column->type));
  for (i = 0 ; i < (*pBlock)->num_rows ; i++)
    set_cell((*pBlock), i, (*pBlock)->num_columns-1, m);
  free(m);
}

void add_int_column(struct Block ** pBlock, char * name)    { _add_column(pBlock, INT_TYPE, name); }
void add_long_column(struct Block ** pBlock, char * name)   { _add_column(pBlock, LONG_TYPE, name); }
void add_float_column(struct Block ** pBlock, char * name)  { _add_column(pBlock, FLOAT_TYPE, name); }
void add_double_column(struct Block ** pBlock, char * name) { _add_column(pBlock, DOUBLE_TYPE, name); }
void add_string_column_with_length(struct Block ** pBlock, char * name, int32_t length) { if (length < 5) length = 5; _add_column(pBlock, length, name); }

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void * get_cell(struct Block * block, int32_t row_id, int32_t column_id)
{
  int32_t * cell_offsets = get_cell_offsets(block);
  return (char*)get_row(block, row_id) + cell_offsets[column_id];
}

void set_cell(struct Block * block, int row_id, int column_id, void * value)
{
  if (block == NULL) { fprintf(stderr, "set_cell called with null block\n"); exit(0); }
  if (row_id >= block->num_rows) { fprintf(stderr, "set_cell called with row_id >= block->num_rows\n"); exit(0); }
  if (column_id >= block->num_columns) { fprintf(stderr, "set_cell called with column_id(%d) >= block->num_columns(%d)\n", column_id, block->num_columns); exit(0); }
  
  struct Column * column = get_column(block, column_id);
  void * cell = get_cell(block, row_id, column_id);
  memcpy(cell, value, get_type_size(column->type));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void add_row(struct Block ** pBlock)
{
  if (pBlock == NULL || *pBlock == NULL) { fprintf(stderr, "add_row called on a NULL block\n"); return; }
  
  set_num_rows(pBlock, (*pBlock)->num_rows + 1);
}

void add_row_with_data(struct Block ** pBlock, int num_columns, ...)
{
  if (pBlock == NULL || *pBlock == NULL) { fprintf(stderr, "add_row_with_data called on a NULL block\n"); return; }
  if ((*pBlock)->num_columns != num_columns) { fprintf(stderr, "block num_columns not the same as provided num_columns\n"); return; }
  
  add_row(pBlock);
  
  int row_id = (*pBlock)->num_rows - 1;
  int column_id;
  va_list v1;
  va_start(v1, num_columns);
  for (column_id = 0 ; column_id < (*pBlock)->num_columns ; column_id++)
  {
    struct Column * column = get_column(*pBlock, column_id);
    if (column->type == INT_TYPE)         { int32_t value = va_arg(v1, int);          set_cell(*pBlock, row_id, column_id, &value); }
    else if (column->type == LONG_TYPE)   { long value = va_arg(v1, long);            set_cell(*pBlock, row_id, column_id, &value); }
    else if (column->type == FLOAT_TYPE)  { float value = (float)va_arg(v1, double);  set_cell(*pBlock, row_id, column_id, &value); }
    else if (column->type == DOUBLE_TYPE) { double value = va_arg(v1, double);        set_cell(*pBlock, row_id, column_id, &value); }
    else if (column->type >= 5)           { char * value = va_arg(v1, char *);        set_cell(*pBlock, row_id, column_id, value); }
  }
  va_end(v1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void inspect_block(struct Block * block)
{
  if (block == NULL) { fprintf(stderr, "inspect_block called on a NULL block\n"); return; }
  fprintf(stderr, "\nblock (%d+%d+%d=%d total size in bytes - with %ld byte header)\n", block->attributes_bsize, block->columns_bsize, block->data_bsize, block->attributes_bsize + block->columns_bsize + block->data_bsize, block_header_size());
  
  if (block->num_attributes == 0)
    fprintf(stderr, "     ->no_attributes\n");
  else
  {
    fprintf(stderr, "     ->attributes = [\n");
    int32_t * offset = (int32_t*)((char*)block + sizeof(struct Block));
    
    int attribute_id;
    for (attribute_id = 0 ; attribute_id < block->num_attributes ;  attribute_id++)
    {
      struct Attribute * attribute = get_attribute(block, attribute_id);
      char * name = attribute_get_name(attribute);
      char * value = attribute_get_value(attribute);
      if (attribute->type == INT_TYPE)         fprintf(stderr, "       [%d][%7s] \"%s\" = %d\n", attribute_id, get_type_name(attribute->type), name, *(int32_t*)value);
      else if (attribute->type == LONG_TYPE)   fprintf(stderr, "       [%d][%7s] \"%s\" = %ld\n", attribute_id, get_type_name(attribute->type), name, *(long*)value);
      else if (attribute->type == FLOAT_TYPE)  fprintf(stderr, "       [%d][%7s] \"%s\" = %f\n", attribute_id, get_type_name(attribute->type), name, *(float*)value);
      else if (attribute->type == DOUBLE_TYPE) fprintf(stderr, "       [%d][%7s] \"%s\" = %lf\n", attribute_id, get_type_name(attribute->type), name, *(double*)value);
      else if (attribute->type == STRING_TYPE) fprintf(stderr, "       [%d][%7s] \"%s\" = \"%s\"\n", attribute_id, get_type_name(attribute->type), name, (char*)value);
      else fprintf(stderr, "       [%d] has invalid attribute->type (%x)\n", attribute_id, attribute->type);
    }
    fprintf(stderr, "     ]\n");
  }
  
  if (block->num_columns == 0)
    fprintf(stderr, "     ->no_fields\n");
  else
  {
    fprintf(stderr, "     ->columns = [\n");
    int column_id;
    for (column_id = 0 ; column_id < block->num_columns ; column_id++)
    {
      struct Column * column = get_column(block, column_id);
      fprintf(stderr, "       [%d][%7s] = \"%s\"\n", column_id, get_type_name(column->type), column_get_name(column));
    }
    fprintf(stderr, "     ]\n");
  }
  
  if (block->num_rows == 0)
    fprintf(stderr, "     ->no_data\n");
  else
  {
    fprintf(stderr, "     ->data = [\n");
    int row_id;
    for (row_id = 0 ; row_id < block->num_rows ; row_id++)
    {
      fprintf(stderr, "       [%d] = { ", row_id);
      
      int column_id;
      for (column_id = 0 ; column_id < block->num_columns ; column_id++)
      {
        struct Column * column = get_column(block, column_id);
        if (column == NULL) continue;
        void * cell = get_cell(block, row_id, column_id);
        if (column_id != 0) fprintf(stderr, ", ");
        
        if (cell == NULL) fprintf(stderr, "NULL");
        else if (column->type == INT_TYPE)    fprintf(stderr, "%d", *(int32_t*)cell);
        else if (column->type == LONG_TYPE)   fprintf(stderr, "%ld", *(long*)cell);
        else if (column->type == FLOAT_TYPE)  fprintf(stderr, "%f", *(float*)cell);
        else if (column->type == DOUBLE_TYPE) fprintf(stderr, "%lf", *(double*)cell);
        else if (column->type >= 5)           fprintf(stderr, "\"%s\"", (char*)cell);
      }
      
      if (row_id >= 25 && row_id < block->num_rows - 5) { fprintf(stderr, " }\n       ....\n"); row_id = block->num_rows-5; continue; }
      fprintf(stderr, " }\n");
    }
    fprintf(stderr, "     ]\n");
  }
  fprintf(stderr, "\n");
}

char * get_type_name(int32_t type)
{
  if (type >= 0 && type < sizeof(block_type_names) / sizeof(block_type_names[0]))
    return block_type_names[type];
  else
  {
    static char get_type_name_temp[100] = "";
    sprintf(get_type_name_temp, "%d", type);
    return get_type_name_temp;
  }
}
