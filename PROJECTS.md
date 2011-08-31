1018,1019,1020,1021,1022,1023,1024,1025,1026,1027,1028,1029

This is for the viewing ttc vehicles, traveling along the which ever specific shape over time
Assumes the following:
  'nextbus' mysql database is setup and populated as the following:
    ./bin/read_nextbus -n 0 | ./bin/write_sql -de | mysql -uroot nextbus
  'nextbus_null' mysql database is setup and populated as the following:
    ./bin/read_nextbus -n 0 -r '' | ./bin/write_sql -de | mysql -uroot nextbus_null
  'ttc_gtfs' mysql database is setup and populated with ttc gtfs data (in the iroquois format, shapes and shape_points tables)
  'nextbus_temp' mysql database is created (points table will be created, emptied)

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
  | ./bin/graph_ttc_performance -a dist_line_192) \
  <(./bin/read_mysql "select x, y, id, -1 * (unix_timestamp() - unix_timestamp(created_at) - secsSinceReport) as reported_at, unique_set_id as vehicle_number, dirTag from nextbus.points where routeTag = 125 and round(secsSinceReport) < 6 order by dirTag, unique_set_id, created_at" \
  | ./bin/reduce_by_attribute -n dirTag -v 125_0_125 \
  | ./bin/align_points_to_line_strips -f <(./bin/read_mysql "SELECT lat as y, lng as x, shape_id as unique_set_id from ttc_gtfs.shape_points where shape_id = 192 order by shape_id, position") \
  | ./bin/graph_ttc_performance -a dist_line_192) \
  | ./bin/write_png cache_images/ttc.125.to.finch.png

./bin/read_mysql "select x, y, id, unix_timestamp(created_at) - (select min(unix_timestamp(created_at)) from nextbus.points where routeTag = 125 and round(secsSinceReport) < 6) - secsSinceReport as reported_at, unique_set_id as vehicle_number, dirTag from nextbus.points where routeTag = 125 and round(secsSinceReport) < 6 order by dirTag, unique_set_id, created_at" \
  | ./bin/reduce_by_attribute -n dirTag -v 125_1_125 \
  | ./bin/align_points_to_line_strips -f <(./bin/read_mysql "SELECT lat as y, lng as x, shape_id as unique_set_id from ttc_gtfs.shape_points where shape_id = 194 order by shape_id, position") \
  | ./bin/graph_ttc_performance -a dist_line_194 \
  | ./bin/write_png cache_images/ttc.125.to.antibes.png

./bin/read_mysql "select x, y, id, unix_timestamp(created_at) - (select min(unix_timestamp(created_at)) from nextbus.points where routeTag = 60 and round(secsSinceReport) < 6) - secsSinceReport as reported_at, unique_set_id as vehicle_number, dirTag from nextbus.points where routeTag = 60 and round(secsSinceReport) < 6 order by dirTag, unique_set_id, created_at" \
  | ./bin/reduce_by_attribute -n dirTag -v 60_0_60D \
  | ./bin/align_points_to_line_strips -f <(./bin/read_mysql "SELECT lat as y, lng as x, shape_id as unique_set_id from ttc_gtfs.shape_points where shape_id = 1020 order by shape_id, position") \
  | ./bin/graph_ttc_performance -a dist_line_1020 \
  | ./bin/write_png cache_images/ttc.60D.to.signal.hill.png

./bin/read_mysql "select x, y, id, unix_timestamp(created_at) - (select min(unix_timestamp(created_at)) from nextbus.points where routeTag = 60 and round(secsSinceReport) < 6) - secsSinceReport as reported_at, unique_set_id as vehicle_number, dirTag from nextbus.points where routeTag = 60 and round(secsSinceReport) < 6 order by dirTag, unique_set_id, created_at" \
  | ./bin/reduce_by_attribute -n dirTag -v 60_1_60D \
  | ./bin/align_points_to_line_strips -f <(./bin/read_mysql "SELECT lat as y, lng as x, shape_id as unique_set_id from ttc_gtfs.shape_points where shape_id = 1027 order by shape_id, position") \
  | ./bin/graph_ttc_performance -a dist_line_1027 \
  | ./bin/write_png cache_images/ttc.60D.to.finch.png

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

