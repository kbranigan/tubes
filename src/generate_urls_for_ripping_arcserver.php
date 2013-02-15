#!/usr/bin/php
<?php

/*
minx = 293176.154
maxx = 336612.886
dx = 43436.732

miny = 4827041.814
maxy = 4857580.084
dy = 30538.27

400x400 px = 333855.923,4849928.319,334914.259,4850986.654
800x800 px = 333855.923,4849928.319,335972.595,4852044.989

400x400 px d = 1058.335,1058.335
800x800 px d = 2116.67,2116.67
*/

$bigbbox = array(293176.154, 4827041.814, 336612.886, 4857580.084);

//20x15 = 300 tiles

$bboxs = array();

for ($x = $bigbbox[0] ; $x <= $bigbbox[2] ; $x += 2116.67) {
	for ($y = $bigbbox[1] ; $y <= $bigbbox[3] ; $y += 2116.67) {
		$bboxs[round(($x-$bigbbox[0])/2116.67)."-".round(($y-$bigbbox[1])/2116.67)] = "$x,$y,".($x+2116.67).",".($y+2116.67);
	}
}

$svg_layers = "13"; // -1,8,9,11,13

$i = 0;
foreach ($bboxs as $localfile => $bbox)
{
	list($x1,$y1,$x2,$y2) = explode(',', $bbox);
	
	$url = "http://gis.toronto.ca/arcgis/rest/services/primary/cot_geospatial2_mtm/MapServer/export";
	$url .= "?dpi=96&transparent=true&format=svg&layers=show:$svg_layers";
	$url .= '&bbox='.$bbox;
	$url .= '&size=800,800&f=image';
	if (!is_file("export.$svg_layers.$localfile.svg"))
		exec("../bin/curl --url=\"$url\" > export.$svg_layers.$localfile.svg\n");
	
	//$cmd = "./bin/read_svg --filename=export.$localfile.svg --bbox=\"$bbox\" | ./bin/coordinate_convert > export.binary.$localfile.b";
	//exec($cmd);
	
	print "export.binary.$localfile.b ";
	
	//$i++;
	sleep(2);
}












?>
