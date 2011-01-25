Pipes is a series of smaller applications that produce, process and consume vertex data.

Usage:

./read_shapefile icitw_wgs84.dbf | ./tesselate | ./group_shapes_on_unique_set_id | ./add_random_colors | ./write_bmp toronto_ward_map.bmp
