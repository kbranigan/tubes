#!/usr/bin/php
<?php

$g = "PARAM_MT[\"Mercator_2SP\",
PARAMETER[\"semi_major\",6370997.0],
PARAMETER[\"semi_minor\",6370997.0], 
PARAMETER[\"central_meridian\",180.0], 
PARAMETER[\"false_easting\",-500000.0], 
PARAMETER[\"false_northing\",-1000000.0], 
PARAMETER[\"standard parallel 1\",60.0]]";

$g = "PROJCS[\"MTM_3Degree\",GEOGCS[\"GCS_North_American_1927\",DATUM[\"D_North_American_1927\",SPHEROID[\"Clarke_1866\",6378206.4,294.9786982]],".
     "PRIMEM[\"Greenwich\",0.0],UNIT[\"Degree\",0.0174532925199433]],PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"False_Easting\",304800.0],".
     "PARAMETER[\"False_Northing\",0.0],PARAMETER[\"Central_Meridian\",-79.5],PARAMETER[\"Scale_Factor\",0.9999],PARAMETER[\"Latitude_Of_Origin\",0.0],".
     "UNIT[\"Meter\",1.0]]";


$g = str_replace("\n", "", $g);
$g = str_replace("\r", "", $g);
$g = str_replace("\t", "", $g);
$g = str_replace(" ,", ",", $g);
$g = str_replace(", ", ",", $g);

$g = preg_replace('/,(\w+)\[/', ',"$1",[', $g);
$g = preg_replace('/^(\w+)\[/', '"$1",[', $g);
print "\n$g\n";

$g = json_decode("[$g]");
print_r($g);

?>