
#include "block.h"
#include <math.h>

// copied from read_block
struct Block * read_old_block(FILE * fp)
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
  
  block->attributes_bsize += sizeof(int32_t)*block->num_attributes;
  block->columns_bsize += sizeof(int32_t)*2*block->num_columns;
  
  block = realloc_block(block);
  
  if (fread((char*)block + sizeof(struct Block) + sizeof(int32_t)*block->num_attributes,
      block->attributes_bsize - sizeof(int32_t)*block->num_attributes, 1, fp) != 1) { fprintf(stderr, "fread block failed (error 2)\n"); return NULL; }
  
  if (fread((char*)block + sizeof(struct Block) + block->attributes_bsize + sizeof(int32_t)*2*block->num_columns,
      block->columns_bsize - sizeof(int32_t)*2*block->num_columns, 1, fp) != 1) { fprintf(stderr, "fread block failed (error 2)\n"); return NULL; }
  
  if (fread((char*)block + sizeof(struct Block) + block->attributes_bsize + block->columns_bsize, 
      block->data_bsize, 1, fp) != 1) { fprintf(stderr, "fread block failed (error 2)\n"); return NULL; }
  
  int32_t * attribute_offsets = get_attribute_offsets(block);
  
  int i;
  struct Attribute * attribute = get_first_attribute(block);
  for (i = 0 ; i < block->num_attributes ; i++)
  {
    attribute_offsets[i] = (int32_t)((char*)attribute - (char*)block - sizeof(struct Block));
    attribute = get_next_attribute(block, attribute);
  }
  
  int32_t * column_offsets = get_column_offsets(block);
  int32_t * cell_offsets = get_cell_offsets(block);
  cell_offsets[0] = 0;
  
  int o = 0;
  struct Column * column = get_first_column(block);
  for (i = 0 ; i < block->num_columns ; i++)
  {
    cell_offsets[i] = o;
    o += get_type_size(column->type);
    column_offsets[i] = (int32_t)((char*)column - (char*)block - sizeof(struct Block) - block->attributes_bsize);
    column = get_next_column(block, column);
  }
  
  return block;
}


int main(int argc, char ** argv)
{
  if (stdout_is_piped()) // other wise you don't see the seg fault
    setup_segfault_handling(argv);
  
  assert_stdin_is_piped();
  assert_stdout_is_piped();
  
  struct Block * block = NULL;
  while ((block = read_old_block(stdin)))
  {
    fprintf(stderr, "attributes_bsize = %d\n", block->attributes_bsize);
    fprintf(stderr, "columns_bsize = %d\n", block->columns_bsize);
    fprintf(stderr, "data_bsize = %d\n", block->data_bsize);
    fprintf(stderr, "row_bsize = %d\n", block->row_bsize);
    fprintf(stderr, "num_attributes = %d\n", block->num_attributes);
    fprintf(stderr, "num_columns = %d\n", block->num_columns);
    fprintf(stderr, "num_rows = %d\n", block->num_rows);
    write_block(stdout, block);
    free_block(block);
  }
  
}