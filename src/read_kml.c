
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
	float lineStyleColor4f[4];
	int lineStyleWidth;
	char polyStyleColor[10];
	float polyStyleColor4f[4];
	int polyStyleFill;
	int polyStyleOutline;
};

struct Style * styles = NULL;
int num_styles = 0;

void add_style(xmlTextReaderPtr reader) {
	//if (num_styles == 1) return;
	num_styles++;
	styles = (struct Style *)realloc(styles, sizeof(struct Style)*num_styles);
	memset(&styles[num_styles-1], 0, sizeof(struct Style));
	
	struct Style * style = &styles[num_styles-1];
	
	while (xmlTextReaderMoveToNextAttribute(reader)) {
		if (strcmp(xmlTextReaderName(reader), "id") == 0) {
			strncpy(style->styleUrl, xmlTextReaderValue(reader), sizeof(style->styleUrl));
			break;
		}
	}
	xmlTextReaderMoveToElement(reader);
	
	//fprintf(stderr, "%d: %s\n", num_styles, style->styleUrl);
	
	char tagName[100] = "";
	char tagValue[100] = "";
	
	int ret = 1;
	while (ret == 1) {
		const xmlChar * tempTagName = xmlTextReaderConstName(reader);
		const xmlChar * tempTagValue = xmlTextReaderConstValue(reader);
		if (tempTagName != NULL) strncpy(tagName, tempTagName, sizeof(tagName));
		if (tempTagValue != NULL) strncpy(tagValue, tempTagValue, sizeof(tagValue));
		if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 2 && strcmp(tagName, "Style")==0) {
			break;
		} else if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 3 && strcmp(tagName, "LineStyle")==0) {
			while (!(xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 3 && strcmp(tagName, "LineStyle")==0)) {
				
				if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 4 && strcmp(tagName, "color")==0) {
					while (!(xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 4 && strcmp(tagName, "color")==0)) {
						if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT) {
							const xmlChar * tempTagName = xmlTextReaderConstName(reader);
							if (tempTagName != NULL) strncpy(tagName, tempTagName, sizeof(tagName));
						}
						if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {
							strncpy(style->lineStyleColor, xmlTextReaderConstValue(reader), sizeof(style->lineStyleColor));
							int red_i = 0, green_i = 0, blue_i = 0, alpha_i = 0;
							sscanf(style->lineStyleColor, "%2x%2x%2x%2x", &alpha_i, &blue_i, &green_i, &red_i);
							style->lineStyleColor4f[0] = red_i / 255.0;
							style->lineStyleColor4f[1] = green_i / 255.0;
							style->lineStyleColor4f[2] = blue_i / 255.0;
							style->lineStyleColor4f[3] = alpha_i / 255.0;
						}
						ret = xmlTextReaderRead(reader);
						if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT || xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT) {
							const xmlChar * tempTagName = xmlTextReaderConstName(reader);
							if (tempTagName != NULL) strncpy(tagName, tempTagName, sizeof(tagName));
						}
					}
				} else if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 4 && strcmp(tagName, "width")==0) {
					while (!(xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 4 && strcmp(tagName, "width")==0)) {
						if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT) {
							const xmlChar * tempTagName = xmlTextReaderConstName(reader);
							if (tempTagName != NULL) strncpy(tagName, tempTagName, sizeof(tagName));
						}
						if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {
							style->lineStyleWidth = atoi(xmlTextReaderConstValue(reader));
						}
						ret = xmlTextReaderRead(reader);
						if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT || xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT) {
							const xmlChar * tempTagName = xmlTextReaderConstName(reader);
							if (tempTagName != NULL) strncpy(tagName, tempTagName, sizeof(tagName));
						}
					}
				}
				ret = xmlTextReaderRead(reader);
				if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT || xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT) {
					const xmlChar * tempTagName = xmlTextReaderConstName(reader);
					if (tempTagName != NULL) strncpy(tagName, tempTagName, sizeof(tagName));
				}
			}
		} else if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 3 && strcmp(tagName, "PolyStyle")==0) {
			while (!(xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 3 && strcmp(tagName, "PolyStyle")==0)) {
				
				if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 4 && strcmp(tagName, "color")==0) {
					while (!(xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 4 && strcmp(tagName, "color")==0)) {
						if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT) {
							const xmlChar * tempTagName = xmlTextReaderConstName(reader);
							if (tempTagName != NULL) strncpy(tagName, tempTagName, sizeof(tagName));
						}
						if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {
							strncpy(style->polyStyleColor, xmlTextReaderConstValue(reader), sizeof(style->polyStyleColor));
							int red_i = 0, green_i = 0, blue_i = 0, alpha_i = 0;
							sscanf(style->polyStyleColor, "%2x%2x%2x%2x", &alpha_i, &blue_i, &green_i, &red_i);
							style->polyStyleColor4f[0] = red_i / 255.0;
							style->polyStyleColor4f[1] = green_i / 255.0;
							style->polyStyleColor4f[2] = blue_i / 255.0;
							style->polyStyleColor4f[3] = alpha_i / 255.0;
						}
						ret = xmlTextReaderRead(reader);
						if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT || xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT) {
							const xmlChar * tempTagName = xmlTextReaderConstName(reader);
							if (tempTagName != NULL) strncpy(tagName, tempTagName, sizeof(tagName));
						}
					}
				} else if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 4 && strcmp(tagName, "outline")==0) {
					while (!(xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 4 && strcmp(tagName, "outline")==0)) {
						if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT) {
							const xmlChar * tempTagName = xmlTextReaderConstName(reader);
							if (tempTagName != NULL) strncpy(tagName, tempTagName, sizeof(tagName));
						}
						if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {
							style->polyStyleOutline = atoi(xmlTextReaderConstValue(reader));
						}
						ret = xmlTextReaderRead(reader);
						if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT || xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT) {
							const xmlChar * tempTagName = xmlTextReaderConstName(reader);
							if (tempTagName != NULL) strncpy(tagName, tempTagName, sizeof(tagName));
						}
					}
				} else if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 4 && strcmp(tagName, "fill")==0) {
					while (!(xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && xmlTextReaderDepth(reader) == 4 && strcmp(tagName, "fill")==0)) {
						if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT) {
							const xmlChar * tempTagName = xmlTextReaderConstName(reader);
							if (tempTagName != NULL) strncpy(tagName, tempTagName, sizeof(tagName));
						}
						if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT) {
							style->polyStyleFill = atoi(xmlTextReaderConstValue(reader));
						}
						ret = xmlTextReaderRead(reader);
						if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT || xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT) {
							const xmlChar * tempTagName = xmlTextReaderConstName(reader);
							if (tempTagName != NULL) strncpy(tagName, tempTagName, sizeof(tagName));
						}
					}
				}
				ret = xmlTextReaderRead(reader);
				if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT || xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT) {
					const xmlChar * tempTagName = xmlTextReaderConstName(reader);
					if (tempTagName != NULL) strncpy(tagName, tempTagName, sizeof(tagName));
				}
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
							set_shape_part(block, row_id, shape_row_id, 0, 5);
							set_xyz(block, row_id, x, y, z);
							if (name_column_id != -1) {
								set_cell_from_string(block, row_id, name_column_id, shape_name);
							}
							if (style_column_id != -1) {
								if (shape_style != NULL && shape_style[0] == '#') {
									set_cell_from_string(block, row_id, style_column_id, &shape_style[1]);
									int i;
									for (i = 0 ; i < num_styles ; i++) {
										if (strncmp(styles[i].styleUrl, &shape_style[1], sizeof(styles[i].styleUrl)) == 0) {
											set_rgba(block, row_id, styles[i].polyStyleColor4f[0], styles[i].polyStyleColor4f[1], styles[i].polyStyleColor4f[2], styles[i].polyStyleColor4f[3]);
											break;
										}
									}
								}
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
	
	char filename[1000] = "";
	
	struct Params * params = NULL;
	params = add_string_param(params, "filename", 'f', filename, 1);
	eval_params(params, argc, argv);
	
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
















