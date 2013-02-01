
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libxml/xpath.h>
#include <libxml/xmlreader.h>

#include "block.h"

struct MemoryStruct {
	char *memory;
	size_t size;
};

struct Node {
	char tagName[20];
	struct Node * parent;
	struct Node ** children;
	int num_children;
	char ** attrNames;
	char ** attrValues;
	int num_attr;
};

void add_child(struct Node * node, struct Node * child) {
	node->num_children++;
	node->children = realloc(node->children, sizeof(struct Node*)*node->num_children);
	node->children[node->num_children-1] = child;
	node->children[node->num_children-1]->parent = node;
}

struct Node * new_node(struct Node * parent, xmlTextReaderPtr reader) {
	if (xmlTextReaderRead(reader) == 0) return parent;
	
	const xmlChar * name = xmlTextReaderConstName(reader);
	const xmlChar * value = xmlTextReaderConstValue(reader);
	if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && strcmp(name, "g")==0)
	{
		while (xmlTextReaderMoveToNextAttribute(reader))
		{
			char * attr_name = xmlTextReaderName(reader);
			char * attr_value = xmlTextReaderValue(reader);
			if (strcmp(attr_name, "transform")==0)
			{
				//push_matrix(&stack, attr_value, xmlTextReaderDepth(reader));
			}
		}
		xmlTextReaderMoveToElement(reader);
	}
}

/*struct Matrix {
	double d[3][3];
	int xmldepth;
};

struct Matrixs {
	struct Matrix * matrixs;
	int num_matrixs;
};

void push_matrix(struct Matrixs * stack, char * str, int xmldepth) {
	if (stack == NULL || str == NULL)
	{
		fprintf(stderr, "push_matrix called with NULL stack\n");
		return;
	}
	
	stack->num_matrixs++;
	stack->matrixs = realloc(stack->matrixs, (stack->num_matrixs)*sizeof(struct Matrix));
	memset(&stack->matrixs[stack->num_matrixs-1], 0, sizeof(struct Matrix));
	struct Matrix * matrix = &stack->matrixs[stack->num_matrixs-1];
	matrix->xmldepth = xmldepth;
	
	//fprintf(stderr, "push %s\n", str);
	
	if (strncmp(str, "identity", 8)==0)
	{
		matrix->d[0][0] = 1;
		matrix->d[1][1] = 1;
		matrix->d[2][2] = 1;
	}
	else if (strncmp(str, "scale", 5)==0)
	{
		char * ptr = strtok(str, ", ");
		matrix->d[0][0] = atof(&ptr[6]);
		ptr = strtok(NULL, " "); matrix->d[1][1] = atof(ptr);
		//fprintf(stderr, "scale (%f %f) IGNORED\n", matrix->d[0], matrix->d[1]);
	}
	else if (strncmp(str, "matrix", 6)==0)
	{
		char * ptr = strtok(str, ", ");
		matrix->d[0][0] = atof(&ptr[7]);
		ptr = strtok(NULL, " "); matrix->d[0][1] = atof(ptr);
		ptr = strtok(NULL, " "); matrix->d[1][0] = atof(ptr);
		ptr = strtok(NULL, " "); matrix->d[1][1] = atof(ptr);
		ptr = strtok(NULL, " "); matrix->d[2][0] = atof(ptr);
		ptr = strtok(NULL, " "); matrix->d[2][1] = atof(ptr);
		//fprintf(stderr, "%f %f %f %f %f %f\n", matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5]);
	}
	else
	{
		fprintf(stderr, "push_matrix called with invalid string\n");
	}
}

void pop_matrix(struct Matrixs * stack, int xmldepth) {
	
	if (stack == NULL || stack->num_matrixs <= 0)
	{
		fprintf(stderr, "pop_matrix called with NULL or empty stack\n");
		return;
	}
	else if (stack->matrixs[stack->num_matrixs-1].xmldepth != xmldepth)
	{
		return;
	}
	else if (stack->num_matrixs == 1)
	{
		//fprintf(stderr, "pop last\n");
		stack->num_matrixs = 0;
		free(stack->matrixs);
		stack->matrixs = NULL;
	}
	else
	{
		//fprintf(stderr, "pop %d\n", stack->num_matrixs);
		stack->matrixs = realloc(stack->matrixs, (--stack->num_matrixs)*sizeof(struct Matrixs));
	}
}

void multi_matrixs(struct Matrix * product,
	 struct Matrix * matrix1, struct Matrix * matrix2)
{
	int x, y, z;
	for (x = 0 ; x < 3 ; ++x)
		for (y = 0 ; y < 3 ; ++y)
		{
			double sum = 0;
			for (z = 0 ; z < 3 ; ++z)
				sum += matrix1->d[x][z] * matrix2->d[z][y];
			product->d[x][y] = sum;
		}
}

void apply_matrixs(double * coord, struct Matrixs * stack)
{
	fprintf(stderr, "coord = {%f, %f}\n", coord[0], coord[1]);
	int i;
	
	struct Matrix all = { { {1,0,0}, {0,1,0}, {0,0,1} }, 0 };
	
	for (i = 0 ; i < stack->num_matrixs ; i++)
	{
		struct Matrix * matrix = &stack->matrixs[i];
		struct Matrix temp;
		memcpy(&temp, &all, sizeof(struct Matrix));
		multi_matrixs(&all, matrix, &temp);
		
		fprintf(stderr, " matrix = {%f,%f,%f},{%f,%f,%f}\n", 
			matrix->d[0][0], matrix->d[1][0], matrix->d[2][0],
			matrix->d[0][1], matrix->d[1][1], matrix->d[2][1]);
		//coord[0] *= matrix->d[0][0];
		//coord[1] *= matrix->d[1][1];
	}
	fprintf(stderr, " matrix = {%f,%f,%f},{%f,%f,%f}\n", 
		all.d[0][0], all.d[1][0], all.d[2][0],
		all.d[0][1], all.d[1][1], all.d[2][1]);
	
	double oldcoord[3] = { coord[0], coord[1], 0 };
	double newcoord[3] = { 0, 0, 0 };
	
	newcoord[0] = all.d[0][0] * coord[0] + all.d[0][1] * coord[1] + all.d[2][1];
	newcoord[1] = all.d[1][0] * coord[0] + all.d[1][1] * coord[1] + all.d[2][1];
	
	fprintf(stderr, "newcoord = {%f, %f}\n", newcoord[0], newcoord[1]);
	exit(1);
}*/

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
		block = add_rgb_columns(block);
		int shape_row_id = 0;
		int shape_start = 0;
		
		int shape_row_id_column_id = get_column_id_by_name(block, "shape_row_id");
		int shape_part_id_column_id = get_column_id_by_name(block, "shape_part_id");
		int shape_part_type_column_id = get_column_id_by_name(block, "shape_part_type");
		
		reader = xmlReaderForMemory(chunk.memory, chunk.size, NULL, NULL, (XML_PARSE_NOBLANKS | XML_PARSE_NOCDATA | XML_PARSE_NOERROR | XML_PARSE_NOWARNING));
		if (reader != NULL)
		{
			int ret = xmlTextReaderRead(reader);
			while (ret == 1)
			{
				const xmlChar * name = xmlTextReaderConstName(reader);
				const xmlChar * value = xmlTextReaderConstValue(reader);
				if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && strcmp(name, "g")==0)
				{
					while (xmlTextReaderMoveToNextAttribute(reader))
					{
						char * attr_name = xmlTextReaderName(reader);
						char * attr_value = xmlTextReaderValue(reader);
						if (strcmp(attr_name, "transform")==0)
						{
							//push_matrix(&stack, attr_value, xmlTextReaderDepth(reader));
						}
					}
					xmlTextReaderMoveToElement(reader);
				}
				/*else if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && strcmp(name, "clipPath")==0)
				{
					while (!((xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && strcmp(name, "clipPath")==0)))
						ret = xmlTextReaderRead(reader);
					//pop_matrix(&stack, xmlTextReaderDepth(reader) + 1);
				}*/
				else if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_END_ELEMENT && strcmp(name, "g")==0)
				{
					//pop_matrix(&stack, xmlTextReaderDepth(reader) + 1);
				}
				else if (shape_row_id < 5 && xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT && xmlTextReaderDepth(reader) == 7 && strcmp(name, "path")==0)
				{
					double red = 0, green = 0, blue = 0;
					
					// get color ( stroke="none" fill="#FFFF73" fill-rule="evenodd" fill-opacity="1" )
					while (xmlTextReaderMoveToNextAttribute(reader))
					{
						char * attr_name = xmlTextReaderName(reader);
						char * attr_value = xmlTextReaderValue(reader);
						if (strcmp(attr_name, "fill")==0)
						{
							int red_i = 0, green_i = 0, blue_i = 0;
							int ret = sscanf(attr_value+1, "%02x%02x%02x", &red_i, &green_i, &blue_i);
							red = red_i / 255.0;
							green = green_i / 255.0;
							blue = blue_i / 255.0;
						}
					}
					xmlTextReaderMoveToElement(reader);
					
					// get points
					while (xmlTextReaderMoveToNextAttribute(reader))
					{
						char * attr_name = xmlTextReaderName(reader);
						char * attr_value = xmlTextReaderValue(reader);
						if (strcmp(attr_name, "d")==0)
						{
							shape_row_id++;
							int shape_part_id = 1;
							char * ptr = strtok(attr_value, " ");
							while (ptr != NULL)
							{
								//srand(time(NULL));
								//red = rand() / (float)RAND_MAX;	 // kbfu
								//green = rand() / (float)RAND_MAX; // kbfu
								//blue = rand() / (float)RAND_MAX;	// kbfu
								// 713.5 239.5
								
								double coord[2] = { 0, 0 };
								int add_point = 0;
								
								switch (ptr[0]) {
									case 'M': case 'm':
										shape_start = block->num_rows;
									case 'L': case 'l':
										coord[0] = atof(&ptr[1]);
										ptr = strtok(NULL, " ");
										coord[1] = atof(&ptr[0]);
										add_point = 1;
										break;
									case 'Z': case 'z':
										coord[0] = get_x(block, shape_start);
										coord[1] = get_y(block, shape_start);
										add_point = 1;
										//shape_part_id++; // this happens later
										break;
									case 'H': case 'h':
									case 'V': case 'b':
									case 'C': case 'c':
									case 'S': case 's':
									case 'Q': case 'q':
									case 'T': case 't':
									case 'A': case 'a':
										fprintf(stderr, "read_svg encountered unsupported path step\n");
										exit(0);
										break;
									default:
										fprintf(stderr, "read_svg failure\n");
										exit(0);
								}
								
								if (add_point) {
									block = add_row(block);
									set_xy(block, block->num_rows-1, coord[0], coord[1]);
									set_rgb(block, block->num_rows-1, red, green, blue);
									set_cell_from_int32(block, block->num_rows-1, shape_row_id_column_id, shape_row_id);
									set_cell_from_int32(block, block->num_rows-1, shape_part_id_column_id, shape_part_id);
									set_cell_from_int32(block, block->num_rows-1, shape_part_type_column_id, 5);
								}
								
								if (ptr[0] == 'Z' || ptr[0] == 'z') {
									shape_part_id++;
								}
								
								ptr = strtok(NULL, " ");
							}
						}
					}
					xmlTextReaderMoveToElement(reader);
				}
				ret = xmlTextReaderRead(reader);
			}
			xmlFreeTextReader(reader);
			if (ret != 0) fprintf(stderr, "xmlReader failed to parse\n");
		}
		free(chunk.memory);
		struct Block * opt = NULL;
		
		opt = new_block();
		opt = add_string_attribute(opt, "column_name", "x");
		opt = add_double_attribute(opt, "offset", 375);
		block = offset(block, opt);
		free_block(opt);
		
		opt = new_block();
		opt = add_string_attribute(opt, "column_name", "y");
		opt = add_double_attribute(opt, "offset", 150);
		block = offset(block, opt);
		free_block(opt);
		
		opt = new_block();
		opt = add_string_attribute(opt, "column_name", "x");
		opt = add_double_attribute(opt, "multiple", 1.3333333);
		block = multiply(block, opt);
		free_block(opt);
		
		opt = new_block();
		opt = add_string_attribute(opt, "column_name", "y");
		opt = add_double_attribute(opt, "multiple", 1.3333333);
		block = multiply(block, opt);
		free_block(opt);
		
		if (x_multiple != 1) {
			opt = new_block();
			opt = add_string_attribute(opt, "column_name", "x");
			opt = add_double_attribute(opt, "multiple", x_multiple);
			block = multiply(block, opt);
			free_block(opt);
		}
		
		if (y_multiple != 1) {
			opt = new_block();
			opt = add_string_attribute(opt, "column_name", "y");
			opt = add_double_attribute(opt, "multiple", y_multiple);
			block = multiply(block, opt);
			free_block(opt);
		}
		
		if (x_offset != 0) {
			opt = new_block();
			opt = add_string_attribute(opt, "column_name", "x");
			opt = add_double_attribute(opt, "offset", x_offset);
			block = offset(block, opt);
			free_block(opt);
		}
		
		if (y_offset != 0) {
			opt = new_block();
			opt = add_string_attribute(opt, "column_name", "y");
			opt = add_double_attribute(opt, "offset", y_offset);
			block = offset(block, opt);
			free_block(opt);
		}
		
		write_block(stdout, block);
		free_block(block);
		xmlCleanupParser();
	}
}
