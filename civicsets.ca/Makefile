
all: main

main: ../shapefile_src/shpopen.o ../shapefile_src/dbfopen.o mongoose.o
	g++ -Wall main.c ../shapefile_src/shpopen.o ../shapefile_src/dbfopen.o mongoose.o -ldl -lpthread -o civicsets.ca
