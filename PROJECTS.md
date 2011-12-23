
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
















