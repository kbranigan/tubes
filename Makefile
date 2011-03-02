
mysql= -DUSING_MYSQL -I/usr/local/mysql/include/mysql -I/usr/include/mysql -L/usr/local/mysql/lib/mysql -lmysqlclient

applegl= -framework OpenGL
linuxgl= -lGL -lGLU -lOSMesa

whichgl= $(applegl)

all: read_mysql_shapes bbox write_bmp write_bmp_sphere tesselate inspect add_random_colors group_shapes_on_unique_set_id read_shapefile produce_single_test_circle write_kml reduce_by_distance reduce_by_id coordinate_convert

therest: read_mysql_line_strips

produce_single_test_circle: scheme.o produce_single_test_circle.c
	gcc scheme.o produce_single_test_circle.c -o produce_single_test_circle

read_mysql_line_strips: scheme.o read_mysql_line_strips.c
	gcc scheme.o read_mysql_line_strips.c -o read_mysql_line_strips $(mysql) 

read_mysql_shapes: scheme.o read_mysql_shapes.c
	gcc scheme.o read_mysql_shapes.c -o read_mysql_shapes $(mysql)

group_shapes_on_unique_set_id: scheme.o group_shapes_on_unique_set_id.c
	gcc scheme.o group_shapes_on_unique_set_id.c -o group_shapes_on_unique_set_id

reduce_by_distance: scheme.o reduce_by_distance.c
	gcc scheme.o reduce_by_distance.c -o reduce_by_distance -lm

reduce_by_id: scheme.o reduce_by_id.c
	gcc scheme.o reduce_by_id.c -o reduce_by_id

bbox: scheme.o bbox.c
	gcc scheme.o bbox.c -o bbox -DMAIN -DFUNC=bbox

coordinate_convert: scheme.o coordinate_convert.c
	gcc scheme.o coordinate_convert.c -o coordinate_convert -lm

add_random_colors: scheme.o add_random_colors.c
	gcc scheme.o add_random_colors.c -o add_random_colors

inspect: scheme.o inspect.c
	gcc scheme.o inspect.c -o inspect

tesselate: scheme.o tesselate.c
	gcc scheme.o tesselate.c -o tesselate $(whichgl)

write_bmp: scheme.o write_bmp.c
	gcc scheme.o write_bmp.c -o write_bmp $(whichgl)

write_bmp_sphere: scheme.o write_bmp_sphere.c
	gcc scheme.o write_bmp_sphere.c -o write_bmp_sphere $(whichgl) -lm

write_kml: scheme.o write_kml.c
	gcc scheme.o write_kml.c -o write_kml

read_shapefile: scheme.o shapefile_src/shpopen.o shapefile_src/dbfopen.o read_shapefile.c
	gcc scheme.o shapefile_src/shpopen.o shapefile_src/dbfopen.o read_shapefile.c -o read_shapefile


scheme.o: scheme.c scheme.h
	gcc scheme.c -c -o scheme.o -Wall
