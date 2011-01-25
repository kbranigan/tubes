Pipes
=====

Pipes is just an idea for a series of smaller applications that produce, process and consume raw vertex data.

Produce
-------

These programs produce raw vertex data which can be piped into one of the other applications.

<dl>
  <dt>./produce_single_test_circle</dt>
    <dd>Outputs a single circle LINE_LOOP - for testing</dd>
  <dt>./read_shapefile <i>[file]</i></dt>
    <dd>reads shapefile format and produces the raw vertex data as line loops.  A lot of the features of shapefile are stripped out, but could be maintained if it mattered.</dd>
  <dt>./read_mysql_shapes <i>[database] [table] [group_field] [order_field] [x] [y] [z]</i></dt>
    <dd>selects from mysql, loading shapes out of the selected database.table. As follows:<br />
        SELECT [group_field], [x], [y], [z] FROM [database].[table] GROUP BY [group_field] ORDER BY [order_field], id</dd>
</dl>


Process
-------

You can use these applications to modify and transform the raw vertex data and pipe the result to something else. With this concept you can chain multiple processes together - once a decent number of these are built, this tool set might actually be useful.

<dl>
  <dt>./tesselate</dt>
  <dd>Translates LINE_LOOPS to TRIANGLES, uses gluTesselator (This is for rendering in OpenGL - it can only render convex polygons)</dd>
  <dt>./group_shapes_on_unique_set_id</dt>
  <dd>The tesselator breaks an area into several TRIANGLE shapes, though they all have the same unique_set_id. This application merges the vertex arrays of the shapes for each unique_set_id.</dd>
  <dt>./add_random_colors</dt>
  <dd>This application adds an additional vertex array for the colour data - it chooses a random colour for each shape.</dd>
</dl>

Consume
-------

These are applications at the end of the chain, they make something real out of all the processing.
    
<dl>
  <dt>./write_bmp <i>[file]</i></dt>
  <dd>Makes a BMP file - rendered off screen by OpenGL.</dd>
</dl>


Examples
--------

<b>./read_shapefile <i>icitw_wgs84.dbf</i> | ./tesselate | ./group_shapes_on_unique_set_id | ./add_random_colors | ./write_bmp <i>toronto_ward_map.bmp</i></b>

The advantage with this process is that you can redirect any stage to a file and and inspect it such as:

<b>./read_shapefile icitw_wgs84.dbf > <i>basic_data</i> ; cat <i>basic_data</i> > ./inspect</b>

This will output something similar to:

    shape:
      unique_set_id: 0
      gl_type: 2
      num_attributes: 0
      num_vertexs: 32
      num_vertex_arrays: 1
      vertex_arrays:
        array_type: 32884
        num_dimensions: 3
        vertexs:
          1.000000 0.000000 0.000000 
          0.980785 0.195090 0.000000 
          0.923880 0.382683 0.000000 
          0.831470 0.555570 0.000000 
          ...
    {
      "num_shapes": 1,
      "num_vertexs": 32,
      "num_each_gl_type": [0,0,1,0,0,0,0]
    }


The raw vertex data is binary and can't be viewed on the terminal.
