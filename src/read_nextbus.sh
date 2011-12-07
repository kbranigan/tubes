#!/bin/bash
kill `pidof read_nextbus`
./bin/read_nextbus -n 0 | ./bin/write_sql -de | mysql -uroot nextbus
./bin/read_nextbus -n 0 -r '' | ./bin/write_sql -de | mysql -uroot nextbus_null
