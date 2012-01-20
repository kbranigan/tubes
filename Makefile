
mysql= -I/usr/local/mysql/include/mysql -I/usr/include/mysql -L/usr/local/mysql/lib/mysql -lmysqlclient

applegl= -framework OpenGL
linuxgl= -lOSMesa -lGL -lGLU

ext= -Isrc/ext -Lsrc/ext

whichgl= $(applegl)

extra= -lm

all: pipe_in  pipe_out  pipe_inout  

# these require additional libs I put them in here just to indicate that
extras: \
	bin/redis \
	bin/read_dwg \
	bin/write_png \
	bin/civicsets.ca \
	bin/tesselate \
	bin/read_mysql \
	bin/read_nextbus \
	bin/read_soundwave \
	bin/read_foursquare \
	bin/join_on_attributes \
	bin/fast_fourier_transform

pipe_in: \
	bin/bbox \
	bin/inspect \
	bin/write_kml \
	bin/write_sql \
	bin/write_json \
	bin/write_webgl \
	bin/pass_through

pipe_out: \
	bin/read_csv \
	bin/read_dem \
	bin/read_shapefile \
	bin/produce_random_data \
	bin/produce_unit_circle \
	bin/produce_unit_square

pipe_inout: \
	bin/rms \
	bin/clip \
	bin/tile \
	bin/delay \
	bin/normalize \
	bin/transform \
	bin/add_marker \
	bin/reduce_by_id \
	bin/add_attribute \
	bin/reduce_by_bbox \
	bin/highlight_vertex \
	bin/add_random_colors \
	bin/remove_attributes \
	bin/add_color_from_csv \
	bin/coordinate_convert \
	bin/reduce_by_overlap \
	bin/reduce_by_distance \
	bin/reduce_by_attribute \
	bin/reset_unique_set_ids \
	bin/add_color_from_mysql \
	bin/graph_ttc_performance \
	bin/align_points_to_line_strips \
	bin/group_shapes_on_unique_set_id

testing: \
	bin/stream_opengl \
	bin/write_bmp \
	bin/write_bmp_sphere \
	bin/read_walk_distance_via_osm_to_bus_stop_from_iroquois

src/ext/shpopen.o: src/ext/shpopen.c
	gcc -c src/ext/shpopen.c -o src/ext/shpopen.o

src/ext/dbfopen.o: src/ext/dbfopen.c
	gcc -c src/ext/dbfopen.c -o src/ext/dbfopen.o

bin/mongoose.o: src/mongoose.c src/mongoose.h
	gcc $(extra) src/mongoose.c -c -o bin/mongoose.o -std=c99 -D_POSIX_SOURCE -D_BSD_SOURCE

bin/scheme.o: src/scheme.c src/scheme.h
	gcc src/scheme.c -c -o bin/scheme.o

bin/civicsets.ca: bin/mongoose.o src/civicsets* src/ext/shpopen.o src/ext/dbfopen.o
	g++ $(extra) -Wall bin/mongoose.o src/civicsets.c src/ext/shpopen.o src/ext/dbfopen.o -o bin/civicsets.ca -ldl -lpthread $(mysql)

bin/produce_unit_circle: bin/scheme.o src/produce_unit_circle.c
	gcc $(extra) bin/scheme.o src/produce_unit_circle.c -o bin/produce_unit_circle -lm

bin/produce_unit_square: bin/scheme.o src/produce_unit_square.c
	gcc $(extra) bin/scheme.o src/produce_unit_square.c -o bin/produce_unit_square -lm

bin/read_dem: bin/scheme.o src/read_dem.c
	gcc $(extra) bin/scheme.o src/read_dem.c -o bin/read_dem

bin/graph_ttc_performance: bin/scheme.o src/graph_ttc_performance.c
	gcc $(extra) bin/scheme.o src/graph_ttc_performance.c -o bin/graph_ttc_performance

bin/read_csv: bin/scheme.o src/read_csv.c
	gcc $(extra) bin/scheme.o src/read_csv.c -o bin/read_csv

bin/rms: bin/scheme.o src/rms.c
	gcc $(extra) bin/scheme.o src/rms.c -o bin/rms

bin/normalize: bin/scheme.o src/normalize.c
	gcc $(extra) bin/scheme.o src/normalize.c -o bin/normalize

bin/highlight_vertex: bin/scheme.o src/highlight_vertex.c
	gcc $(extra) bin/scheme.o src/highlight_vertex.c -o bin/highlight_vertex

bin/read_dwg: bin/scheme.o src/read_dwg.c
	gcc $(extra) bin/scheme.o src/read_dwg.c -o bin/read_dwg -lredwg $(ext)

bin/redis: bin/scheme.o src/redis.c
	gcc $(extra) bin/scheme.o src/redis.c -o bin/redis -lhiredis $(ext)

bin/read_soundwave: bin/scheme.o src/read_soundwave.c
	gcc $(extra) bin/scheme.o src/read_soundwave.c -o bin/read_soundwave -lsndfile $(ext)

bin/read_mysql: bin/scheme.o src/read_mysql.c
	gcc $(extra) bin/scheme.o src/read_mysql.c -o bin/read_mysql $(mysql)

bin/read_nextbus: bin/scheme.o src/read_nextbus.c
	gcc $(extra) bin/scheme.o src/read_nextbus.c -o bin/read_nextbus -lcurl `xml2-config --cflags --libs`

bin/read_foursquare: bin/scheme.o src/read_foursquare.c
	gcc $(extra) bin/scheme.o src/read_foursquare.c src/ext/cJSON.c -o bin/read_foursquare -lcurl

bin/join_on_attributes: bin/scheme.o src/join_on_attributes.c
	gcc $(extra) bin/scheme.o src/join_on_attributes.c -o bin/join_on_attributes

bin/read_shapefile: bin/scheme.o src/ext/shpopen.c src/ext/dbfopen.c src/read_shapefile.c src/ext/shpopen.o src/ext/dbfopen.o
	gcc $(extra) bin/scheme.o src/ext/shpopen.o src/ext/dbfopen.o src/read_shapefile.c -o bin/read_shapefile

bin/group_shapes_on_unique_set_id: bin/scheme.o src/group_shapes_on_unique_set_id.c
	gcc $(extra) bin/scheme.o src/group_shapes_on_unique_set_id.c -o bin/group_shapes_on_unique_set_id

bin/reduce_by_distance: bin/scheme.o src/reduce_by_distance.c
	gcc $(extra) bin/scheme.o src/reduce_by_distance.c -o bin/reduce_by_distance -lm

bin/2dtree.o: src/2dtree.cpp
	g++ src/2dtree.cpp -c -o bin/2dtree.o -Isrc/ext

bin/reduce_by_overlap: bin/scheme.o src/reduce_by_overlap.c bin/2dtree.o
	g++ $(extra) bin/scheme.o src/reduce_by_overlap.c bin/2dtree.o -Isrc/ext -o bin/reduce_by_overlap

bin/reduce_by_attribute: bin/scheme.o src/reduce_by_attribute.c
	gcc $(extra) bin/scheme.o src/reduce_by_attribute.c -o bin/reduce_by_attribute

bin/reset_unique_set_ids: bin/scheme.o src/reset_unique_set_ids.c
	gcc $(extra) bin/scheme.o src/reset_unique_set_ids.c -o bin/reset_unique_set_ids

bin/reduce_by_id: bin/scheme.o src/reduce_by_id.c
	gcc $(extra) bin/scheme.o src/reduce_by_id.c -o bin/reduce_by_id

bin/reduce_by_bbox: bin/scheme.o src/reduce_by_bbox.c
	gcc $(extra) bin/scheme.o src/reduce_by_bbox.c -o bin/reduce_by_bbox

bin/remove_attributes: bin/scheme.o src/remove_attributes.c
	gcc $(extra) bin/scheme.o src/remove_attributes.c -o bin/remove_attributes

bin/pass_through: bin/scheme.o src/pass_through.c
	gcc $(extra) bin/scheme.o src/pass_through.c -o bin/pass_through

bin/add_marker: bin/scheme.o src/add_marker.c
	gcc $(extra) bin/scheme.o src/add_marker.c -o bin/add_marker

bin/fast_fourier_transform: bin/scheme.o src/fast_fourier_transform.c
	gcc $(extra) bin/scheme.o src/fast_fourier_transform.c -o bin/fast_fourier_transform -lfftw3 -lm $(ext)

bin/delay: bin/scheme.o src/delay.c
	gcc $(extra) bin/scheme.o src/delay.c -o bin/delay

bin/produce_random_data: bin/scheme.o src/produce_random_data.c
	gcc $(extra) bin/scheme.o src/produce_random_data.c -o bin/produce_random_data

bin/clip: bin/scheme.o src/clip.c src/ext/gpc.c
	gcc $(extra) bin/scheme.o src/clip.c src/ext/gpc.c -o bin/clip

bin/tile: bin/scheme.o src/tile.c
	gcc $(extra) bin/scheme.o src/tile.c -o bin/tile

bin/bbox: bin/scheme.o src/bbox.c
	gcc $(extra) bin/scheme.o src/bbox.c -o bin/bbox

bin/add_attribute: bin/scheme.o src/add_attribute.c
	gcc $(extra) bin/scheme.o src/add_attribute.c -o bin/add_attribute

bin/coordinate_convert: bin/scheme.o src/coordinate_convert.c
	gcc $(extra) bin/scheme.o src/coordinate_convert.c -o bin/coordinate_convert -lm

bin/add_color_from_mysql: bin/scheme.o src/add_color_from_mysql.c
	gcc $(extra) bin/scheme.o src/add_color_from_mysql.c -o bin/add_color_from_mysql $(mysql)

bin/add_color_from_csv: bin/scheme.o src/add_color_from_csv.c
	gcc $(extra) bin/scheme.o src/add_color_from_csv.c -o bin/add_color_from_csv

bin/align_points_to_line_strips: bin/scheme.o src/align_points_to_line_strips.c
	gcc $(extra) bin/scheme.o src/align_points_to_line_strips.c -o bin/align_points_to_line_strips

bin/add_random_colors: bin/scheme.o src/add_random_colors.c
	gcc $(extra) bin/scheme.o src/add_random_colors.c -o bin/add_random_colors

bin/inspect: bin/scheme.o src/inspect.c
	gcc $(extra) bin/scheme.o src/inspect.c -o bin/inspect

bin/tesselate: bin/scheme.o src/tesselate.c
	gcc $(extra) bin/scheme.o src/tesselate.c -o bin/tesselate $(whichgl)

bin/transform: bin/scheme.o src/transform.c
	gcc $(extra) bin/scheme.o src/transform.c -o bin/transform

bin/write_png: bin/scheme.o src/write_png.c src/setup_opengl.c
	gcc $(extra) bin/scheme.o src/write_png.c -o bin/write_png $(whichgl) -lpng

bin/write_bmp: bin/scheme.o src/write_bmp.c src/setup_opengl.c
	gcc $(extra) bin/scheme.o src/write_bmp.c -o bin/write_bmp $(whichgl)

bin/write_bmp_sphere: bin/scheme.o src/write_bmp_sphere.c src/setup_opengl.c
	gcc $(extra) bin/scheme.o src/write_bmp_sphere.c -o bin/write_bmp_sphere $(whichgl) -lm

bin/write_kml: bin/scheme.o src/write_kml.c
	gcc $(extra) bin/scheme.o src/write_kml.c -o bin/write_kml

bin/write_sql: bin/scheme.o src/write_sql.c
	gcc $(extra) bin/scheme.o src/write_sql.c -o bin/write_sql

bin/write_json: bin/scheme.o src/write_json.c
	gcc $(extra) bin/scheme.o src/write_json.c -o bin/write_json

bin/write_webgl: bin/scheme.o bin/mongoose.o src/write_webgl.c
	gcc $(extra) bin/scheme.o bin/mongoose.o src/write_webgl.c -o bin/write_webgl -ldl -lpthread

bin/read_walk_distance_via_osm_to_bus_stop_from_iroquois: bin/scheme.o src/read_walk_distance_via_osm_to_bus_stop_from_iroquois.c
	gcc $(extra) -lcurl bin/scheme.o src/read_walk_distance_via_osm_to_bus_stop_from_iroquois.c -o bin/read_walk_distance_via_osm_to_bus_stop_from_iroquois

bin/stream_opengl: bin/scheme.o src/stream_opengl.c
	mkdir -p bin/stream_opengl.app/Contents/Resources/English.lproj/
	/Developer/usr/bin/ibtool --strip bin/stream_opengl.app/Contents/Resources/English.lproj/MainMenu.nib --output-format human-readable-text src/ext/MainMenu.nib
	mkdir -p bin/stream_opengl.app/Contents/MacOS/
	gcc $(extra) -framework Cocoa -framework OpenGL bin/scheme.o src/stream_opengl.c -o bin/stream_opengl.app/Contents/MacOS/stream_opengl

#%.o: %.c
#	cc $(extra) -O3 -Wall $(mysql) $*.c -c -o bin/$@

#bin/write_http: bin/scheme.o src/write_http.c
#	gcc $(extra) bin/scheme.o src/mongoose.o src/write_http.c -o bin/write_http -ldl -lpthread

#read_mysql_line_strips: bin/scheme.o read_mysql_line_strips.c
#	gcc $(extra) bin/scheme.o read_mysql_line_strips.c -o bin/read_mysql_line_strips $(mysql) 

#read_mysql_shapes: bin/scheme.o read_mysql_shapes.c
#	gcc $(extra) bin/scheme.o read_mysql_shapes.c -o bin/read_mysql_shapes $(mysql)

#convert_to_voronoi: bin/scheme.o convert_to_voronoi.c
#	gcc $(extra) bin/scheme.o convert_to_voronoi.c -o bin/convert_to_voronoi
