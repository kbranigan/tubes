Tubes
======

Tubes is my attempt at some hopefully quick to use ETL scripts, written in C to be fast enough for large datasets.
Each 'script' is a small C application, complete with it's own int main() that reads data from stdin, and writes to stdout.

We'll see how that works out for me.

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

Usage Examples
--------

These tools require linux/mac and build tools.  First you must run 'make'.

icitw_wgs84.dbf is available at http://www.toronto.ca/open/datasets/wards/

    ./bin/shapefile -f icitw_wgs84.dbf | ./bin/tesselate | ./bin/png --filename=toronto_ward_map.png

I had more examples up here, but then I changed a lot of things and the examples became severely mis-leading.  (So i deleted them)