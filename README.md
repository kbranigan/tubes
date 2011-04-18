Pipes
=====

Pipes is just an idea for a series of smaller applications that produce, process and consume raw vertex data.

Pipe in
-------

These programs produce raw vertex data which can be piped into one of the other applications.

<dl>
  <dt>./produce_single_test_circle</dt>
    <dd>Outputs a single circle LINE_LOOP - for testing</dd>
  <dt>./produce_random_data</dt>
    <dd>Outputs random vertex data, x and y clamped to [0.0, 1.0]</dd>
  <dt>./read_shapefile <i>[file] (id)</i></dt>
    <dd>reads shapefile format and produces the raw vertex data as line loops.  A lot of the features of shapefile are stripped out, but could be maintained if it mattered.</dd>
  <dt>./read_mysql <i>[sql]</i></dt>
    <dd>selects from mysql, loading shapes out of the returned rows - must select fields named 'x', 'y' and 'unique_set_id'</dd>
  <dt>./read_mysql_shapes <i>[database] [table] [group_field] [order_field] [x] [y] [z]</i></dt>
    <dd>selects from mysql, loading shapes out of the selected database.table. As follows:<br />
        SELECT [group_field], [x], [y], [z] FROM [database].[table] GROUP BY [group_field] ORDER BY [order_field], id</dd>
</dl>


Pipe inout
-------

You can use these applications to modify and transform the raw vertex data and pipe the result to something else. With this concept you can chain multiple processes together - once a decent number of these are built, this tool set might actually be useful.

<dl>
  <dt>./tesselate</dt>
  <dd>Translates LINE_LOOPS to TRIANGLES, uses gluTesselator (This is for rendering in OpenGL - it can only render convex polygons)</dd>
  <dt>./group_shapes_on_unique_set_id</dt>
  <dd>This application merges the vertex arrays of the shapes for each unique_set_id, used to be needed when tesselating, but is no longer.</dd>
  <dt>./add_random_colors</dt>
  <dd>This application adds an additional vertex array for the colour data - it chooses a random colour for each shape.</dd>
  <dt>./reduce_by_distance <i>[distance]</i></dt>
  <dd>This application strips out vertexs that are too close to their previous vertex.</dd>
  <dt>./coordinate_convert <i>[nad27/wgs84] [utm/mtm] [zone] (mrm)</i></dt>
  <dd>Converts silly coordinate systems to lat/lng</dd>
</dl>

Pipe out
-------

These are applications at the end of the chain, they make something real out of all the processing.
    
<dl>
  <dt>./write_png <i>[file]</i></dt>
  <dd>Makes a PNG file - rendered off screen by OpenGL.</dd>
  <dt>./write_bmp <i>[file]</i></dt>
  <dd>Makes a BMP file - rendered off screen by OpenGL.</dd>
  <dt>./write_kml <i>[file]</i></dt>
  <dd>Makes a KML file - this'll work in google earth, or google maps provided you have a url for it.</dd>
  <dt>./write_json <i>[file]</i></dt>
  <dd>Outputs to a JSON file</dd>
</dl>


Examples
--------

icitw_wgs84.dbf is available at http://www.toronto.ca/open/datasets/wards/<br />
This repo only includes the source files, to create them, type 'make'

    ./read_shapefile icitw_wgs84.dbf | ./tesselate | ./add_random_colors | ./write_bmp toronto_ward_map.bmp

This produces: http://branigan.ca/toronto_ward_map.gif

    ./read_shapefile icitw_wgs84.dbf | ./write_kml toronto_ward_map.kml

This produces: http://maps.google.com/maps?q=http://branigan.ca/toronto_ward_map.kml

The advantage with this process is that you can redirect any stage to a file and and inspect it such as:

    ./read_shapefile icitw_wgs84.dbf > some_binary_data_file ; cat some_binary_data_file | ./inspect

This will output something similar to:

    shape:
      unique_set_id: 0
      gl_type: 2
      num_attributes: 0
      num_vertexs: 410
      num_vertex_arrays: 1
      vertex_arrays:
        array_type: 32884
        num_dimensions: 2
        vertexs:
          -79.264856 43.779556 
          -79.264953 43.779541 
          -79.266121 43.779358 
          -79.266228 43.779343 
          ...
    shape:
      unique_set_id: 1
      gl_type: 2
      num_attributes: 0
      num_vertexs: 909
      num_vertex_arrays: 1
      vertex_arrays:
        array_type: 32884
        num_dimensions: 2
        vertexs:
          -79.170768 43.755638 
          -79.170804 43.755717 
          -79.170929 43.755827 
          -79.171088 43.755951 
          ...
    {
      "num_shapes": 44,
      "num_vertexs": 26787,
      "num_each_gl_type": [0,0,44,0,0,0,0]
    }

The raw vertex data stream is binary and can't be viewed directly on the terminal.
