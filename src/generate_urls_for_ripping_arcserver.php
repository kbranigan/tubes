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

$svg_layer = "8"; // -1,8,9,11,13

foreach ($bboxs as $localfile => $bbox)
{
	list($x1,$y1,$x2,$y2) = explode(',', $bbox);
	
	// download
	if (!is_file("layer$svg_layer/export.$svg_layer.$localfile.svg")) {
		$url = "http://gis.toronto.ca/arcgis/rest/services/primary/cot_geospatial2_mtm/MapServer/export";
		$url .= "?dpi=96&transparent=true&format=svg&layers=show:$svg_layer";
		$url .= '&bbox='.$bbox;
		$url .= '&size=800,800&f=image';
		$cmd = "./bin/curl --url=\"$url\" > layer$svg_layer/export.$svg_layer.$localfile.svg\n";
		print "$cmd\n";
		exec($cmd);
		sleep(2);
	}
	
	// convert
	if (!is_file("layer$svg_layer/export.binary.$svg_layer.$localfile.b")) {
		$cmd =	"./bin/read_svg --filename=layer$svg_layer/export.$svg_layer.$localfile.svg --bbox=\"$bbox\" ".
						"> layer$svg_layer/export.binary.$svg_layer.$localfile.b";
		print "$cmd\n";
		exec($cmd);
	}
	
	// tspan
	if (!is_file("layer$svg_layer/export.tspan.$svg_layer.$localfile.b")) {
		$cmd =	"cat layer$svg_layer/export.binary.$svg_layer.$localfile.b ".
						"| ./bin/filter --column=tagName --value=tspan ".
						"> layer$svg_layer/export.tspan.$svg_layer.$localfile.b";
		print "$cmd\n";
		exec($cmd);
	}
	
	// path
	if (!is_file("layer$svg_layer/export.path.$svg_layer.$localfile.b")) {
		$cmd =	"cat layer$svg_layer/export.binary.$svg_layer.$localfile.b ".
						"| ./bin/filter --column=tagName --value=tspan --operator=DELETE ".
						"| ./bin/filter --column=tagName --value=rect --operator=DELETE ".
						"> layer$svg_layer/export.path.$svg_layer.$localfile.b";
		print "$cmd\n";
		exec($cmd);
	}
	
	// tesselate
	if (!is_file("layer$svg_layer/export.tesselate.$svg_layer.$localfile.b")) {
		$cmd =	"cat layer$svg_layer/export.path.$svg_layer.$localfile.b ".
						"| ./bin/tesselate ".
						"> layer$svg_layer/export.tesselate.$svg_layer.$localfile.b";
		print "$cmd\n";
		exec($cmd);
	}
	
	// joined geographic
	if (!is_file("layer$svg_layer/export.joined_geographic.$svg_layer.$localfile.b")) {
		$cmd =	"cat layer$svg_layer/export.tspan.$svg_layer.$localfile.b ".
						"| ./bin/join_geographic -f layer$svg_layer/export.tesselate.$svg_layer.$localfile.b ".
						"| ./bin/unique --column=first_hit_shape_row_id ".
						"> layer$svg_layer/export.joined_geographic.$svg_layer.$localfile.b";
		print "$cmd\n";
		exec($cmd);
	}
	
	// final
	if (!is_file("layer$svg_layer/export.final.$svg_layer.$localfile.b")) {
		$cmd =	"cat layer$svg_layer/export.path.$svg_layer.$localfile.b ".
						"| ./bin/columns --remove=text ".
						"| ./bin/join --join=left ".
								" --right_file=layer$svg_layer/export.joined_geographic.$svg_layer.$localfile.b ".
								" --left_column=shape_row_id ".
								" --right_column=first_hit_shape_row_id".
						"| ./bin/coordinate_convert ".
						"> layer$svg_layer/export.final.$svg_layer.$localfile.b";
		print "$cmd\n";
		exec($cmd);
	}
	
	//cat tspan2.b | ./bin/join_geographic -f shapes2.t.b | ./bin/unique --column=first_hit_shape_row_id > joined_geographic2.b

}

//if (!is_file("layer$svg_layer/export.binary.all.b")) {
//	$cmd = "cat layer$svg_layer/export.binary.$svg_layer.* | ./bin/append > layer$svg_layer/export.binary.all.b";
//	print "$cmd\n";
//}










?>
