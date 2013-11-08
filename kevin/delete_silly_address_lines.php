#!/usr/bin/php
<?php

mysql_connect("localhost", "root", "");
mysql_select_db("mb");

/*
mysql> select FCODE, FCODE_DESC, COUNT(*) from centreline_july2013 group by FCODE;
+--------+-------------------------------+----------+
| FCODE  | FCODE_DESC                    | COUNT(*) |
+--------+-------------------------------+----------+
| 201100 | Expressway                    |    16676 |
| 201101 | Expressway Ramp               |    33432 |
| 201200 | Major Arterial                |    21486 |
| 201201 | Major Arterial Ramp           |     4998 |
| 201300 | Minor Arterial                |    14758 |
| 201301 | Minor Arterial Ramp           |       18 |
| 201400 | Collector                     |    35087 |
| 201401 | Collector Ramp                |      227 |
| 201500 | Local                         |   150450 |
| 201600 | Other                         |    21489 |
| 201601 | Other Ramp                    |      544 |
| 201700 | Laneway                       |    10430 |
| 201800 | Pending                       |     3458 |
| 201801 | Busway                        |       80 |
| 201803 | Access Road                   |      422 |
| 202001 | Major Railway                 |    11422 |
| 202002 | Minor Railway                 |      148 |
| 203001 | River                         |    16164 |
| 203002 | Creek/Tributary               |      426 |
| 204001 | Trail                         |    53642 |
| 204002 | Walkway                       |      806 |
| 205001 | Hydro Line                    |      838 |
| 206001 | Major Shoreline               |    15230 |
| 206002 | Minor Shoreline (Land locked) |      401 |
| 207001 | Geostatistical line           |    12237 |
| 208001 | Ferry Route                   |       74 |
+--------+-------------------------------+----------+
26 rows in set (0.48 sec)
*/

$r = mysql_query("SELECT geo_id FROM centreline_july2013 WHERE FCODE IN (
	201100, 201101, 
	201800, 201801, 
	202001, 202002, 
	203001, 203002, 
	204001, 204002, 
	205001, 
	206001, 206002, 
	207001, 
	208001) GROUP BY geo_id");
$n = mysql_num_rows($r);
while (list($geo_id) = mysql_fetch_row($r)) {
	mysql_query("DELETE FROM address_lines WHERE geo_id = $geo_id"); print mysql_error();
	$i ++;
  if ($i > 100) {
  	$i = 0;
  	print ".";
  }
}

