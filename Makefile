
vpath %.c src/functions/in:src/functions/inout:src/functions/out

all: mkbin \
	kevin \
	bin/read_csv \
	bin/write_csv \
	bin/write_kml \
	bin/write_mysql \
	bin/shapefile \
	bin/write_shapefile \
	bin/block_test \
	bin/inspect \
	bin/nextbus \
	bin/columns \
	bin/filter \
	bin/filter_by_distance \
	bin/append \
	bin/add_color \
	bin/read_dem \
	bin/strcat \
	bin/read_kml \
	bin/read_svg \
	bin/curl \
	bin/read_nextbus \
	bin/read_mysql \
	bin/read_mysql_table \
	bin/upgrade_block \
	bin/png \
	bin/test \
	bin/pass_through \
	bin/join_geographic \
	bin/join_adjacent_geographic_shapes \
	bin/join \
	bin/generate \
	bin/unique \
	bin/tesselate \
	bin/coordinate_convert \
	bin/bounds

kevin: \
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
	kevin/filter_no_at_and_no_op_ticket \
	kevin/test

mkbin:
	@mkdir -p bin

kevin/add_ticket_totals_to_addresses: bin/block.o kevin/add_ticket_totals_to_addresses.c
	gcc -lm bin/block.o kevin/add_ticket_totals_to_addresses.c -o kevin/add_ticket_totals_to_addresses

kevin/filter_no_at_and_no_op_ticket: bin/block.o kevin/filter_no_at_and_no_op_ticket.c
	gcc -lm bin/block.o kevin/filter_no_at_and_no_op_ticket.c -o kevin/filter_no_at_and_no_op_ticket

kevin/add_color_to_addresses_tickets_by_wday_and_time: bin/block.o kevin/add_color_to_addresses_tickets_by_wday_and_time.c bin/hashtable.o
	gcc -lm bin/block.o kevin/add_color_to_addresses_tickets_by_wday_and_time.c bin/hashtable.o -o kevin/add_color_to_addresses_tickets_by_wday_and_time

kevin/add_distance_to_nearest_neighbour: bin/block.o bin/block_kdtree.o kevin/add_distance_to_nearest_neighbour.c
	g++ -lm bin/block.o kevin/add_distance_to_nearest_neighbour.c bin/block_kdtree.o -o kevin/add_distance_to_nearest_neighbour

kevin/add_edge_to_street: bin/block.o bin/block_kdtree.o kevin/add_edge_to_street.c
	g++ -lm bin/block.o kevin/add_edge_to_street.c bin/block_kdtree.o -o kevin/add_edge_to_street

kevin/move_to_nearest_street: bin/block.o bin/block_kdtree.o kevin/move_to_nearest_street.c bin/functions.o
	g++ -lm bin/functions.o bin/block.o kevin/move_to_nearest_street.c bin/block_kdtree.o -o kevin/move_to_nearest_street

kevin/host_ticket_data: bin/block.o bin/block_kdtree.o ext/mongoose.c kevin/host_ticket_data.c
	g++ -lm -lpthread -ldl bin/block.o bin/block_kdtree.o ext/mongoose.c kevin/host_ticket_data.c -o kevin/host_ticket_data

kevin/host_address_lines: ext/mongoose.c kevin/host_address_lines.c
	g++ -lm -lpthread -ldl ext/mongoose.c kevin/host_address_lines.c -o kevin/host_address_lines -I/usr/local/include/mysql -L/usr/local/lib/mysql -lmysqlclient

kevin/update_tickets.address_id: bin/hashtable.o kevin/update_tickets.address_id.c
	g++ -lm -lmysqlclient bin/hashtable.o kevin/update_tickets.address_id.c -o kevin/update_tickets.address_id -I/usr/local/include/mysql -L/usr/local/lib/mysql

kevin/test: bin/block.o kevin/test.c bin/hashtable.o bin/block_kdtree.o
	g++ -lm bin/block.o kevin/test.c bin/hashtable.o bin/block_kdtree.o -o kevin/test

kevin/add_address_id_and_is_opp: bin/block.o bin/block_hashtable.o bin/hashtable.o kevin/add_address_id_and_is_opp.c
	gcc -lm bin/block.o bin/block_hashtable.o bin/hashtable.o kevin/add_address_id_and_is_opp.c -o kevin/add_address_id_and_is_opp

kevin/add_wday: bin/block.o kevin/add_wday.c
	gcc -lm bin/block.o kevin/add_wday.c -o kevin/add_wday

bin/hashtable.o: ext/hashtable.c
	gcc ext/hashtable.c -c -o bin/hashtable.o

bin/functions.o: src/functions/functions.c $(wildcard src/functions/in/*.c) $(wildcard src/functions/inout/*.c)  $(wildcard src/functions/out/*.c)
	gcc src/functions/functions.c -c -o bin/functions.o

bin/block.o: src/block.c src/block.h unique.c
	gcc src/block.c -c -o bin/block.o

bin/block_hashtable.o: bin/block.o src/block_hashtable.c
	gcc src/block_hashtable.c -c -o bin/block_hashtable.o

bin/block_kdtree.o: bin/block.o src/block_kdtree.c
	g++ src/block_kdtree.c -c -o bin/block_kdtree.o

bin/shapefile: bin/block.o src/shapefile.c ext/dbfopen.c ext/shpopen.c
	gcc -lm bin/block.o src/shapefile.c ext/dbfopen.c ext/shpopen.c -o bin/shapefile

bin/write_shapefile: bin/block.o src/write_shapefile.c ext/dbfopen.c ext/shpopen.c
	gcc -lm bin/block.o src/write_shapefile.c ext/dbfopen.c ext/shpopen.c -o bin/write_shapefile

bin/block_test: bin/block.o src/block_test.c
	gcc -lm bin/block.o src/block_test.c -o bin/block_test

bin/generate: bin/block.o src/generate.c
	gcc -lm bin/block.o src/generate.c -o bin/generate

bin/join_geographic: bin/block.o src/join_geographic.c
	gcc -lm bin/block.o src/join_geographic.c -o bin/join_geographic

bin/join_adjacent_geographic_shapes: bin/block.o src/join_adjacent_geographic_shapes.c
	gcc -lm bin/block.o src/join_adjacent_geographic_shapes.c -o bin/join_adjacent_geographic_shapes

bin/join: bin/block.o src/join.c
	gcc -lm bin/block.o src/join.c -o bin/join

bin/inspect: bin/block.o src/inspect.c
	gcc -lm bin/block.o src/inspect.c -o bin/inspect

bin/bounds: bin/block.o src/bounds.c
	gcc -lm bin/block.o src/bounds.c -o bin/bounds

bin/unique: bin/block.o src/unique.c
	gcc -lm bin/block.o src/unique.c -o bin/unique

bin/strcat: bin/block.o src/strcat.c
	gcc -lm bin/block.o src/strcat.c -o bin/strcat

bin/add_color: bin/block.o src/add_color.c
	gcc -lm bin/block.o src/add_color.c -o bin/add_color

bin/tesselate: bin/block.o src/tesselate.c
	gcc -lm bin/block.o src/tesselate.c -framework OpenGL -o bin/tesselate

bin/nextbus: bin/block.o src/nextbus.c
	gcc -lm bin/block.o src/nextbus.c -o bin/nextbus -lcurl `xml2-config --cflags --libs`

bin/upgrade_block: bin/block.o src/upgrade_block.c
	gcc -lm bin/block.o src/upgrade_block.c -o bin/upgrade_block

bin/columns: bin/block.o src/columns.c
	gcc -lm bin/block.o src/columns.c -o bin/columns

bin/read_dem: bin/block.o src/read_dem.c
	gcc -lm bin/block.o src/read_dem.c -o bin/read_dem

bin/pass_through: bin/block.o src/pass_through.c
	gcc -lm bin/block.o src/pass_through.c -o bin/pass_through

bin/coordinate_convert: bin/block.o src/coordinate_helpers.h src/coordinate_helpers.c src/coordinate_convert.c
	gcc -lm bin/block.o src/coordinate_helpers.c src/coordinate_convert.c -o bin/coordinate_convert

bin/read_nextbus: bin/block.o src/read_nextbus.c
	gcc -lm bin/block.o src/read_nextbus.c -o bin/read_nextbus -lcurl `xml2-config --cflags --libs`

bin/read_svg: bin/block.o src/read_svg.c src/functions/inout/offset.c src/functions/inout/multiply.c
	gcc -I/usr/include/libxml2 -lxml2 -lm bin/block.o src/read_svg.c src/functions/inout/offset.c src/functions/inout/multiply.c -o bin/read_svg -lcurl `xml2-config --cflags --libs`

bin/curl: bin/block.o src/curl.c
	gcc -lm bin/block.o src/curl.c -o bin/curl -lcurl

bin/read_csv: bin/block.o src/read_csv.c src/bsv.c
	gcc -lm bin/block.o src/read_csv.c -o bin/read_csv

bin/read_kml: bin/block.o src/read_kml.c
	gcc -I/usr/include/libxml2 -lxml2 bin/block.o src/read_kml.c -o bin/read_kml

bin/write_csv: bin/block.o src/write_csv.c
	gcc -lm bin/block.o src/write_csv.c -o bin/write_csv

bin/write_kml: bin/block.o src/write_kml.c
	gcc -lm bin/block.o src/write_kml.c -o bin/write_kml

bin/write_mysql: bin/block.o src/write_mysql.c
	gcc -lm bin/block.o src/write_mysql.c -o bin/write_mysql

bin/append: bin/block.o src/append.c
	gcc -lm bin/block.o src/append.c -o bin/append

bin/filter: bin/block.o src/filter.c
	gcc -lm bin/block.o src/filter.c -o bin/filter

bin/filter_by_distance: bin/block.o src/filter_by_distance.c
	gcc -lm bin/block.o src/filter_by_distance.c -o bin/filter_by_distance

bin/test: bin/block.o src/test.c
	gcc -lm bin/block.o src/test.c -o bin/test

bin/read_mysql: bin/block.o src/read_mysql.c
	gcc -lm bin/block.o src/read_mysql.c -o bin/read_mysql -I/usr/local/include/mysql -L/usr/local/lib/mysql -lmysqlclient

bin/read_mysql_table: bin/block.o src/read_mysql_table.c
	gcc -lm bin/block.o src/read_mysql_table.c -o bin/read_mysql_table -I/usr/local/include/mysql -L/usr/local/lib/mysql -lmysqlclient

bin/png: bin/block.o src/png.c
	gcc -lm bin/block.o src/png.c ext/SOIL/src/*.c -o bin/png -Iext/SOIL/src -framework OpenGL -framework CoreFoundation -lpng
