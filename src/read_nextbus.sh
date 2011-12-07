#!/bin/bash
kill `pidof read_nextbus`
./read_nextbus -n 0 | ./write_sql -de | mysql -uroot nextbus
./read_nextbus -n 0 -r '' | ./write_sql -de | mysql -uroot nextbus_null
