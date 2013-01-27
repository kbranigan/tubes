#!/usr/bin/php
<?php

date_default_timezone_set("America/Toronto");

mysql_connect("localhost", "root", "");
mysql_select_db("mb");

$start = strtotime('2008-01-01');
$end = strtotime('2011-12-31');

$wdays = array();

for ($date = $start ; $date <= $end ; $date = strtotime("+1 day", $date))
{
  mysql_query("UPDATE tickets SET wday = " . (date("N", $date)-1) . " WHERE date_of_infraction = " . date("Ymd", $date));
  print date("Ymd", $date) . "\n";
}

//$r = mysql_query("SELECT id, date_of_infraction FROM tickets LIMIT 5"); print mysql_error();
//while (($address = mysql_fetch_object($r)))
//{
//  print_r($address);
//}

?>