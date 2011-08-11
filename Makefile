
mysql= -I/usr/local/mysql/include/mysql -I/usr/include/mysql -L/usr/local/mysql/lib/mysql -lmysqlclient

applegl= -framework OpenGL
linuxgl= -lOSMesa -lGL -lGLU

whichgl= $(applegl)

extra= 

all: pipe_in  pipe_out  pipe_inout extras

# these require additional libs I put them in here just to indicate that
extras: \
	write_png \
	read_dwg \
	read_mysql \
	read_nextbus \
	read_soundwave \
	fast_fourier_transform

pipe_in: \
	bbox \
	inspect \
	write_bmp \
	write_kml \
	write_sql \
	write_json \
	write_webgl \
	write_bmp_sphere \
	pass_through

pipe_out: \
	read_dem \
	read_shapefile \
	produce_random_data \
	produce_unit_circle \
	produce_unit_square

pipe_inout: \
	clip \
	delay \
	tesselate \
	transform \
	reduce_by_id \
	add_attribute \
	reduce_by_bbox \
	add_random_colors \
	remove_attributes \
	add_color_from_csv \
	coordinate_convert \
	reduce_by_distance \
	reduce_by_attribute \
	reset_unique_set_ids \
	add_color_from_mysql \
	group_shapes_on_unique_set_id

civicsets: shapefile_src/shpopen.o shapefile_src/dbfopen.o mongoose.o
	g++ $(extra) -Wall civicsets.c shapefile_src/shpopen.o shapefile_src/dbfopen.o mongoose.o -ldl -lpthread -o civicsets.ca $(mysql)

produce_unit_circle: scheme.o produce_unit_circle.c
	gcc $(extra) scheme.o produce_unit_circle.c -o produce_unit_circle -lm

produce_unit_square: scheme.o produce_unit_square.c
	gcc $(extra) scheme.o produce_unit_square.c -o produce_unit_square -lm

read_dem: scheme.o read_dem.c
	gcc $(extra) scheme.o read_dem.c -o read_dem

read_dwg: scheme.o read_dwg.c
	gcc $(extra) scheme.o read_dwg.c -o read_dwg -ldwg -Iext -Lext

read_soundwave: scheme.o read_soundwave.c
	gcc $(extra) scheme.o read_soundwave.c -o read_soundwave -lsndfile -Iext -Lext

read_mysql: scheme.o read_mysql.c
	gcc $(extra) scheme.o read_mysql.c -o read_mysql $(mysql)

read_nextbus: scheme.o read_nextbus.c
	gcc $(extra) scheme.o read_nextbus.c -o read_nextbus -lcurl `xml2-config --cflags --libs`

read_shapefile: scheme.o shapefile_src/shpopen.o shapefile_src/dbfopen.o read_shapefile.c
	gcc $(extra) scheme.o shapefile_src/shpopen.o shapefile_src/dbfopen.o read_shapefile.c -o read_shapefile

group_shapes_on_unique_set_id: scheme.o group_shapes_on_unique_set_id.c
	gcc $(extra) scheme.o group_shapes_on_unique_set_id.c -o group_shapes_on_unique_set_id

reduce_by_distance: scheme.o reduce_by_distance.c
	gcc $(extra) scheme.o reduce_by_distance.c -o reduce_by_distance -lm

reduce_by_attribute: scheme.o reduce_by_attribute.c
	gcc $(extra) scheme.o reduce_by_attribute.c -o reduce_by_attribute

reset_unique_set_ids: scheme.o reset_unique_set_ids.c
	gcc $(extra) scheme.o reset_unique_set_ids.c -o reset_unique_set_ids

reduce_by_id: scheme.o reduce_by_id.c
	gcc $(extra) scheme.o reduce_by_id.c -o reduce_by_id

reduce_by_bbox: scheme.o reduce_by_bbox.c
	gcc $(extra) scheme.o reduce_by_bbox.c -o reduce_by_bbox

remove_attributes: scheme.o remove_attributes.c
	gcc $(extra) scheme.o remove_attributes.c -o remove_attributes

pass_through: scheme.o pass_through.c
	gcc $(extra) scheme.o pass_through.c -o pass_through

delay: scheme.o delay.c
	gcc $(extra) scheme.o delay.c -o delay

produce_random_data: scheme.o produce_random_data.c
	gcc $(extra) scheme.o produce_random_data.c -o produce_random_data

clip: scheme.o clip.c
	gcc $(extra) scheme.o clip.c gpc/gpc.c -o clip

bbox: scheme.o bbox.c
	gcc $(extra) scheme.o bbox.c -o bbox

add_attribute: scheme.o add_attribute.c
	gcc $(extra) scheme.o add_attribute.c -o add_attribute

coordinate_convert: scheme.o coordinate_convert.c
	gcc $(extra) scheme.o coordinate_convert.c -o coordinate_convert -lm

add_color_from_mysql: scheme.o add_color_from_mysql.c
	gcc $(extra) scheme.o add_color_from_mysql.c -o add_color_from_mysql $(mysql)

add_color_from_csv: scheme.o add_color_from_csv.c
	gcc $(extra) scheme.o add_color_from_csv.c -o add_color_from_csv

align_points_to_line_strips: scheme.o align_points_to_line_strips.c
	gcc $(extra) scheme.o align_points_to_line_strips.c -o align_points_to_line_strips

add_random_colors: scheme.o add_random_colors.c
	gcc $(extra) scheme.o add_random_colors.c -o add_random_colors

inspect: scheme.o inspect.c
	gcc $(extra) scheme.o inspect.c -o inspect

tesselate: scheme.o tesselate.c
	gcc $(extra) scheme.o tesselate.c -o tesselate $(whichgl)

transform: scheme.o transform.c
	gcc $(extra) scheme.o transform.c -o transform

write_png: scheme.o write_png.c setup_opengl.c
	gcc $(extra) scheme.o write_png.c -o write_png $(whichgl) -lpng

write_bmp: scheme.o write_bmp.c setup_opengl.c
	gcc $(extra) scheme.o write_bmp.c -o write_bmp $(whichgl)

write_bmp_sphere: scheme.o write_bmp_sphere.c setup_opengl.c
	gcc $(extra) scheme.o write_bmp_sphere.c -o write_bmp_sphere $(whichgl) -lm

write_kml: scheme.o write_kml.c
	gcc $(extra) scheme.o write_kml.c -o write_kml

write_sql: scheme.o write_sql.c
	gcc $(extra) scheme.o write_sql.c -o write_sql

write_json: scheme.o write_json.c
	gcc $(extra) scheme.o write_json.c -o write_json

write_webgl: scheme.o write_webgl.c
	gcc $(extra) scheme.o mongoose.c write_webgl.c -o write_webgl

scheme.o: scheme.c scheme.h
	gcc $(extra) scheme.c -c -o scheme.o -Wall

%.o: %.c
	cc $(extra) -O3 -Wall $(mysql) $*.c -c -o $@

#read_mysql_line_strips: scheme.o read_mysql_line_strips.c
#	gcc $(extra) scheme.o read_mysql_line_strips.c -o read_mysql_line_strips $(mysql) 

#read_mysql_shapes: scheme.o read_mysql_shapes.c
#	gcc $(extra) scheme.o read_mysql_shapes.c -o read_mysql_shapes $(mysql)

#convert_to_voronoi: scheme.o convert_to_voronoi.c
#	gcc scheme.o convert_to_voronoi.c -o convert_to_voronoi
