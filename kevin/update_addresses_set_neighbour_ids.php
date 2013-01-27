#!/usr/bin/php
<?php

function haversine($long_1,$lat_1,$long_2,$lat_2)
{
  $earth_radius = 6378137.0; // in km  3963.1676; // in miles
  $sin_lat   = sin(deg2rad($lat_2  - $lat_1)  / 2.0);
  $sin2_lat  = $sin_lat * $sin_lat;
  $sin_long  = sin(deg2rad($long_2 - $long_2) / 2.0);
  $sin2_long = $sin_long * $sin_long;
  $cos_lat_1 = cos($lat_1);
  $cos_lat_2 = cos($lat_2);
  $sqrt      = sqrt($sin2_lat + ($cos_lat_1 * $cos_lat_2 * $sin2_long));
  $distance  = 2.0 * $earth_radius * asin($sqrt);
  return $distance;
}

mysql_connect("localhost", "root", "");
mysql_select_db("mb");

$r_streets = mysql_query("SELECT id, lf_name FROM street_names");// WHERE LF_NAME = 'CAMDEN ST'");
while ((list($street_id, $lf_name) = mysql_fetch_row($r_streets)))
{
  $r_addresses = mysql_query("SELECT id, x, y, FULL_ADDRESS, numeric_address as address, angle, FLOOR(FULL_ADDRESS)%2 AS side FROM addresses WHERE"
        . " LF_NAME = \"".mysql_real_escape_string($lf_name)."\""
        . " AND num_tickets != 0"
        );
  while (($address = mysql_fetch_object($r_addresses)))
  {
    //print_r($address);
    //print "$address->FULL_ADDRESS\n";
    
    $q = "SELECT id, x, y, FULL_ADDRESS, numeric_address as address, angle FROM addresses WHERE".
    " LF_NAME = \"$lf_name\" AND numeric_address < $address->address ".
    " AND FLOOR(FULL_ADDRESS)%2 = $address->side AND num_tickets != 0 ORDER BY numeric_address DESC LIMIT 1";
    //print $q."\n";
    $less = mysql_fetch_object(mysql_query($q));
    $q = "SELECT id, x, y, FULL_ADDRESS, numeric_address as address, angle FROM addresses WHERE".
    " LF_NAME = \"$lf_name\" AND numeric_address > $address->address ".
    " AND FLOOR(FULL_ADDRESS)%2 = $address->side AND num_tickets != 0 ORDER BY numeric_address ASC LIMIT 1";
    //print $q."\n";
    $more = mysql_fetch_object(mysql_query($q));
    
    $less_dx = $address->x - $less->x;
    $less_dy = $address->y - $less->y;
    
    $more_dx = $address->x - $more->x;
    $more_dy = $address->y - $more->y;
    
    $less_angle = atan2($less_dy, $less_dx);
    $more_angle = atan2($more_dy, $more_dx);
    
    $less_dist = haversine($address->x, $address->y, $less->x, $less->y);
    $more_dist = haversine($address->x, $address->y, $more->x, $more->y);
    
    $left_angle = $address->angle - (3.14159265/2.0);
    if ($left_angle < -3.14159265) $left_angle += 3.14159265*2.0;
    $right_angle = $address->angle + (3.14159265/2.0);
    if ($right_angle > 3.14159265) $right_angle -= 3.14159265*2.0;
    
    $less_vs_left_angle  = acos(cos($less_angle)*cos($left_angle)  + sin($less_angle)*sin($left_angle));
    $less_vs_right_angle = acos(cos($less_angle)*cos($right_angle) + sin($less_angle)*sin($right_angle));
    $more_vs_left_angle  = acos(cos($more_angle)*cos($left_angle)  + sin($more_angle)*sin($left_angle));
    $more_vs_right_angle = acos(cos($more_angle)*cos($right_angle) + sin($more_angle)*sin($right_angle));
    
    //print "$address->angle vs $less_angle\n";
    //print "$address->angle vs $more_angle\n";
    //print "left_angle  = $left_angle\n";
    //print "right_angle = $right_angle\n";
    
    unset($left);
    unset($right);
    
    if      ($less_vs_left_angle  < 3.14159265 / 4.0) $left  = $less;
    else if ($more_vs_left_angle  < 3.14159265 / 4.0) $left  = $more;
    if      ($less_vs_right_angle < 3.14159265 / 4.0) $right = $less;
    else if ($more_vs_right_angle < 3.14159265 / 4.0) $right = $more;
    
    //print "less_vs_left_angle  = $less_vs_left_angle\n";
    //print "less_vs_right_angle = $less_vs_right_angle\n";
    //print "more_vs_left_angle  = $more_vs_left_angle\n";
    //print "more_vs_right_angle = $more_vs_right_angle\n";
    
    //print "$left->FULL_ADDRESS - $address->FULL_ADDRESS - $right->FULL_ADDRESS\n";
    
    if (isset($left))  mysql_query("UPDATE addresses SET left_address_id = $left->id WHERE id = $address->id LIMIT 1");
    if (isset($right)) mysql_query("UPDATE addresses SET right_address_id = $right->id WHERE id = $address->id LIMIT 1");
    
    //print_r($right);
  }
}

?>