
#include <stdio.h>
#include <stdlib.h>

#include "../ext/shapefil.h"

#include "block.h"

int main(int argc, char ** argv)
{
	if (stdout_is_piped()) // other wise you don't see the seg fault
		setup_segfault_handling(argv);
	
	assert_stdin_is_piped();
	//assert_stdout_is_piped();
	//assert_stdin_or_out_is_piped();
	
	static char filename[1000] = "";
	
	static int debug = 0;
	
	int c;
	while (1)
	{
		static struct option long_options[] = {
			{"filename", required_argument, 0, 'f'},
			{"debug", no_argument, &debug, 1},
			{0, 0, 0, 0}
		};
		
		int option_index = 0;
		c = getopt_long(argc, argv, "f:", long_options, &option_index);
		if (c == -1) break;
		
		switch (c)
		{
			case 0: break;
			case 'f': strncpy(filename, optarg, sizeof(filename)); break;
			default: abort();
		}
	}
	
	if (filename[0] == 0 && argc == 2 && argv[1] != NULL)
		strncpy(filename, argv[1], sizeof(filename));
	
	if (filename[0] == 0)
	{
		fprintf(stderr, "output filename not given, using 'output.dbf'\n");
		strcpy(filename, "output.dbf");
	}
	
	DBFHandle dbf = DBFCreate(filename);
	SHPHandle shp = SHPCreate(filename, SHPT_POLYGON);
	
	if (dbf == NULL)// && shp == NULL)
	{
		fprintf(stderr, "%s: Error, neither dbf or shp file found\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	struct Block * block = NULL;
	while ((block = read_block(stdin)))
	{
		int shape_row_id_column_id = get_column_id_by_name(block, "shape_row_id");
		int shape_part_id_column_id = get_column_id_by_name(block, "shape_part_id");
		int shape_part_type_column_id = get_column_id_by_name(block, "shape_part_type");
		int x_column_id = get_column_id_by_name(block, "x");
		int y_column_id = get_column_id_by_name(block, "y");
		
		if (DBFGetFieldCount(dbf) == 0)
		{
			int i;
			for (i = 0 ; i < block->num_columns ; i++)
			{
				struct Column * column = get_column(block, i);
				char * column_name = column_get_name(column);
				if (strcmp(column_name, "shape_row_id") == 0) continue;
				if (strcmp(column_name, "shape_part_id") == 0) continue;
				if (strcmp(column_name, "shape_part_type") == 0) continue;
				if (strcmp(column_name, "x") == 0) continue;
				if (strcmp(column_name, "y") == 0) continue;
				
				fprintf(stderr, "DBFAddField(%d)(%s)\n", i, column_name);
				
				//if (column->type == TYPE_INT || column->type == TYPE_UINT)
				//	DBFAddField(dbf, column_name, FTInteger, (column->bsize == 4 ? 11 : 21), 0);
				//else 
				if (column->type == TYPE_FLOAT)
				{
					DBFAddField(dbf, column_name, FTDouble, 11, 4);
				}
				/*else if (column->type == TYPE_CHAR)
					DBFAddField(dbf, column_name, FTString, column->bsize, 0);
				else
				{
					fprintf(stderr, "Failure with column type %d.\n", column->type);
				}*/
				break;
			}
		}
		
		int shape_start_id = 0, shape_end_id;
		while ((shape_end_id = get_next_shape_start(block, shape_start_id)))
		{
			int shape_row_id = get_cell_as_int32(block, shape_start_id, shape_row_id_column_id);
			if (shape_row_id == 212) // kbfu
			{
				fprintf(stderr, "shape = %d to %d (#%d)\n", shape_start_id, shape_end_id, get_cell_as_int32(block, shape_start_id, shape_row_id_column_id));
				
				int nParts = 0;
				int nVertices = shape_end_id - shape_start_id;
				int * panPartStart = NULL;
				double * padfX = malloc(sizeof(double)*nVertices);
				double * padfY = malloc(sizeof(double)*nVertices);
				
				int i;
				for (i = shape_start_id ; i < shape_end_id ; i++)
				{
					padfX[i - shape_start_id] = get_x(block, i);
					padfY[i - shape_start_id] = get_y(block, i);
				}
				
				int part_start_id = shape_start_id, part_end_id;
				while ((part_end_id = get_next_part_start(block, part_start_id)))
				{
					nParts++;
					panPartStart = realloc(panPartStart, nParts*sizeof(int));
					panPartStart[nParts-1] = part_start_id - shape_start_id;
					
					fprintf(stderr, "  part = %d to %d (#%d)\n", part_start_id, part_end_id, get_cell_as_int32(block, part_start_id, shape_part_id_column_id));
					if (part_end_id == shape_end_id) break;
					part_start_id = part_end_id;
				}
				
				fprintf(stderr, "shape_row_id = %d\n", shape_row_id);
				fprintf(stderr, "nVertices = %d\n", nVertices);
				fprintf(stderr, "nParts = %d\n", nParts);
				for (i = 0 ; i < nParts ; i++)
					fprintf(stderr, " %d: %d\n", i, panPartStart[i]);
				
				SHPObject * shpObject = SHPCreateObject(
						SHPT_POLYGON,
						0,
						nParts,
						panPartStart,
						NULL, //panPartType
						shape_end_id - shape_start_id,
						padfX,
						padfY,
						NULL, //padfZ
						NULL //padfM
					);
				fprintf(stderr, "hi\n");
				SHPWriteObject(shp, -1, shpObject);
				fprintf(stderr, "hi\n");
				
				int ret = 0;// DBFWriteDoubleAttribute(dbf, 0, 5, 1.5);
				fprintf(stderr, "ret = %d\n", ret);
				
				fprintf(stderr, "shpObject->nShapeId = %d\n", shpObject->nShapeId);
				
				int nEntities = 0;
				SHPGetInfo(shp, &nEntities, NULL, NULL, NULL);
				fprintf(stderr, "nEntities = %d\n", nEntities);
				
				//SHPDestroyObject(shpObject);
				free(padfX);
				free(padfY);
				free(panPartStart);
			}
			
			if (shape_end_id == block->num_rows) break;
			shape_start_id = shape_end_id;
		}
	}
	
	if (dbf) DBFClose(dbf);
	if (shp) SHPClose(shp);
}









