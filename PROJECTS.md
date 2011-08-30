

This is for the viewing ttc vehicles, traveling along the which ever specific shape over time
Assumes the following:
  'nextbus' mysql database is setup and populated as the following:
    ./bin/read_nextbus -n 0 | ./bin/write_sql -de | mysql -uroot nextbus
  'nextbus_null' mysql database is setup and populated as the following:
    ./bin/read_nextbus -n 0 -r '' | ./bin/write_sql -de | mysql -uroot nextbus_null
  'ttc_gtfs' mysql database is setup and populated with ttc gtfs data (in the iroquois format, shapes and shape_points tables)
  'nextbus_temp' mysql database is created (points table will be created, emptied)


./bin/read_mysql "select x, y, id, created_at as reported_at, unique_set_id as vehicle_number, secsSinceReport from nextbus.points where routeTag = 510 and dirTag = '510_0_510' order by unique_set_id, created_at" \
| ./bin/align_points_to_line_strips -f <(./bin/read_mysql "SELECT lat as y, lng as x, shape_id as unique_set_id from ttc.shape_points where shape_id = 667 order by position") \
| ./bin/write_sql -d \
| mysql -uroot nextbus_temp

./bin/read_mysql "select (dist_line_667+0.0)/2 as x, unix_timestamp(reported_at) - secsSinceReport as y, vehicle_number as unique_set_id from nextbus_temp.points order by vehicle_number, reported_at" \
| ./bin/write_png
