
#############################################################

Takes the first channel of a wave file, grabs the root mean square of that wave, normalizes it and writes it to json

./bin/read_soundwave -c 1 -f data/Mists_of_Time-4T.wav | ./bin/rms | ./bin/normalize | ./bin/write_json

#############################################################

This is for the viewing ttc vehicles, traveling along the which ever specific shape over time
Assumes the following:
  'nextbus' mysql database is setup and populated as the following:
    ./bin/read_nextbus -n 0 | ./bin/write_sql -de | mysql -uroot nextbus
  'nextbus_null' mysql database is setup and populated as the following:
    ./bin/read_nextbus -n 0 -r '' | ./bin/write_sql -de | mysql -uroot nextbus_null
  'ttc_gtfs' mysql database is setup and populated with ttc gtfs data (in the iroquois format, shapes and shape_points tables)
  'nextbus_temp' mysql database is created (points table will be created, emptied)

#############################################################

./bin/read_mysql "SELECT shape_id = 192 as r, 0 as g, 0 as b, round((shape_id = 192) * 0.5) + (!(shape_id = 192))* 0.1 as a, lat as y, lng as x, shape_id as id FROM ttc_gtfs.shape_points WHERE shape_id IN (192,194) ORDER BY shape_id = '192' asc, shape_id, position" | \
  ./bin/tile <(./bin/read_mysql "SELECT shape_id = 194 as r, 0 as g, 0 as b, round((shape_id = 194) * 0.5) + (!(shape_id = 194))* 0.1 as a, lat as y, lng as x, shape_id as id FROM ttc_gtfs.shape_points WHERE shape_id IN (192,194) ORDER BY shape_id = '194' asc, shape_id, position") | \
  ./bin/write_png

#############################################################

./bin/read_shapefile -f /work/data/canada/hydro_v3r1/hydro_candd.dbf \
  | ./bin/clip -f <(./bin/read_shapefile -f /work/data/canada/provinces/prov_ab_p_geo83_e.dbf | ./bin/reduce_by_attribute -n NAME -v ONTARIO) \
  > hydro.ontario.b

./bin/read_shapefile -f /work/data/canada/municipalities/ontario.dbf \
  | ./bin/clip -f hydro.ontario.b \
  > ontario.muni.minus.water.b

cat ontario.muni.minus.water.b \
 | ./bin/clip -f <(./bin/produce_unit_square -x -79.38698 -y 43.67008 -w 1.5 -h 1.5) \
 | ./bin/tesselate \
 | ./bin/add_color_from_csv -f colors_for_andrew.txt \
 | ./bin/write_png

#############################################################

cat <(./bin/read_dwg -f ../property_data_for_trinity_spadina_ward/49g-13-2000.dwg -l BUILDING_LINE* | ./bin/coordinate_convert from NAD83 UTM 17 T) \
    <(./bin/read_dwg -f ../property_data_for_trinity_spadina_ward/49j-13-2000.dwg -l BUILDING_LINE* | ./bin/coordinate_convert from NAD83 UTM 17 T) \
    <(./bin/read_dwg -f ../property_data_for_trinity_spadina_ward/50g-11-2000.dwg -l BUILDING_LINE* | ./bin/coordinate_convert from NAD83 UTM 17 T) \
    <(./bin/read_dwg -f ../property_data_for_trinity_spadina_ward/50g-12-2000.dwg -l BUILDING_LINE* | ./bin/coordinate_convert from NAD83 UTM 17 T) \
    <(./bin/read_dwg -f ../property_data_for_trinity_spadina_ward/50g-13-2000.dwg -l BUILDING_LINE* | ./bin/coordinate_convert from NAD83 UTM 17 T) \
    <(./bin/read_dwg -f ../property_data_for_trinity_spadina_ward/50g-21-2000.dwg -l BUILDING_LINE* | ./bin/coordinate_convert from NAD83 UTM 17 T) \
    <(./bin/read_dwg -f ../property_data_for_trinity_spadina_ward/50g-22-2000.dwg -l BUILDING_LINE* | ./bin/coordinate_convert from NAD83 UTM 17 T) \
    <(./bin/read_dwg -f ../property_data_for_trinity_spadina_ward/50g-23-2000.dwg -l BUILDING_LINE* | ./bin/coordinate_convert from NAD83 UTM 17 T) \
    <(./bin/read_dwg -f ../property_data_for_trinity_spadina_ward/50h-11-2000.dwg -l BUILDING_LINE* | ./bin/coordinate_convert from NAD83 UTM 17 T) \
    <(./bin/read_dwg -f ../property_data_for_trinity_spadina_ward/50h-12-2000.dwg -l BUILDING_LINE* | ./bin/coordinate_convert from NAD83 UTM 17 T) \
    <(./bin/read_dwg -f ../property_data_for_trinity_spadina_ward/50h-21-2000.dwg -l BUILDING_LINE* | ./bin/coordinate_convert from NAD83 UTM 17 T) \
    <(./bin/read_dwg -f ../property_data_for_trinity_spadina_ward/50h-22-2000.dwg -l BUILDING_LINE* | ./bin/coordinate_convert from NAD83 UTM 17 T) \
    <(./bin/read_dwg -f ../property_data_for_trinity_spadina_ward/50j-11-2000.dwg -l BUILDING_LINE* | ./bin/coordinate_convert from NAD83 UTM 17 T) \
    <(./bin/read_dwg -f ../property_data_for_trinity_spadina_ward/50j-12-2000.dwg -l BUILDING_LINE* | ./bin/coordinate_convert from NAD83 UTM 17 T) \
    <(./bin/read_shapefile -f /work/data/canada/toronto/address_points/ADDRESS_POINT_WGS84.dbf -a 0 | ./bin/reduce_by_bbox -f <(./bin/read_shapefile -f /work/data/canada/toronto/wards/icitw_wgs84.dbf | ./bin/reduce_by_attribute -n SCODE_NAME -v 20 | ./bin/bbox)) \
    <(./bin/read_shapefile -f /work/data/canada/toronto/wards/icitw_wgs84.dbf | ./bin/reduce_by_attribute -n SCODE_NAME -v 20) \
    | ./bin/add_random_colors \
    | ./bin/write_png -w 2500
    
    | ./bin/clip -f <(./bin/read_shapefile -f /work/data/canada/toronto/wards/icitw_wgs84.dbf | ./bin/reduce_by_attribute -n SCODE_NAME -v 20) \
    > buildings_in_trinity_spadina_ward20_wgs84.b

    cat buildings_in_trinity_spadina_ward20_wgs84.b \
    <(./bin/read_shapefile -f /work/data/canada/toronto/wards/icitw_wgs84.dbf | ./bin/reduce_by_attribute -n SCODE_NAME -v 20) \
    | ./bin/add_random_colors \
    | ./bin/write_png

#############################################################

cat <(./bin/read_dem -f data/030/m/030m11/030m11_0200_demw.dem) <(./bin/read_dem -f data/030/m/030m11/030m11_0200_deme.dem) \
    <(./bin/read_dem -f data/030/m/030m12/030m12_0200_demw.dem) <(./bin/read_dem -f data/030/m/030m12/030m12_0200_deme.dem) \
    <(./bin/read_dem -f data/030/m/030m13/030m13_0200_demw.dem) <(./bin/read_dem -f data/030/m/030m13/030m13_0200_deme.dem) \
    <(./bin/read_dem -f data/030/m/030m14/030m14_0200_demw.dem) <(./bin/read_dem -f data/030/m/030m14/030m14_0200_deme.dem) \
    | ./bin/write_png

cat <(./bin/read_dem -f data/030/m/030m03/030m03_0200_demw.dem) <(./bin/read_dem -f data/030/m/030m03/030m03_0200_deme.dem) \
    <(./bin/read_dem -f data/030/m/030m04/030m04_0200_demw.dem) <(./bin/read_dem -f data/030/m/030m04/030m04_0200_deme.dem) \
    <(./bin/read_dem -f data/030/m/030m05/030m05_0200_demw.dem) <(./bin/read_dem -f data/030/m/030m05/030m05_0200_deme.dem) \
    <(./bin/read_dem -f data/030/m/030m06/030m06_0200_demw.dem) <(./bin/read_dem -f data/030/m/030m06/030m06_0200_deme.dem) \
    <(./bin/read_dem -f data/031/d/031d03/031d03_0200_demw.dem) <(./bin/read_dem -f data/031/d/031d03/031d03_0200_deme.dem) \
    <(./bin/read_dem -f data/031/d/031d04/031d04_0200_demw.dem) <(./bin/read_dem -f data/031/d/031d04/031d04_0200_deme.dem) \
    <(./bin/read_dem -f data/031/d/031d05/031d05_0200_demw.dem) <(./bin/read_dem -f data/031/d/031d05/031d05_0200_deme.dem) \
    <(./bin/read_dem -f data/031/d/031d06/031d06_0200_demw.dem) <(./bin/read_dem -f data/031/d/031d06/031d06_0200_deme.dem) \
    <(./bin/read_dem -f data/030/m/030m11/030m11_0200_demw.dem) <(./bin/read_dem -f data/030/m/030m11/030m11_0200_deme.dem) \
    <(./bin/read_dem -f data/030/m/030m12/030m12_0200_demw.dem) <(./bin/read_dem -f data/030/m/030m12/030m12_0200_deme.dem) \
    <(./bin/read_dem -f data/030/m/030m13/030m13_0200_demw.dem) <(./bin/read_dem -f data/030/m/030m13/030m13_0200_deme.dem) \
    <(./bin/read_dem -f data/030/m/030m14/030m14_0200_demw.dem) <(./bin/read_dem -f data/030/m/030m14/030m14_0200_deme.dem) \
    <(./bin/read_dem -f data/030/m/030m15/030m15_0200_demw.dem) <(./bin/read_dem -f data/030/m/030m15/030m15_0200_deme.dem) \
    <(./bin/read_dem -f data/030/m/030m16/030m16_0200_demw.dem) <(./bin/read_dem -f data/030/m/030m16/030m16_0200_deme.dem) \
    | ./bin/write_png

#############################################################

./bin/read_mysql "SELECT shape_id = 871 as r, 0 as g, 0 as b, round((shape_id = 871) * 0.5) + (!(shape_id = 871))* 0.1 as a, lat as y, lng as x, shape_id as id FROM ttc_gtfs.shape_points WHERE shape_id IN (871,872,873,874,875,876,877,878,879) ORDER BY shape_id = '871' asc, shape_id, position" | \
  ./bin/tile <(./bin/read_mysql "SELECT shape_id = 872 as r, 0 as g, 0 as b, round((shape_id = 872) * 0.5) + (!(shape_id = 872))* 0.1 as a, lat as y, lng as x, shape_id as id FROM ttc_gtfs.shape_points WHERE shape_id IN (871,872,873,874,875,876,877,878,879) ORDER BY shape_id = '872' asc, shape_id, position") | \
  ./bin/tile <(./bin/read_mysql "SELECT shape_id = 873 as r, 0 as g, 0 as b, round((shape_id = 873) * 0.5) + (!(shape_id = 873))* 0.1 as a, lat as y, lng as x, shape_id as id FROM ttc_gtfs.shape_points WHERE shape_id IN (871,872,873,874,875,876,877,878,879) ORDER BY shape_id = '873' asc, shape_id, position" | \
  ./bin/tile <(./bin/read_mysql "SELECT shape_id = 874 as r, 0 as g, 0 as b, round((shape_id = 874) * 0.5) + (!(shape_id = 874))* 0.1 as a, lat as y, lng as x, shape_id as id FROM ttc_gtfs.shape_points WHERE shape_id IN (871,872,873,874,875,876,877,878,879) ORDER BY shape_id = '874' asc, shape_id, position")) | \
  ./bin/tile <(./bin/read_mysql "SELECT shape_id = 875 as r, 0 as g, 0 as b, round((shape_id = 875) * 0.5) + (!(shape_id = 875))* 0.1 as a, lat as y, lng as x, shape_id as id FROM ttc_gtfs.shape_points WHERE shape_id IN (871,872,873,874,875,876,877,878,879) ORDER BY shape_id = '875' asc, shape_id, position" | \
  ./bin/tile <(./bin/read_mysql "SELECT shape_id = 876 as r, 0 as g, 0 as b, round((shape_id = 876) * 0.5) + (!(shape_id = 876))* 0.1 as a, lat as y, lng as x, shape_id as id FROM ttc_gtfs.shape_points WHERE shape_id IN (871,872,873,874,875,876,877,878,879) ORDER BY shape_id = '876' asc, shape_id, position")) | \
  ./bin/tile <(./bin/read_mysql "SELECT shape_id = 877 as r, 0 as g, 0 as b, round((shape_id = 877) * 0.5) + (!(shape_id = 877))* 0.1 as a, lat as y, lng as x, shape_id as id FROM ttc_gtfs.shape_points WHERE shape_id IN (871,872,873,874,875,876,877,878,879) ORDER BY shape_id = '877' asc, shape_id, position" | \
  ./bin/tile <(./bin/read_mysql "SELECT shape_id = 878 as r, 0 as g, 0 as b, round((shape_id = 878) * 0.5) + (!(shape_id = 878))* 0.1 as a, lat as y, lng as x, shape_id as id FROM ttc_gtfs.shape_points WHERE shape_id IN (871,872,873,874,875,876,877,878,879) ORDER BY shape_id = '878' asc, shape_id, position")) | \
  ./bin/tile -y -f <(./bin/read_mysql "SELECT shape_id = 879 as r, 0 as g, 0 as b, round((shape_id = 879) * 0.5) + (!(shape_id = 879))* 0.1 as a, lat as y, lng as x, shape_id as id FROM ttc_gtfs.shape_points WHERE shape_id IN (871,872,873,874,875,876,877,878,879) ORDER BY shape_id = '879' asc, shape_id, position") | \
  ./bin/write_png

#############################################################

./bin/read_mysql "select lat, lng, arrival_time as y, departure_time, trip_id as id, stop_sequence as x from ttc_gtfs.stop_times st left join ttc_gtfs.trips t using (gtfs_trip_id) left join ttc_gtfs.stops using (gtfs_stop_id) where route_id = 25 and service_id = 1 and shape_id = 192 order by trip_id, stop_sequence" \
  | ./bin/write_png

#############################################################

cat <(./bin/read_mysql "select 1 as r, 0 as g, 0 as b, 1 as a, lat as y, lng as x, ss.id as id, 1 as reported_at, 0 as vehicle_number from ttc_gtfs.shape_stops ss left join ttc_gtfs.stops s on ss.stop_id = s.id where shape_id = 192" \
    | ./bin/align_points_to_line_strips -f <(./bin/read_mysql "SELECT lat as y, lng as x, shape_id as unique_set_id from ttc_gtfs.shape_points where shape_id = 192 order by shape_id, position") \
    | ./bin/graph_ttc_performance -a dist_line_192 \
    ) \
  <(./bin/read_mysql "select x, y, id, -1 * (unix_timestamp() - unix_timestamp(created_at) - secsSinceReport) as reported_at, unique_set_id as vehicle_number, dirTag from nextbus.points where routeTag = 125 and round(secsSinceReport) < 6 order by dirTag, unique_set_id, created_at" \
    | ./bin/reduce_by_attribute -n dirTag -v 125_0_125 \
    | ./bin/align_points_to_line_strips -f <(./bin/read_mysql "SELECT lat as y, lng as x, shape_id as unique_set_id from ttc_gtfs.shape_points where shape_id = 192 order by shape_id, position") \
    | ./bin/graph_ttc_performance -a dist_line_192 \
    ) \
  <(./bin/read_mysql "select x, y, id, -1 * (unix_timestamp() - unix_timestamp(created_at) - secsSinceReport) as reported_at, unique_set_id as vehicle_number, dirTag from nextbus.points where routeTag = 125 and round(secsSinceReport) < 6 order by dirTag, unique_set_id, created_at" \
    | ./bin/reduce_by_attribute -n dirTag -v 125_1_125 \
    | ./bin/align_points_to_line_strips -f <(./bin/read_mysql "SELECT lat as y, lng as x, shape_id as unique_set_id from ttc_gtfs.shape_points where shape_id = 194 order by shape_id, position") \
    | ./bin/graph_ttc_performance -a dist_line_194 -i \
    ) \
  | ./bin/write_png -f cache_images/ttc.125.to.finch.png


#############################################################

./bin/read_mysql "SELECT lat as y, lng as x, shape_id as unique_set_id from ttc_gtfs.shape_points where shape_id IN (192) order by shape_id, position" \
  > 125.shape.192.b

./bin/read_mysql "select x, y, id, unix_timestamp(created_at) as reported_at, secsSinceReport, unique_set_id as vehicle_number, dirTag from nextbus.points where routeTag = 125 order by dirTag, unique_set_id, created_at" \
  > 125.gps.b

cat 125.gps.b \
  | ./bin/align_points_to_line_strips -f 125.shape.192.b \
  > 125.aligned.b

cat 125.aligned.b \
  | ./bin/reduce_by_attribute -n dirTag -v 125_0_125 \
  | ./bin/graph_ttc_performance \
  | ./bin/write_png

cat <(./bin/read_mysql "select 1 as r, 0 as g, 0 as b, 1 as a, lat as y, lng as x, ss.id as id, 1 as reported_at, 0 as vehicle_number from ttc_gtfs.shape_stops ss left join ttc_gtfs.stops s on ss.stop_id = s.id where shape_id = 192" \
    | ./bin/align_points_to_line_strips -f <(./bin/read_mysql "SELECT lat as y, lng as x, shape_id as unique_set_id from ttc_gtfs.shape_points where shape_id = 192 order by shape_id, position") \
    | ./bin/graph_ttc_performance -a dist_line_192 \
    ) \
  <(./bin/read_mysql "select x, y, id, -1 * (unix_timestamp() - unix_timestamp(created_at) - secsSinceReport) as reported_at, unique_set_id as vehicle_number, dirTag from nextbus.points where routeTag = 125 and round(secsSinceReport) < 6 order by dirTag, unique_set_id, created_at" \
    | ./bin/reduce_by_attribute -n dirTag -v 125_0_125 \
    | ./bin/align_points_to_line_strips -f <(./bin/read_mysql "SELECT lat as y, lng as x, shape_id as unique_set_id from ttc_gtfs.shape_points where shape_id = 192 order by shape_id, position") \
    | ./bin/graph_ttc_performance -a dist_line_192 \
    ) \
  | ./bin/write_png -f cache_images/ttc.125.to.finch.png

./bin/read_mysql "select x, y, id, unix_timestamp(created_at) - (select min(unix_timestamp(created_at)) from nextbus.points where routeTag = 125 and round(secsSinceReport) < 6) - secsSinceReport as reported_at, unique_set_id as vehicle_number, dirTag from nextbus.points where routeTag = 125 and round(secsSinceReport) < 6 order by dirTag, unique_set_id, created_at" \
  | ./bin/reduce_by_attribute -n dirTag -v 125_1_125 \
  | ./bin/align_points_to_line_strips -f <(./bin/read_mysql "SELECT lat as y, lng as x, shape_id as unique_set_id from ttc_gtfs.shape_points where shape_id = 194 order by shape_id, position") \
  | ./bin/graph_ttc_performance -a dist_line_194 \
  | ./bin/write_png -f cache_images/ttc.125.to.antibes.png

./bin/read_mysql "select x, y, id, unix_timestamp(created_at) - (select min(unix_timestamp(created_at)) from nextbus.points where routeTag = 60 and round(secsSinceReport) < 6) - secsSinceReport as reported_at, unique_set_id as vehicle_number, dirTag from nextbus.points where routeTag = 60 and round(secsSinceReport) < 6 order by dirTag, unique_set_id, created_at" \
  | ./bin/reduce_by_attribute -n dirTag -v 60_0_60D \
  | ./bin/align_points_to_line_strips -f <(./bin/read_mysql "SELECT lat as y, lng as x, shape_id as unique_set_id from ttc_gtfs.shape_points where shape_id = 1020 order by shape_id, position") \
  | ./bin/graph_ttc_performance -a dist_line_1020 \
  | ./bin/write_png -f cache_images/ttc.60D.to.signal.hill.png

./bin/read_mysql "select x, y, id, unix_timestamp(created_at) - (select min(unix_timestamp(created_at)) from nextbus.points where routeTag = 60 and round(secsSinceReport) < 6) - secsSinceReport as reported_at, unique_set_id as vehicle_number, dirTag from nextbus.points where routeTag = 60 and round(secsSinceReport) < 6 order by dirTag, unique_set_id, created_at" \
  | ./bin/reduce_by_attribute -n dirTag -v 60_1_60D \
  | ./bin/align_points_to_line_strips -f <(./bin/read_mysql "SELECT lat as y, lng as x, shape_id as unique_set_id from ttc_gtfs.shape_points where shape_id = 1027 order by shape_id, position") \
  | ./bin/graph_ttc_performance -a dist_line_1027 \
  | ./bin/write_png -f cache_images/ttc.60D.to.finch.png

#############################################################

./bin/read_mysql "select lat as y, lng as x, ss.id as id, 1 as reported_at, 0 as vehicle_number from ttc_gtfs.shape_stops ss left join ttc_gtfs.stops s on ss.stop_id = s.id where shape_id = 192" \
  | ./bin/align_points_to_line_strips -f <(./bin/read_mysql "SELECT lat as y, lng as x, shape_id as unique_set_id from ttc_gtfs.shape_points where shape_id = 192 order by shape_id, position") \
  | ./bin/graph_ttc_performance -a dist_line_192 \
  | ./bin/write_png


#############################################################

./bin/read_mysql "select x, y, id, created_at as reported_at, unique_set_id as vehicle_number, secsSinceReport from nextbus.points where routeTag = 510 and dirTag = '510_0_510' order by unique_set_id, created_at" \
  | ./bin/align_points_to_line_strips -f <(./bin/read_mysql "SELECT lat as y, lng as x, shape_id as unique_set_id from ttc.shape_points where shape_id = 667 order by position") \
  | ./bin/write_sql -d \
  | mysql -uroot nextbus_temp

./bin/read_mysql "select (dist_line_667+0.0)/2 as x, unix_timestamp(reported_at) - (select min(unix_timestamp(reported_at)) from nextbus_temp.points) - secsSinceReport as y, vehicle_number as unique_set_id from nextbus_temp.points order by vehicle_number, reported_at" \
  | ./bin/write_png


#############################################################

# this shows the shapes for ONTC data, 37 = ONR, 38 = ONT

./bin/read_mysql "SELECT lat as y, lng as x, shape_id as id FROM ontc.shape_points sp left join ontc.shapes s on sp.shape_id = s.id left join ontc.routes r on s.route_id = r.id WHERE r.agency_id = 37 order by shape_id, sequence" \
  | ./bin/write_png

# this shows the point to point fares as line segments

./bin/read_mysql "SELECT s.lat as y, s.lng as x, fr.id as unique_set_id, 1 as s from ontc.fare_rules fr left join ontc.stops s on fr.origin_id = s.id WHERE fr.ccf_code LIKE 'ON%' UNION SELECT s.lat as y, s.lng as x, fr.id as unique_set_id, 2 as s from ontc.fare_rules fr left join ontc.stops s on fr.destination_id = s.id WHERE fr.ccf_code LIKE 'ON%' ORDER BY unique_set_id, s" \
  | ./bin/write_png
















