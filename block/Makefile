
all: \
	bin/read_csv \
	bin/write_csv \
	bin/shapefile \
	bin/block_test \
	bin/inspect \
	bin/nextbus \
	bin/columns \
	bin/filter \
	bin/add_color \
	bin/read_dem \
	bin/strcat \
	bin/read_nextbus \
	bin/read_mysql \
	bin/read_mysql_table \
	bin/upgrade_block \
	bin/png \
	bin/bounds

bin/shapefile: src/block.c src/shapefile.c
	gcc src/block.c src/shapefile.c ext/dbfopen.c ext/shpopen.c -o bin/shapefile

bin/block_test: src/block.c src/block_test.c
	gcc -g src/block.c src/block_test.c -o bin/block_test

bin/inspect: src/block.c src/inspect.c
	gcc src/block.c src/inspect.c -o bin/inspect

bin/bounds: src/block.c src/bounds.c
	gcc src/block.c src/bounds.c -o bin/bounds

bin/strcat: src/block.c src/strcat.c
	gcc src/block.c src/strcat.c -o bin/strcat

bin/add_color: src/block.c src/add_color.c
	gcc -g src/block.c src/add_color.c -o bin/add_color

bin/nextbus: src/block.c src/nextbus.c
	gcc src/block.c src/nextbus.c -o bin/nextbus -lcurl `xml2-config --cflags --libs`

bin/upgrade_block: src/block.c src/upgrade_block.c
	gcc src/block.c src/upgrade_block.c -o bin/upgrade_block

bin/columns: src/block.c src/columns.c
	gcc src/block.c src/columns.c -o bin/columns

bin/read_dem: src/block.c src/read_dem.c
	gcc src/block.c src/read_dem.c -o bin/read_dem

bin/read_nextbus: src/block.c src/read_nextbus.c
	gcc src/block.c src/read_nextbus.c -o bin/read_nextbus -lcurl `xml2-config --cflags --libs`

bin/read_csv: src/block.c src/read_csv.c src/bsv.c
	gcc src/block.c src/read_csv.c -o bin/read_csv

bin/write_csv: src/block.c src/write_csv.c
	gcc src/block.c src/write_csv.c -o bin/write_csv

bin/filter: src/block.c src/filter.c
	gcc src/block.c src/filter.c -o bin/filter

bin/read_mysql: src/block.c src/read_mysql.c
	gcc src/block.c src/read_mysql.c -o bin/read_mysql -I/usr/local/include/mysql -L/usr/local/lib/mysql -lmysqlclient

bin/read_mysql_table: src/block.c src/read_mysql_table.c
	gcc src/block.c src/read_mysql_table.c -o bin/read_mysql_table -I/usr/local/include/mysql -L/usr/local/lib/mysql -lmysqlclient

bin/png: src/block.c src/png.c
	gcc src/block.c src/png.c ../src/ext/SOIL/src/*.c -o bin/png -I../src/ext/SOIL/src -framework OpenGL -framework CoreFoundation -lpng
