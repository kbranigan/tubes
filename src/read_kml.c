
#include "block.h"
#include "libxml/xmlreader.h"

struct Block * block = NULL;
int row_id = 0;
char shape_name[1000] = "";

struct Block * add_placemark(struct Block * block, xmlTextReaderPtr reader) {
	
	int row_id_column_id = get_column_id_by_name(block, "shape_row_id");
	int part_type_column_id = get_column_id_by_name(block, "shape_part_type");
	int x_column_id = get_column_id_by_name(block, "x");
	int y_column_id = get_column_id_by_name(block, "y");
	
	int ret = 1;
	
	if (reader != NULL) {
		while (ret == 1) {
			const xmlChar * name = xmlTextReaderConstName(reader);
			const xmlChar * value = xmlTextReaderConstValue(reader);
			//fprintf(stderr, " %s %d %d\n", name, xmlTextReaderNodeType(reader), xmlTextReaderDepth(reader));
			if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 2 && strcmp(name, "Placemark")==0)
			{
				// end of Placemark yo
				return block;
			}
			else if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 3 && strcmp(name, "name")==0)
			{
				while (!(xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 3 && strcmp(name, "name")==0))
				{
					if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT)
					{
						value = xmlTextReaderConstValue(reader);
						strncpy(shape_name, value, sizeof(shape_name));
						// kbfu do something here
						//fprintf(stderr, "%s\n", value);
					}
					ret = xmlTextReaderRead(reader);
				}
			}
			else if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 6 && strcmp(name, "coordinates")==0)
			{
				row_id++;
				//fprintf(stderr, "  %d %s\n", xmlTextReaderDepth(reader), name);
				while (!(xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 6 && strcmp(name, "coordinates")==0))
				{
					if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT)
					{
						value = xmlTextReaderConstValue(reader);
						//fprintf(stderr, "%s", value);
						char * ptr = strtok((char*)value, "\n");
						while (ptr != NULL)
						{
							block = add_row(block);
							float x, y, z;
							sscanf(ptr, "%f,%f,%f", &x, &y, &z);
							set_cell_from_int32(block, block->num_rows-1, row_id_column_id, row_id);
							set_cell_from_int32(block, block->num_rows-1, part_type_column_id, 5);
							set_cell_from_double(block, block->num_rows-1, x_column_id, x);
							set_cell_from_double(block, block->num_rows-1, y_column_id, y);
							//fprintf(stderr, "%f %f %f\n", x, y, z);
							ptr = strtok(NULL, "\n");
						}
						//fprintf(stderr, "    %s\n", xmlTextReaderConstValue(reader));
					}
					ret = xmlTextReaderRead(reader);
				}
			}
			ret = xmlTextReaderRead(reader);
		}
		//ret = xmlTextReaderRead(reader);
	}
	return block;
}

int main(int argc, char ** argv) {
	
	if (stdout_is_piped()) // other wise you don't see the seg fault
		setup_segfault_handling(argv);
	
	//assert_stdin_is_piped();
	assert_stdout_is_piped();
	//assert_stdin_or_out_is_piped();
	
	char filename[1000];
	
	int c;
	while (1)
	{
		static struct option long_options[] = {
			{"file", required_argument, 0, 'f'},
			//{"drop", no_argument, &drop, 1},
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
	
	if (filename[0] == 0) { fprintf(stderr, "ERROR %s: filename not provided\n", argv[0]); return EXIT_FAILURE; }
	
	LIBXML_TEST_VERSION
	
	xmlTextReaderPtr reader;
	
	int ret;
	
	block = new_block();
	block = add_command(block, argc, argv);
	block = add_int32_column(block, "shape_row_id");
	block = add_int32_column(block, "shape_part_type");
	block = add_float_column(block, "x");
	block = add_float_column(block, "y");
	
	reader = xmlReaderForFile(filename, NULL, 0);
	if (reader != NULL) {
		ret = xmlTextReaderRead(reader);
		while (ret == 1) {
			const xmlChar * name = xmlTextReaderConstName(reader);
			const xmlChar * value = xmlTextReaderConstValue(reader);
			
			if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 2 && strcmp(name, "Placemark")==0)
			{
				//if (block->num_rows == 0) // kbfu
				{
					//fprintf(stderr, "add_placemark\n");
					//fprintf(stderr, "block->num_rows = %d\n", block->num_rows);
					block = add_placemark(block, reader);
					//fprintf(stderr, "block->num_rows = %d\n", block->num_rows);
				}
			}
			// xmlTextReaderDepth(reader),
			// xmlTextReaderNodeType(reader),
			// xmlTextReaderIsEmptyElement(reader),
			// xmlTextReaderHasValue(reader));
			ret = xmlTextReaderRead(reader);
		}
		xmlFreeTextReader(reader);
		if (ret != 0) {
			fprintf(stderr, "%s : failed to parse\n", filename);
		}
	} else {
		fprintf(stderr, "Unable to open %s\n", filename);
	}
	
	write_block(stdout, block);
	
	return EXIT_SUCCESS;
}
















