
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libxml/xmlreader.h>

#include "block.h"

struct MemoryStruct {
	char *memory;
	size_t size;
};

struct Node {
	char tagName[20];
	char * text;
	struct Node * parent;
	struct Node ** children;
	int num_children;
	char ** attrNames;
	char ** attrValues;
	int num_attr;
};

char * get_node_attr_value(struct Node * node, char * attrName) {
	int i;
	for (i = 0 ; i < node->num_attr ; i++) {
		if (strcmp(node->attrNames[i], attrName)==0) {
			return node->attrValues[i];
		}
	}
	return NULL;
}

void add_child(struct Node * node, struct Node * child) {
	node->num_children++;
	node->children = (struct Node **)realloc(node->children, sizeof(struct Node*)*node->num_children);
	node->children[node->num_children-1] = child;
	node->children[node->num_children-1]->parent = node;
}

void reverse_transform(struct Node * node, double * x, double * y) {
	
	//fprintf(stderr, "reverse_transform (%f, %f)\n", *x, *y);
	while (node != NULL) {
		
		char * transform = get_node_attr_value(node, "transform");
		
		if (transform != NULL) {
			char * temp = malloc(strlen(transform)+1);
			strncpy(temp, transform, strlen(transform));
			
			if (strncmp(temp, "matrix(", 7)==0) { // matrix(1.3333333 0 0 1.3333333 701.58727 132.15322)
				char * ptr = &temp[7];
				double matrix[6] = {0,0,0,0,0,0};
				matrix[0] = atof(ptr); ptr = strstr(ptr, " ")+1; if (ptr == NULL) { fprintf(stderr, "%s: failure parsing tranform '%s' on node '%s'\n", __func__, transform, node->tagName); return; }
				matrix[1] = atof(ptr); ptr = strstr(ptr, " ")+1; if (ptr == NULL) { fprintf(stderr, "%s: failure parsing tranform '%s' on node '%s'\n", __func__, transform, node->tagName); return; }
				matrix[2] = atof(ptr); ptr = strstr(ptr, " ")+1; if (ptr == NULL) { fprintf(stderr, "%s: failure parsing tranform '%s' on node '%s'\n", __func__, transform, node->tagName); return; }
				matrix[3] = atof(ptr); ptr = strstr(ptr, " ")+1; if (ptr == NULL) { fprintf(stderr, "%s: failure parsing tranform '%s' on node '%s'\n", __func__, transform, node->tagName); return; }
				matrix[4] = atof(ptr); ptr = strstr(ptr, " ")+1; if (ptr == NULL) { fprintf(stderr, "%s: failure parsing tranform '%s' on node '%s'\n", __func__, transform, node->tagName); return; }
				matrix[5] = atof(ptr);
				(*x) *= matrix[0];
				(*y) *= matrix[3];
				(*x) += matrix[4];
				(*y) += matrix[5];
				
				//fprintf(stderr, "matrix %f %f %f %f %f %f\n", matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5]);
			} else if (strncmp(temp, "scale(", 6)==0) {
				char * ptr = &temp[6];
				double scale[2] = {0,0};
				scale[0] = atof(ptr); ptr = strstr(ptr, ",")+1; if (ptr == NULL) { fprintf(stderr, "%s: failure parsing tranform '%s' on node '%s'\n", __func__, transform, node->tagName); return; }
				scale[1] = atof(ptr);
				(*x) *= scale[0];
				(*y) *= scale[1];
				//fprintf(stderr, "scale %f %f\n", scale[0], scale[1]);
			}
			free(temp);
		}
		
		node = node->parent;
	}
}

struct Node * new_node(xmlTextReaderPtr reader) {
	
	int nodeType = xmlTextReaderNodeType(reader);
	int depth = xmlTextReaderDepth(reader);
	if (nodeType == XML_READER_TYPE_ELEMENT || nodeType == XML_READER_TYPE_DOCUMENT_TYPE) {
		
		//int i;
		//for (i = 0 ; i < depth ; i++) fprintf(stderr, "  ");
		//fprintf(stderr, "%d: %d %s\n", depth, nodeType, xmlTextReaderConstName(reader));
		
		struct Node * node = (struct Node *)malloc(sizeof(struct Node));
		memset(node, 0, sizeof(struct Node));
		
		const xmlChar * tagName = xmlTextReaderConstName(reader);
		if (tagName != NULL) {
			strncpy(node->tagName, tagName, 20);
		}
		//fprintf(stderr, "%d: %s\n", depth, tagName);
		
		while (xmlTextReaderMoveToNextAttribute(reader)) {
			char * attr_name = xmlTextReaderName(reader);
			char * attr_value = xmlTextReaderValue(reader);
			
			if (attr_name != NULL && attr_value != NULL) {
				node->num_attr++;
				node->attrNames = (char**)realloc(node->attrNames, sizeof(char*)*node->num_attr);
				node->attrValues = (char**)realloc(node->attrValues, sizeof(char*)*node->num_attr);
				node->attrNames[node->num_attr-1] = (char*)malloc(strlen(attr_name)+1);
				strcpy(node->attrNames[node->num_attr-1], attr_name);
				node->attrValues[node->num_attr-1] = (char*)malloc(strlen(attr_value)+1);
				strcpy(node->attrValues[node->num_attr-1], attr_value);
			}
		}
		xmlTextReaderMoveToElement(reader);
		
		if (xmlTextReaderIsEmptyElement(reader)) {
			return node;
		}
		
		int ret = xmlTextReaderRead(reader);
		if (ret == 0) {
			return NULL;
		}
		
		while (xmlTextReaderNodeType(reader) != XML_READER_TYPE_END_ELEMENT || 
					 xmlTextReaderDepth(reader) > depth || strcmp(xmlTextReaderConstName(reader), tagName) != 0) {
			if (ret != 0) {
				struct Node * child = new_node(reader);
				if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_TEXT && child == NULL) {
					const char * text = xmlTextReaderConstValue(reader);
					
					child = (struct Node *)malloc(sizeof(struct Node));
					memset(child, 0, sizeof(struct Node));
					sprintf(child->tagName, "#text");
					
					int length = strlen(text);
					//fprintf(stderr, "%s: %d\n", text, length);
					child->text = (char*)malloc(length+1);
					strncpy(child->text, text, length);
				}
				if (child != NULL) {
					add_child(node, child);
				}
			} else {
				return NULL;
			}
			ret = xmlTextReaderRead(reader);
		}
		
		return node;
	}
	return NULL;
}

struct Block * append_rect_to_block(struct Node * node, struct Block * block) {
	int shape_row_id = 1;
	if (block->num_rows > 0) shape_row_id = get_cell_as_int32(block, block->num_rows-1, get_column_id_by_name(block, "shape_row_id")) + 1;
	
	if (get_node_attr_value(node, "x") == NULL) { fprintf(stderr, "%s expects rect node to have x attribute\n", __func__); return block; }
	if (get_node_attr_value(node, "y") == NULL) { fprintf(stderr, "%s expects rect node to have y attribute\n", __func__); return block; }
	if (get_node_attr_value(node, "width") == NULL) { fprintf(stderr, "%s expects rect node to have width attribute\n", __func__); return block; }
	if (get_node_attr_value(node, "height") == NULL) { fprintf(stderr, "%s expects rect node to have height attribute\n", __func__); return block; }
	
	double left = atof(get_node_attr_value(node, "x"));
	double bottom = atof(get_node_attr_value(node, "y"));
	double right = left + atof(get_node_attr_value(node, "width"));
	double top = bottom + atof(get_node_attr_value(node, "height"));
	
	reverse_transform(node, &left, &bottom); // traverses up the xml to apply scales and tranforms, rotates don't work
	reverse_transform(node, &right, &top); // traverses up the xml to apply scales and tranforms, rotates don't work
	
	char * fill_char = get_node_attr_value(node, "fill");
	char * fill_opacity_char = get_node_attr_value(node, "fill-opacity");
	
	double red = 0, green = 0, blue = 0;
	if (fill_char != NULL) {
		int red_i = 0, green_i = 0, blue_i = 0;
		sscanf(&fill_char[1], "%2x%2x%2x", &red_i, &green_i, &blue_i);
		red   = 128;//red_i / 255.0;
		green = 128;//red_i / 255.0;
		blue  = 128;//red_i / 255.0;
	}
	
	double alpha = 1;
	if (fill_opacity_char != NULL) {
		alpha = atof(fill_opacity_char);
	}
	
	int shape_row_id_column_id = get_column_id_by_name(block, "shape_row_id");
	int shape_part_id_column_id = get_column_id_by_name(block, "shape_part_id");
	int shape_part_type_column_id = get_column_id_by_name(block, "shape_part_type");
	int tagName_column_id = get_column_id_by_name(block, "tagName");
	
	int i;
	for (i = 0 ; i < 4 ; i++)
	{
		block = add_row_and_blank(block);
		set_rgba(block, block->num_rows-1, red, green, blue, alpha);
		set_cell_from_int32(block, block->num_rows-1, shape_row_id_column_id, shape_row_id);
		set_cell_from_int32(block, block->num_rows-1, shape_part_id_column_id, 1);
		set_cell_from_int32(block, block->num_rows-1, shape_part_type_column_id, 7); // GL_QUADS
		set_cell_from_string(block, block->num_rows-1, tagName_column_id, node->tagName);
	}
	set_xy(block, block->num_rows-4, left, bottom);
	set_xy(block, block->num_rows-3, right, bottom);
	set_xy(block, block->num_rows-2, right, top);
	set_xy(block, block->num_rows-1, left, top);
	
	return block;
}

struct Block * append_path_to_block(struct Node * node, char * path_string, struct Block * block) {
	
	int shape_row_id_column_id = get_column_id_by_name(block, "shape_row_id");
	if (shape_row_id_column_id == -1) return block;
	
	int shape_part_id_column_id = get_column_id_by_name(block, "shape_part_id");
	if (shape_part_id_column_id == -1) return block;
	
	int shape_part_type_column_id = get_column_id_by_name(block, "shape_part_type");
	if (shape_part_type_column_id == -1) return block;
	
	int tagName_column_id = get_column_id_by_name(block, "tagName");
	if (tagName_column_id == -1) return block;
	
	int shape_row_id = 1;
	if (block->num_rows > 0) {
		shape_row_id = get_cell_as_int32(block, block->num_rows-1, shape_row_id_column_id) + 1;
	}
	
	double red = 0, green = 0, blue = 0, alpha = 1;
	
	char * fill_char = get_node_attr_value(node, "fill");
	char * fill_opacity_char = get_node_attr_value(node, "fill-opacity");
	//fprintf(stderr, "tag = %s, fill_char = %s\n", node->tagName, fill_char);
	
	if (fill_char != NULL && strlen(fill_char)==7) {
		int red_i = 0, green_i = 0, blue_i = 0;
		sscanf(&fill_char[1], "%2x%2x%2x", &red_i, &green_i, &blue_i);
		red = red_i / 255.0;
		green = red_i / 255.0;
		blue = red_i / 255.0;
		//fprintf(stderr, "fill_char = %s\n", fill_char);
	}
	if (fill_opacity_char != NULL) {
		alpha = atof(fill_opacity_char);
	}
	
	if (fill_char == NULL || strcmp(fill_char, "none")==0) {
		char * stroke_char = get_node_attr_value(node, "stroke");
		char * stroke_opacity_char = get_node_attr_value(node, "stroke-opacity");
		
		if (stroke_char != NULL && strlen(stroke_char)==7) {
			int red_i, green_i, blue_i;
			sscanf(&stroke_char[1], "%2x%2x%2x", &red_i, &green_i, &blue_i);
			red = red_i / 255.0;
			green = green_i / 255.0;
			blue = blue_i / 255.0;
		}
		if (stroke_opacity_char != NULL) {
			alpha = atof(stroke_opacity_char);
		}
	}
	
	int shape_start = block->num_rows;
	
	int shape_part_id = 1;
	char * ptr = strtok(path_string, " ");
	while (ptr != NULL)
	{
		//srand(time(NULL));
		//red = rand() / (float)RAND_MAX;	 // kbfu
		//green = rand() / (float)RAND_MAX; // kbfu
		//blue = rand() / (float)RAND_MAX;	// kbfu
		// 713.5 239.5
		
		double coord[4] = { 0, 0, 0, 0 };
		int add_point = 0;
		
		char err[5]; err[0] = ptr[0]; err[1] = 0;
		
		switch (ptr[0]) {
			case 'M': case 'm':
				shape_start = block->num_rows;
			case 'L': case 'l':
				coord[0] = atof(&ptr[1]);
				ptr = strtok(NULL, " "); if (ptr == NULL) { fprintf(stderr, "%s failure at line %d\n", __FILE__, __LINE__); exit(0); }
				coord[1] = atof(&ptr[0]);
				add_point = 1;
				reverse_transform(node, &coord[0], &coord[1]);
				break;
			case 'Z': case 'z':
				coord[0] = get_x(block, shape_start);
				coord[1] = get_y(block, shape_start);
				add_point = 1;
				//reverse_transform(node, &coord[0], &coord[1]); // already did it
				//shape_part_id++; // this happens later
				break;
			case 'Q': case 'q':
				coord[0] = atof(&ptr[1]);
				ptr = strtok(NULL, " "); if (ptr == NULL) { fprintf(stderr, "%s failure at line %d\n", __FILE__, __LINE__); exit(0); }
				coord[1] = atof(&ptr[0]);
				ptr = strtok(NULL, " "); if (ptr == NULL) { fprintf(stderr, "%s failure at line %d\n", __FILE__, __LINE__); exit(0); }
				coord[2] = atof(&ptr[0]);
				ptr = strtok(NULL, " "); if (ptr == NULL) { fprintf(stderr, "%s failure at line %d\n", __FILE__, __LINE__); exit(0); }
				coord[3] = atof(&ptr[0]);
				break;
			case 'H': case 'h':
			case 'V': case 'b':
			case 'C': case 'c':
			case 'S': case 's':
			case 'T': case 't':
			case 'A': case 'a':
			{
				char err[5]; err[0] = ptr[0]; err[1] = 0;
				fprintf(stderr, "read_svg encountered unsupported path step '%s'\n", err);
				exit(0);
				break;
			}
			default:
			{
				char err[5]; err[0] = ptr[0]; err[1] = 0;
				fprintf(stderr, "read_svg failure at '%s'\n", err);
				exit(0);
			}
		}
		
		if (add_point) {
			block = add_row_and_blank(block);
			set_xy(block, block->num_rows-1, coord[0], coord[1]);
			set_rgba(block, block->num_rows-1, red, green, blue, alpha);
			//fprintf(stderr, "%f %f %f\n", red, green, blue);
			set_cell_from_int32(block, block->num_rows-1, shape_row_id_column_id, shape_row_id);
			set_cell_from_int32(block, block->num_rows-1, shape_part_id_column_id, shape_part_id);
			set_cell_from_int32(block, block->num_rows-1, shape_part_type_column_id, 5);
			set_cell_from_string(block, block->num_rows-1, tagName_column_id, node->tagName);
		}
		
		if (ptr[0] == 'Z' || ptr[0] == 'z') {
			shape_part_id++;
		}
		
		ptr = strtok(NULL, " ");
	}
	return block;
}

struct Block * append_node_to_block(int depth, struct Node * node, struct Block * block) {
	if (strcmp(node->tagName, "clipPath")==0) {
		//return block;
	}
	
	if (strcmp(node->tagName, "g")==0) {
		if (get_node_attr_value(node, "clip-path") == NULL) {
			//return block;
		}
	}
	
	if (strcmp(node->tagName, "path")==0) {
		if (get_node_attr_value(node, "fill") != NULL && strcmp(get_node_attr_value(node, "fill"), "none") != 0) {
			block = append_path_to_block(node, get_node_attr_value(node, "d"), block);
			//return block;
		}
		
		//char * colour = NULL;
		//if (get_node_attr_value(node, "clip-path") == NULL) 
		{// && strcmp(get_node_attr_value(node, "fill"), "none")!=0) {
			//block = append_path_to_block(node, get_node_attr_value(node, "d"), block);
		}
	}
	
	if (strcmp(node->tagName, "rect")==0) {
		block = append_rect_to_block(node, block);
	}
	
	if (strcmp(node->tagName, "tspan")==0) {
		char * x_list = get_node_attr_value(node, "x");
		
		int shape_row_id = get_new_shape_row_id(block);
		
		double x = 0, y = 0;
		if (x_list == NULL && strlen(x_list) > 0) {
		} else {
			int i = strlen(x_list) - 1;
			if (x_list[i] == ' ') i--;
			while (x_list[i] != ' ' && i >= 0) {
				i--;
			}
			i++;
			x += atof(&x_list[i]) / 2.7;
		}
		
		int text_column_id = -1;
		char * text = NULL;
		if (node->num_children == 1 || strcmp(node->children[0]->tagName, "#text")==0) {
			text = node->children[0]->text;
			if (text != NULL) {
				int length = strlen(text);
				text_column_id = get_column_id_by_name(block, "text");
				if (text_column_id == -1) {
					if (length < 50) length = 50;
					block = add_string_column_with_length_and_blank(block, "text", length);
					text_column_id = get_column_id_by_name(block, "text");
				}
			}
		}
		
		reverse_transform(node, &x, &y);
		block = add_row_and_blank(block);
		set_xy(block, block->num_rows-1, x+10, y);
		set_rgba(block, block->num_rows-1, 1, 0, 0, 1);
		set_cell_from_int32(block, block->num_rows-1, get_column_id_by_name(block, "shape_row_id"), shape_row_id);
		set_cell_from_int32(block, block->num_rows-1, get_column_id_by_name(block, "shape_part_id"), 1);
		set_cell_from_int32(block, block->num_rows-1, get_column_id_by_name(block, "shape_part_type"), 0); // GL_POINTS
		set_cell_from_string(block, block->num_rows-1, get_column_id_by_name(block, "tagName"), node->tagName);
		
		if (text_column_id != -1 && text != NULL) {
			set_cell_from_string(block, block->num_rows-1, text_column_id, text); // GL_POINTS
		}
		
		//fprintf(stderr, "text at %f %f\n", x, y);
	}
	if (strcmp(node->tagName, "circle")==0 || 
			strcmp(node->tagName, "ellipse")==0 || 
			strcmp(node->tagName, "line")==0 || 
			strcmp(node->tagName, "polyline")==0 || 
			strcmp(node->tagName, "polygon")==0 || 
			strcmp(node->tagName, "tref")==0 || 
			strcmp(node->tagName, "image")==0) {
		fprintf(stderr, "unsupported SVG tag '%s'\n", node->tagName);
	}
	
	int i;
	for (i = 0 ; i < node->num_children ; i++) {
		block = append_node_to_block(depth+1, node->children[i], block);
	}
	return block;
}

void free_node(struct Node * node) {
	int i;
	for (i = 0 ; i < node->num_attr ; i++) {
		free(node->attrNames[i]);
		free(node->attrValues[i]);
	}
	free(node->attrNames);
	free(node->attrValues);
	
	for (i = 0 ; i < node->num_children ; i++) {
		free_node(node->children[i]);
	}
	free(node->children);
	free(node->text);
	free(node);
}

int main(int argc, char ** argv)
{
	if (stdout_is_piped()) // other wise you don't see the seg fault
		setup_segfault_handling(argv);
	
	//assert_stdin_is_piped();
	assert_stdout_is_piped();
	//assert_stdin_or_out_is_piped();
	
	static char filename[1000] = "";
	static int debug = 0;
	static double x_offset = 0;
	static double y_offset = 0;
	static double x_multiple = 1;
	static double y_multiple = 1;
	
	int c;
	while (1)
	{
		static struct option long_options[] = {
			{"filename", required_argument, 0, 'f'},
			{"xoffset", required_argument, 0, 'a'},
			{"yoffset", required_argument, 0, 'b'},
			{"xmultiple", required_argument, 0, 'c'},
			{"ymultiple", required_argument, 0, 'd'},
			{"debug", no_argument, &debug, 1},
			{0, 0, 0, 0}
		};
		
		int option_index = 0;
		c = getopt_long(argc, argv, "f:a:b:c:d:", long_options, &option_index);
		if (c == -1) break;
		
		switch (c)
		{
			case 0: break;
			case 'f': strncpy(filename, optarg, sizeof(filename)); break;
			case 'a': x_offset = atof(optarg); break;
			case 'b': y_offset = atof(optarg); break;
			case 'c': x_multiple = atof(optarg); break;
			case 'd': y_multiple = atof(optarg); break;
			default: abort();
		}
	}
	
	struct MemoryStruct chunk;
	chunk.memory = NULL;
	chunk.size = 0;
	
	FILE * fp = filename[0] == 0 ? stdin : fopen(filename, "r");
	
	if (fp == NULL) {
		fprintf(stderr, "ERROR: file '%s' couldn't be opened for reading.\n", filename);
	}
	
	while ((c = fgetc(fp)) != EOF) {
		chunk.memory = realloc(chunk.memory, ++chunk.size);
		chunk.memory[chunk.size-1] = c;
	}
	fclose(fp);
	
	if (chunk.size == 0)
	{
		fprintf(stderr, "- received 0 byte response.\n");
	}
	else
	{
		xmlTextReaderPtr reader;
		
		//float matrix[6] = { 1, 0, 0, 1, 0, 0 }; // identity
		
		//struct Matrixs stack = { NULL, 0 };
		//push_matrix(&stack, "identity", 0);
		
		struct Block * block = new_block();
		block = add_command(block, argc, argv);
		block = add_int32_column(block, "shape_row_id");
		block = add_int32_column(block, "shape_part_id");
		block = add_int32_column(block, "shape_part_type");
		block = add_xy_columns(block);
		block = add_rgba_columns(block);
		block = add_string_column_with_length_and_blank(block, "tagName", 10);
		int shape_row_id = 0;
		int shape_start = 0;
		
		int shape_row_id_column_id = get_column_id_by_name(block, "shape_row_id");
		int shape_part_id_column_id = get_column_id_by_name(block, "shape_part_id");
		int shape_part_type_column_id = get_column_id_by_name(block, "shape_part_type");
		
		reader = xmlReaderForMemory(chunk.memory, chunk.size, NULL, NULL, (XML_PARSE_NOBLANKS | XML_PARSE_NOCDATA | XML_PARSE_NOERROR | XML_PARSE_NOWARNING));
		if (reader != NULL)
		{
			int ret = xmlTextReaderRead(reader);
			while (ret != 0 && strcmp(xmlTextReaderConstName(reader), "svg")!=0) {
				ret = xmlTextReaderRead(reader);
			}
			ret = xmlTextReaderRead(reader); // why is there two svg tags?  when thats not the case in the file, I just don't know.
			
			struct Node * root_svg_node = new_node(reader);
			//fprintf(stderr, "%s, %s\n", root_svg_node->tagName, get_node_attr_value(root_svg_node, "viewBox"));
			block = append_node_to_block(0, root_svg_node, block);
			
			free_node(root_svg_node);
			
			write_block(stdout, block);
			free_block(block);
			xmlFreeTextReader(reader);
			xmlCleanupParser();
		}
		free(chunk.memory);
	}
}
