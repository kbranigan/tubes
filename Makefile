
mysql= -I/usr/local/mysql/include/mysql -I/usr/include/mysql -L/usr/local/mysql/lib/mysql -lmysqlclient

applegl= -framework OpenGL
linuxgl= -lOSMesa -lGL -lGLU

whichgl= $(applegl)

all: pipe_in  pipe_out  pipe_inout

pipe_in: \
	bbox \
	inspect \
	write_png \
	write_bmp \
	write_kml \
	write_json \
	write_bmp_sphere \
	pass_through

pipe_out: \
	read_shapefile \
	read_mysql \
	produce_random_data \
	produce_single_test_circle

pipe_inout: \
	clip \
	tesselate \
	reduce_by_id \
	reduce_by_distance \
	reduce_by_attribute \
	coordinate_convert \
	add_random_colors \
	add_color_from_mysql \
	group_shapes_on_unique_set_id

civicsets: shapefile_src/shpopen.o shapefile_src/dbfopen.o mongoose.o
	g++ -Wall civicsets.c shapefile_src/shpopen.o shapefile_src/dbfopen.o mongoose.o -ldl -lpthread -o civicsets.ca $(mysql)

produce_single_test_circle: scheme.o produce_single_test_circle.c
	gcc scheme.o produce_single_test_circle.c -o produce_single_test_circle -lm

read_mysql: scheme.o read_mysql.c
	gcc scheme.o read_mysql.c -o read_mysql $(mysql)

group_shapes_on_unique_set_id: scheme.o group_shapes_on_unique_set_id.c
	gcc scheme.o group_shapes_on_unique_set_id.c -o group_shapes_on_unique_set_id

reduce_by_distance: scheme.o reduce_by_distance.c
	gcc scheme.o reduce_by_distance.c -o reduce_by_distance -lm

reduce_by_attribute: scheme.o reduce_by_attribute.c
	gcc scheme.o reduce_by_attribute.c -o reduce_by_attribute

reduce_by_id: scheme.o reduce_by_id.c
	gcc scheme.o reduce_by_id.c -o reduce_by_id

pass_through: scheme.o pass_through.c
	gcc scheme.o pass_through.c -o pass_through
	
produce_random_data: scheme.o produce_random_data.c
	gcc scheme.o produce_random_data.c -o produce_random_data

clip: scheme.o clip.c
	gcc scheme.o clip.c gpc/gpc.c -o clip

bbox: scheme.o bbox.c
	gcc scheme.o bbox.c -o bbox

coordinate_convert: scheme.o coordinate_convert.c
	gcc scheme.o coordinate_convert.c -o coordinate_convert -lm

add_color_from_mysql: scheme.o add_color_from_mysql.c
	gcc scheme.o add_color_from_mysql.c -o add_color_from_mysql $(mysql)

add_random_colors: scheme.o add_random_colors.c
	gcc scheme.o add_random_colors.c -o add_random_colors

inspect: scheme.o inspect.c
	gcc scheme.o inspect.c -o inspect

tesselate: scheme.o tesselate.c
	gcc scheme.o tesselate.c -o tesselate $(whichgl)

write_png: scheme.o write_png.c setup_opengl.c
	gcc scheme.o write_png.c -o write_png $(whichgl) -lpng

write_bmp: scheme.o write_bmp.c setup_opengl.c
	gcc scheme.o write_bmp.c -o write_bmp $(whichgl)

write_bmp_sphere: scheme.o write_bmp_sphere.c setup_opengl.c
	gcc scheme.o write_bmp_sphere.c -o write_bmp_sphere $(whichgl) -lm

write_kml: scheme.o write_kml.c
	gcc scheme.o write_kml.c -o write_kml

write_json: scheme.o write_json.c
	gcc scheme.o write_json.c -o write_json

read_shapefile: scheme.o shapefile_src/shpopen.o shapefile_src/dbfopen.o read_shapefile.c
	gcc scheme.o shapefile_src/shpopen.o shapefile_src/dbfopen.o read_shapefile.c -o read_shapefile

scheme.o: scheme.c scheme.h
	gcc scheme.c -c -o scheme.o -Wall

#read_mysql_line_strips: scheme.o read_mysql_line_strips.c
#	gcc scheme.o read_mysql_line_strips.c -o read_mysql_line_strips $(mysql) 

#read_mysql_shapes: scheme.o read_mysql_shapes.c
#	gcc scheme.o read_mysql_shapes.c -o read_mysql_shapes $(mysql)

#convert_to_voronoi: scheme.o convert_to_voronoi.c
#	gcc scheme.o convert_to_voronoi.c -o convert_to_voronoi
