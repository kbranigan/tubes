
all: read_mysql_shapes bbox write_bmp tesselate inspect add_random_colors group_shapes_on_unique_set_id read_shapefile

therest: read_mysql_line_strips


read_mysql_line_strips: scheme.o read_mysql_line_strips.c
	gcc scheme.o read_mysql_line_strips.c -o read_mysql_line_strips -I/usr/local/mysql/include/mysql -L/usr/local/mysql/lib/mysql -lmysqlclient

read_mysql_shapes: scheme.o read_mysql_shapes.c
	gcc scheme.o read_mysql_shapes.c -o read_mysql_shapes -I/usr/local/mysql/include/mysql -L/usr/local/mysql/lib/mysql -lmysqlclient

group_shapes_on_unique_set_id: scheme.o group_shapes_on_unique_set_id.c
	gcc scheme.o group_shapes_on_unique_set_id.c -o group_shapes_on_unique_set_id

bbox: scheme.o bbox.c
	gcc scheme.o bbox.c -o bbox

add_random_colors: scheme.o add_random_colors.c
	gcc scheme.o add_random_colors.c -o add_random_colors

inspect: scheme.o inspect.c
	gcc scheme.o inspect.c -o inspect

tesselate: scheme.o tesselate.c
	gcc scheme.o tesselate.c -o tesselate -framework OpenGL

write_bmp: scheme.o write_bmp.c
	gcc scheme.o write_bmp.c -o write_bmp -framework OpenGL

read_shapefile: scheme.o shapefile_src/shpopen.o shapefile_src/dbfopen.o read_shapefile.c
	gcc scheme.o shapefile_src/shpopen.o shapefile_src/dbfopen.o read_shapefile.c -o read_shapefile


scheme.o: scheme.c scheme.h
	gcc scheme.c -c -o scheme.o -Wall
