
all: bin/csv \
	bin/shapefile \
	bin/block_test \
	bin/inspect \
	bin/nextbus \
	bin/columns \
	bin/filter \
	bin/read_mysql \
	bin/read_mysql_table \
	bin/png \
	bin/bounds

bin/shapefile: block.c shapefile.c
	gcc block.c shapefile.c ext/dbfopen.c ext/shpopen.c -o bin/shapefile

bin/block_test: block.c block_test.c
	gcc block.c block_test.c -o bin/block_test

bin/inspect: block.c inspect.c
	gcc block.c inspect.c -o bin/inspect

bin/bounds: block.c bounds.c
	gcc block.c bounds.c -o bin/bounds

bin/nextbus: block.c nextbus.c
	gcc block.c nextbus.c -o bin/nextbus -lcurl `xml2-config --cflags --libs`

bin/columns: block.c columns.c
	gcc block.c columns.c -o bin/columns

bin/filter: block.c filter.c
	gcc block.c filter.c -o bin/filter

bin/read_mysql: block.c read_mysql.c
	gcc block.c read_mysql.c -o bin/read_mysql -I/usr/local/include/mysql -L/usr/local/lib/mysql -lmysqlclient

bin/read_mysql_table: block.c read_mysql_table.c
	gcc block.c read_mysql_table.c -o bin/read_mysql_table -I/usr/local/include/mysql -L/usr/local/lib/mysql -lmysqlclient

bin/png: block.c png.c
	gcc block.c png.c ../src/ext/SOIL/src/*.c -o bin/png -I../src/ext/SOIL/src -framework OpenGL -framework CoreFoundation -lpng
