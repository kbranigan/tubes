Hookah
======

Hookah is just an idea for a series of smaller applications that produce, process and consume raw vertex data.

Examples
--------

    - Reading an AutoCad DWG file
    - clipping the content by a specific bbox
    - adding colours according to a csv stylesheet
    - outputting to a webserver that serves WebGL code

    - Read in the soil map data of a province (which extends to the political boundaries - miles off the coast)
    - Cull data by the hydrological data of the same island (where there is land)
    - Add colours according to water tables
    - Produce pretty maps (png, webgl, kml)

    - Read in current transit vehicle position information from a remote server (nextbus)
    - Aligning those positions to the road or rail they should be on, to eliminate bad data
    - Produce cleaned versions of low quality data

Pipe in
-------

These programs produce raw vertex data which can be piped into one of the other applications.

<dl>
  <dt>./produce_unit_circle</dt>
    <dd>Outputs a single circle LINE_LOOP - radius 1, centre at 0,0</dd>
  <dt>./produce_unit_square</dt>
    <dd>Outputs a single square LINE_LOOP - [0,0] - [1,1]</dd>
  <dt>./produce_random_data</dt>
    <dd>Outputs random vertex data, x and y both clamped to [0.0, 1.0]</dd>
  <dt>./read_mysql <i>[sql]</i></dt>
    <dd>selects from mysql, loading shapes out of the returned rows - must select fields named 'x', 'y' and 'unique_set_id'</dd>
  <dt>./read_shapefile <i>[file] (id)</i></dt>
    <dd>reads shapefile format and produces the raw vertex data as line loops.  A lot of the features of shapefile are stripped out, but could be maintained if it mattered.</dd>
</dl>


Pipe inout
----------

You can use these applications to modify and transform the raw vertex data and pipe the result to something else. With this concept you can chain multiple processes together - once a decent number of these are built, this tool set might actually be useful.

<dl>
  <dt>./tesselate</dt>
  <dd>Translates LINE_LOOPS to TRIANGLES, uses gluTesselator (This is for rendering in OpenGL - it can only render convex polygons)</dd>
  <dt>./clip -f [file] -o [difference|intersection|exclusive-or|union]</dt>
  <dd>This takes shapes and produces the difference, intersection, exclusive-or or union of shapes.  For example, reducing a soil map to just where there is land or reducing a street graph to specific ward.</dd>
  <dt>./group_shapes_on_unique_set_id</dt>
  <dd>This application merges the vertex arrays of the shapes for each unique_set_id, used to be needed when tesselating, but is no longer.</dd>
  <dt>./add_random_colors</dt>
  <dd>This application adds an additional vertex array for the colour data - it chooses a random colour for each shape.</dd>
  <dt>./add_color_from_csv -f [file]</dt>
  <dd>This pulls in color rules from a csv file and adds those colors to the shapes that match (on unique_set_id or exact attribute values)</dd>
  <dt>./reduce_by_id <i>[id]</i></dt>
  <dd>This application drops shapes where unique_set_id != id.</dd>
  <dt>./reduce_by_distance <i>[distance]</i></dt>
  <dd>This application strips out vertexs that are too close to their previous vertex.</dd>
  <dt>./reduce_by_attribute -n <i>[name]</i> -v <i>[value]</i></dt>
  <dd>This application strips out shapes which do not have the specified attribute</dd>
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


Usage Examples
--------

icitw_wgs84.dbf is available at http://www.toronto.ca/open/datasets/wards/<br />
This repo only includes the source files, to create them, type 'make'

    ./read_shapefile icitw_wgs84.dbf | ./tesselate | ./add_random_colors | ./write_bmp toronto_ward_map.bmp
    or
    ./read_shapefile pei_hydro/prince_edward_island.dbf | ./tesselate | ./add_color_from_csv -f hydro_colors.csv | ./write_png

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
