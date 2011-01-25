Pipes is a series of smaller applications that produce, process and consume vertex data.

Usage:

./read_shapefile icitw_wgs84.dbf | ./tesselate | ./group_shapes_on_unique_set_id | ./add_random_colors | ./write_bmp toronto_ward_map.bmp

The advantage with this process is that you can redirect any stage to a file and and inspect it such as:

./read_shapefile icitw_wgs84.dbf > basic_data

cat basic_data > ./inspect

The vertex data format is a custom, binary format to increase performance.  Little Endian / Big Endian swapping is not performed.
