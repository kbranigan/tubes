
vpath %.c src/functions/in:src/functions/inout:src/functions/out

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	mysql= -I/usr/include/mysql -lmysqlclient -L/usr/lib/mysql
endif
ifeq ($(UNAME_S),Darwin)
	mysql= -I/usr/local/mysql/include /usr/local/mysql/lib/libmysqlclient.a
endif

all: mkbin \
	bin/block.o \
	bin/block_hashtable.o \
	bin/block_glib_hashtable_test \
	bin/block_varint.o \
	bin/read_csv \
	bin/read_csv_fast \
	bin/write_csv \
	bin/write_kml \
	bin/write_js \
	bin/write_mysql \
	bin/shapefile \
	bin/write_shapefile \
	bin/block_test \
	bin/inspect \
	bin/columns \
	bin/filter \
	bin/filter_by_bbox \
	bin/filter_by_distance \
	bin/filter_loop_overlap \
	bin/append \
	bin/add_color \
	bin/read_dem \
	bin/strcat \
	bin/curl \
	bin/upgrade_block \
	bin/trim_whitespace \
	bin/test \
	bin/pass_through \
	bin/join_geographic \
	bin/join_adjacent_geographic_shapes \
	bin/join \
	bin/generate \
	bin/unique \
	bin/clip \
	bin/attributes \
	bin/coordinate_convert \
	bin/simplify_shapes \
	bin/winding_direction \
	bin/read_osm_pbf \
	bin/bounds

extras: mkbin \
	bin/read_mysql \
	bin/read_mysql_table \
	bin/read_svg \
	bin/tesselate \
	bin/png
#	bin/read_kml \
#	bin/read_nextbus \

kevin: mkbin \
	kevin/add_wday \
	kevin/add_ticket_totals_to_addresses \
	kevin/add_color_to_addresses_tickets_by_wday_and_time \
	kevin/add_distance_to_nearest_neighbour \
	kevin/add_edge_to_street \
	kevin/move_to_nearest_street \
	kevin/host_ticket_data \
	kevin/host_address_lines \
	kevin/update_tickets.address_id \
	kevin/add_address_id_and_is_opp \
	kevin/convert_location1_to_is_opp \
	kevin/filter_no_at_and_no_op_ticket \
	kevin/update_addresses.num_tickets \
	kevin/run_changes \
	kevin/test

mkbin:
	@mkdir -p bin

#DEBUG= -g

kevin/run_changes: bin/block.o kevin/run_changes.c ext/cJSON.c
	gcc $(DEBUG) -lm bin/block.o kevin/run_changes.c ext/cJSON.c -o kevin/run_changes

kevin/add_ticket_totals_to_addresses: bin/block.o kevin/add_ticket_totals_to_addresses.c
	gcc $(DEBUG) -lm bin/block.o kevin/add_ticket_totals_to_addresses.c -o kevin/add_ticket_totals_to_addresses

kevin/filter_no_at_and_no_op_ticket: bin/block.o kevin/filter_no_at_and_no_op_ticket.c
	gcc $(DEBUG) -lm bin/block.o kevin/filter_no_at_and_no_op_ticket.c -o kevin/filter_no_at_and_no_op_ticket

kevin/add_color_to_addresses_tickets_by_wday_and_time: bin/block.o kevin/add_color_to_addresses_tickets_by_wday_and_time.c bin/hashtable.o
	gcc $(DEBUG) -lm bin/block.o kevin/add_color_to_addresses_tickets_by_wday_and_time.c bin/hashtable.o -o kevin/add_color_to_addresses_tickets_by_wday_and_time

kevin/add_distance_to_nearest_neighbour: bin/block.o bin/block_kdtree.o kevin/add_distance_to_nearest_neighbour.c
	g++ $(DEBUG) -lm bin/block.o kevin/add_distance_to_nearest_neighbour.c bin/block_kdtree.o -o kevin/add_distance_to_nearest_neighbour

kevin/add_edge_to_street: bin/block.o bin/block_kdtree.o kevin/add_edge_to_street.c
	g++ $(DEBUG) -lm bin/block.o kevin/add_edge_to_street.c bin/block_kdtree.o -o kevin/add_edge_to_street

kevin/move_to_nearest_street: bin/block.o bin/block_kdtree.o kevin/move_to_nearest_street.c bin/functions.o
	g++ $(DEBUG) -lm bin/functions.o bin/block.o kevin/move_to_nearest_street.c bin/block_kdtree.o -o kevin/move_to_nearest_street

kevin/host_ticket_data: bin/block.o bin/block_kdtree.o ext/mongoose.c kevin/host_ticket_data.c
	g++ $(DEBUG) -lm -lpthread -ldl bin/block.o bin/block_kdtree.o ext/mongoose.c kevin/host_ticket_data.c -o kevin/host_ticket_data

kevin/host_address_lines: ext/mongoose.c kevin/host_address_lines.c
	g++ $(DEBUG) -lm -lpthread -ldl ext/mongoose.c kevin/host_address_lines.c -o kevin/host_address_lines $(mysql)

kevin/update_tickets.address_id: bin/hashtable.o kevin/update_tickets.address_id.c
	g++ $(DEBUG) -lm bin/hashtable.o kevin/update_tickets.address_id.c -o kevin/update_tickets.address_id $(mysql)

kevin/test: bin/block.o kevin/test.c bin/hashtable.o bin/block_kdtree.o
	g++ $(DEBUG) -lm bin/block.o kevin/test.c bin/hashtable.o bin/block_kdtree.o -o kevin/test

kevin/add_address_id_and_is_opp: bin/block.o bin/block_hashtable.o bin/hashtable.o kevin/add_address_id_and_is_opp.c
	gcc $(DEBUG) -lm bin/block.o bin/block_hashtable.o bin/hashtable.o kevin/add_address_id_and_is_opp.c -o kevin/add_address_id_and_is_opp

kevin/convert_location1_to_is_opp: bin/block.o kevin/convert_location1_to_is_opp.c
	gcc $(DEBUG) -lm bin/block.o kevin/convert_location1_to_is_opp.c -o kevin/convert_location1_to_is_opp

kevin/update_addresses.num_tickets: bin/block.o kevin/update_addresses.num_tickets.c
	gcc $(DEBUG) -lm bin/block.o kevin/update_addresses.num_tickets.c -o kevin/update_addresses.num_tickets $(mysql)

kevin/add_wday: bin/block.o kevin/add_wday.c
	gcc $(DEBUG) -lm bin/block.o kevin/add_wday.c -o kevin/add_wday

bin/gpc.o: ext/gpc.c
	gcc $(DEBUG) ext/gpc.c -c -o bin/gpc.o

bin/hashtable.o: ext/hashtable.c
	gcc $(DEBUG) ext/hashtable.c -c -o bin/hashtable.o

bin/functions.o: src/functions/functions.c $(wildcard src/functions/in/*.c) $(wildcard src/functions/inout/*.c)  $(wildcard src/functions/out/*.c)
	gcc $(DEBUG) src/functions/functions.c -c -o bin/functions.o

bin/block.o: src/block.c src/block.h unique.c
	gcc $(DEBUG) src/block.c -c -o bin/block.o

bin/block_hashtable.o: bin/block.o src/block_hashtable.c
	gcc $(DEBUG) src/block_hashtable.c -c -o bin/block_hashtable.o

bin/block_glib_hashtable.o: bin/block.o src/block_glib_hashtable.c
	gcc $(DEBUG) src/block_glib_hashtable.c -c -o bin/block_glib_hashtable.o `pkg-config --cflags glib-2.0`

bin/block_glib_hashtable_test: bin/block.o bin/block_glib_hashtable.o src/block_glib_hashtable_test.c
	gcc $(DEBUG) $^ -o $@ `pkg-config --cflags --libs glib-2.0`

bin/block_varint.o: bin/block.o src/block_varint.c
	gcc $(DEBUG) src/block_varint.c -c -o bin/block_varint.o

bin/block_kdtree.o: bin/block.o src/block_kdtree.c
	g++ $(DEBUG) src/block_kdtree.c -c -o bin/block_kdtree.o

bin/shapefile: bin/block.o src/shapefile.c ext/dbfopen.c ext/shpopen.c
	gcc $(DEBUG) -lm bin/block.o src/shapefile.c ext/dbfopen.c ext/shpopen.c -o bin/shapefile

bin/write_shapefile: bin/block.o src/write_shapefile.c ext/dbfopen.c ext/shpopen.c
	gcc $(DEBUG) -lm bin/block.o src/write_shapefile.c ext/dbfopen.c ext/shpopen.c -o bin/write_shapefile

bin/block_test: bin/block.o src/block_test.c
	gcc $(DEBUG) -lm bin/block.o src/block_test.c -o bin/block_test

bin/generate: bin/block.o src/generate.c
	gcc $(DEBUG) -lm bin/block.o src/generate.c -o bin/generate

bin/trim_whitespace: bin/block.o src/trim_whitespace.c
	gcc $(DEBUG) -lm bin/block.o src/trim_whitespace.c -o bin/trim_whitespace

bin/clip: bin/block.o src/clip.c bin/gpc.o
	gcc $(DEBUG) -lm bin/block.o bin/gpc.o src/clip.c -o bin/clip

bin/simplify_shapes: bin/block.o src/simplify_shapes.c
	gcc $(DEBUG) -lm bin/block.o src/simplify_shapes.c -o bin/simplify_shapes

bin/winding_direction: bin/block.o src/winding_direction.c
	gcc $(DEBUG) -lm bin/block.o src/winding_direction.c -o bin/winding_direction

bin/join_geographic: bin/block.o src/join_geographic.c
	gcc $(DEBUG) -lm bin/block.o src/join_geographic.c -o bin/join_geographic

bin/join_adjacent_geographic_shapes: bin/block.o src/join_adjacent_geographic_shapes.c
	gcc $(DEBUG) -lm bin/block.o src/join_adjacent_geographic_shapes.c -o bin/join_adjacent_geographic_shapes

bin/join: bin/block.o src/join.c
	gcc $(DEBUG) -lm bin/block.o src/join.c -o bin/join

bin/attributes: bin/block.o src/attributes.c
	gcc $(DEBUG) -lm bin/block.o src/attributes.c -o bin/attributes

bin/inspect: bin/block.o src/inspect.c
	gcc $(DEBUG) -lm bin/block.o src/inspect.c -o bin/inspect

bin/bounds: bin/block.o src/bounds.c
	gcc $(DEBUG) -lm bin/block.o src/bounds.c -o bin/bounds

bin/unique: bin/block.o src/unique.c
	gcc $(DEBUG) -lm bin/block.o src/unique.c -o bin/unique

bin/strcat: bin/block.o src/strcat.c
	gcc $(DEBUG) -lm bin/block.o src/strcat.c -o bin/strcat

bin/add_color: bin/block.o src/add_color.c
	gcc $(DEBUG) -lm bin/block.o src/add_color.c -o bin/add_color

bin/tesselate: bin/block.o src/tesselate.c
	gcc $(DEBUG) -lm bin/block.o src/tesselate.c -framework OpenGL -o bin/tesselate

bin/upgrade_block: bin/block.o src/upgrade_block.c
	gcc $(DEBUG) -lm bin/block.o src/upgrade_block.c -o bin/upgrade_block

bin/columns: bin/block.o src/columns.c
	gcc $(DEBUG) -lm bin/block.o src/columns.c -o bin/columns

bin/read_dem: bin/block.o src/read_dem.c
	gcc $(DEBUG) -lm bin/block.o src/read_dem.c -o bin/read_dem

bin/pass_through: bin/block.o src/pass_through.c
	gcc $(DEBUG) -lm bin/block.o src/pass_through.c -o bin/pass_through

bin/coordinate_convert: bin/block.o src/coordinate_helpers.h src/coordinate_helpers.c src/coordinate_convert.c
	gcc $(DEBUG) -lm bin/block.o src/coordinate_helpers.c src/coordinate_convert.c -o bin/coordinate_convert

bin/read_nextbus: bin/block.o src/read_nextbus.c
	gcc $(DEBUG) -lm bin/block.o src/read_nextbus.c -o bin/read_nextbus -lcurl `xml2-config --cflags --libs`

bin/read_svg: bin/block.o src/read_svg.c src/functions/inout/offset.c src/functions/inout/multiply.c
	gcc $(DEBUG) -I/usr/include/libxml2 -lxml2 -lm bin/block.o src/read_svg.c src/functions/inout/offset.c src/functions/inout/multiply.c -o bin/read_svg -lcurl `xml2-config --cflags --libs`

bin/curl: bin/block.o src/curl.c
	gcc $(DEBUG) -lm bin/block.o src/curl.c -o bin/curl -lcurl

bin/read_csv: bin/block.o src/read_csv.c src/bsv.c
	gcc $(DEBUG) -lm bin/block.o src/read_csv.c -o bin/read_csv

bin/read_csv_fast: bin/block.o src/read_csv_fast.c
	gcc $(DEBUG) -lm bin/block.o src/read_csv_fast.c -o bin/read_csv_fast

bin/read_kml: bin/block.o src/read_kml.c
	gcc $(DEBUG) -I/usr/include/libxml2 -lxml2 bin/block.o src/read_kml.c -o bin/read_kml

bin/write_csv: bin/block.o src/write_csv.c
	gcc $(DEBUG) -lm bin/block.o src/write_csv.c -o bin/write_csv

bin/write_kml: bin/block.o src/write_kml.c
	gcc $(DEBUG) -lm bin/block.o src/write_kml.c -o bin/write_kml

bin/write_js: bin/block.o src/write_js.c
	gcc $(DEBUG) -lm bin/block.o src/write_js.c -o bin/write_js

bin/write_mysql: bin/block.o src/write_mysql.c
	gcc $(DEBUG) -lm bin/block.o src/write_mysql.c -o bin/write_mysql

bin/append: bin/block.o src/append.c
	gcc $(DEBUG) -lm bin/block.o src/append.c -o bin/append

bin/filter: bin/block.o src/filter.c
	gcc $(DEBUG) -lm bin/block.o src/filter.c -o bin/filter

bin/filter_by_bbox: bin/block.o src/filter_by_bbox.c
	gcc $(DEBUG) -lm bin/block.o src/filter_by_bbox.c -o bin/filter_by_bbox

bin/filter_by_distance: bin/block.o src/filter_by_distance.c
	gcc $(DEBUG) -lm bin/block.o src/filter_by_distance.c -o bin/filter_by_distance

bin/filter_loop_overlap: bin/block.o src/filter_loop_overlap.c
	gcc $(DEBUG) -lm bin/block.o src/filter_loop_overlap.c -o bin/filter_loop_overlap

bin/test: bin/block.o src/test.c
	gcc $(DEBUG) -lm bin/block.o src/test.c -o bin/test

bin/read_mysql: bin/block.o src/read_mysql.c
	g++ $(DEBUG) -lm bin/block.o src/read_mysql.c -o bin/read_mysql $(mysql)

bin/read_mysql_table: bin/block.o src/read_mysql_table.c
	g++ $(DEBUG) -lm bin/block.o src/read_mysql_table.c -o bin/read_mysql_table $(mysql)

ext/SOIL/libsoil.a: ext/SOIL/src/SOIL.o ext/SOIL/src/image_DXT.o ext/SOIL/src/image_helper.o ext/SOIL/src/stb_image_aug.o
	ar r $@ $^

bin/png: bin/block.o src/png.c
	gcc $(DEBUG) `pkg-config --libs --cflags libpng` bin/block.o src/png.c ext/SOIL/libsoil.a -Iext/SOIL/src -o bin/png -framework OpenGL -framework CoreFoundation -Wno-deprecated-declarations

bin/read_osm_pbf: bin/block.o ext/fileformat.pb.o ext/osmformat.pb.o src/read_osm_pbf.cpp
	g++ --std=c++11 $(DEBUG) -I/opt/homebrew/include -L/opt/homebrew/lib -lprotobuf-lite -Lext -lz $^ -o bin/read_osm_pbf

ext/%.pb.o: ext/%.pb.cc
	$(CXX) --std=c++11 $(CXXFLAGS) -I/opt/homebrew/include -fPIC -c -o $@ $<

ext/%.pb.cc ext/%.pb.h: ext/%.proto
	protoc --proto_path=ext --cpp_out=ext $<
