
#include "block.h"
#include "libxml/xmlreader.h"
#include <string.h>

struct Block * block = NULL;
int shape_row_id = 0;
char shape_name[1000] = "";
int name_column_id = -1;
char shape_style[1000] = "";
int style_column_id = -1;
char shape_description[1000] = "";
int description_column_id = -1;

struct Style {
	char styleUrl[10];
	char lineStyleColor[10];
	int lineStyleWidth;
	char polyStyleColor[10];
	int polyStyleFill;
	int polyStyleOutline;
};

struct Style * styles = NULL;
int num_styles = 0;

void add_style(xmlTextReaderPtr reader) {
	if (num_styles == 1) return;
	num_styles++;
	styles = (struct Style *)realloc(styles, sizeof(struct Style)*num_styles);
	memset(&styles[num_styles-1], 0, sizeof(struct Style));
	
	struct Style * style = &styles[num_styles-1];
	
	while (xmlTextReaderMoveToNextAttribute(reader)) {
		char * attr_name = xmlTextReaderName(reader);
		char * attr_value = xmlTextReaderValue(reader);
		
		if (strcmp(attr_name, "id") == 0) {
			strncpy(style->styleUrl, attr_value, sizeof(style->styleUrl));
			break;
		}
	}
	xmlTextReaderMoveToElement(reader);
	//fprintf(stderr, "%d: %s\n", num_styles, style->styleUrl);
	
	int ret = 1;
	while (ret == 1) {
		const xmlChar * tagName = xmlTextReaderConstName(reader);
		const xmlChar * tagValue = xmlTextReaderConstValue(reader);
		if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 2 && strcmp(tagName, "Style")==0) {
			return;
		} else if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 3 && strcmp(tagName, "LineStyle")==0) {
			while (!(xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 3 && strcmp(tagName, "LineStyle")==0)) {
				if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {
					//strncpy(shape_description, xmlTextReaderConstValue(reader), sizeof(shape_description));
				}
				ret = xmlTextReaderRead(reader);
			}
		} else if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 3 && strcmp(tagName, "PolyStyle")==0) {
			while (!(xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 3 && strcmp(tagName, "PolyStyle")==0)) {
				if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {
					//strncpy(shape_description, xmlTextReaderConstValue(reader), sizeof(shape_description));
				}
				ret = xmlTextReaderRead(reader);
			}
		}
		ret = xmlTextReaderRead(reader);
	}
}

struct Block * add_placemark(struct Block * block, xmlTextReaderPtr reader) {
	
	name_column_id = get_column_id_by_name(block, "name");
	style_column_id = get_column_id_by_name(block, "style");
	description_column_id = get_column_id_by_name(block, "description");
	
	int ret = 1;
	
	if (reader != NULL) {
		while (ret == 1) {
			const xmlChar * tagName = xmlTextReaderConstName(reader);
			const xmlChar * tagValue = xmlTextReaderConstValue(reader);
			//fprintf(stderr, " %s %d %d\n", name, xmlTextReaderNodeType(reader), xmlTextReaderDepth(reader));
			if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 2 && strcmp(tagName, "Placemark")==0) {
				return block;
			} else if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 3 && strcmp(tagName, "name")==0) {
				while (!(xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 3 && strcmp(tagName, "name")==0)) {
					if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {
						strncpy(shape_name, xmlTextReaderConstValue(reader), sizeof(shape_name));
					}
					ret = xmlTextReaderRead(reader);
				}
			} else if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 3 && strcmp(tagName, "styleUrl")==0) {
				while (!(xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 3 && strcmp(tagName, "styleUrl")==0)) {
					if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {
						strncpy(shape_style, xmlTextReaderConstValue(reader), sizeof(shape_style));
					}
					ret = xmlTextReaderRead(reader);
				}
			} else if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 3 && strcmp(tagName, "description")==0) {
				while (!(xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 3 && strcmp(tagName, "description")==0)) {
					if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {
						strncpy(shape_description, xmlTextReaderConstValue(reader), sizeof(shape_description));
					}
					ret = xmlTextReaderRead(reader);
				}
			} else if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 6 && strcmp(tagName, "coordinates")==0) {
				shape_row_id++;
				while (!(xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 6 && strcmp(tagName, "coordinates")==0)) {
					if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {
						tagValue = xmlTextReaderConstValue(reader);
						//fprintf(stderr, "%s", value);
						char * ptr = strtok((char*)tagValue, "\n");
						while (ptr != NULL) {
							block = add_row_and_blank(block);
							int row_id = block->num_rows-1;
							float x, y, z;
							sscanf(ptr, "%f,%f,%f", &x, &y, &z);
							set_shape_part(block, row_id, shape_row_id, 5);
							set_xyz(block, row_id, x, y, z);
							if (name_column_id != -1) {
								set_cell_from_string(block, row_id, name_column_id, shape_name);
							}
							if (style_column_id != -1) {
								set_cell_from_string(block, row_id, style_column_id, shape_style);
							}
							if (description_column_id != -1) {
								set_cell_from_string(block, row_id, description_column_id, shape_description);
							}
							//set_cell_from_int32(block, block->num_rows-1, row_id_column_id, shape_row_id);
							//set_cell_from_int32(block, block->num_rows-1, part_type_column_id, 5);
							//set_cell_from_double(block, block->num_rows-1, x_column_id, x);
							//set_cell_from_double(block, block->num_rows-1, y_column_id, y);
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
	block = add_shape_columns(block);
	block = add_rgba_columns(block);
	block = add_string_column_with_length(block, "name", 20);
	block = add_string_column_with_length(block, "style", 20);
	block = add_string_column_with_length(block, "description", 80);
	
	reader = xmlReaderForFile(filename, NULL, 0);
	if (reader != NULL) {
		ret = xmlTextReaderRead(reader);
		while (ret == 1) {
			const xmlChar * name = xmlTextReaderConstName(reader);
			
			if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 2 && strcmp(name, "Style")==0) {
				add_style(reader);
			} else if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 2 && strcmp(name, "Placemark")==0) {
				block = add_placemark(block, reader);
			}
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
















