#!/usr/bin/php
<?php

/*
range (3511.0278, 1267.3567)
	
bbox of toronto is:
293568.683 to 335747.204   (range 42178.521)
4826563.776 to 4857106.675 (range 30542.899)

310880.783,4833361.400,314391.812,4834628.757

*/

$bboxs = array(
'293033.9404368807,4843309.42288968,299648.5369994071,4845955.26151469',
'299648.5369994071,4843309.42288968,306263.133561933,4845955.26151469',
);

//6614.5965625264

$i = 0;
foreach ($bboxs as $bbox)
{
	$url = 'http://gis.toronto.ca/arcgis/rest/services/primary/cot_geospatial2_mtm/MapServer/export?dpi=96&transparent=true&format=svg&layers=show:3,12';
	$url .= '&bbox='.$bbox;
	$url .= '&bboxSR=2019&imageSR=2019&size=1000,400&f=image';
	
	list($x1,$y1,$x2,$y2) = explode(',', $bbox);
	$xoffset = min($x1, $x2);
	$yoffset = min($y1, $y2);
	$xmultiple = abs($x1 - $x2) / 1000;
	$ymultiple = abs($y1 - $y2) / 400;
	
	if (!is_file("export$i.svg"))
		print "./bin/curl --url=\"$url\" > export$i.svg\n";
	
	$cmd = "cat export$i.svg ".
					"| ./bin/read_svg --xoffset=$xoffset --yoffset=$yoffset --xmultiple=$xmultiple --ymultiple=$ymultiple ".
	" > fart$i.b ";// | ./bin/bounds -f fart$i.png";
	print $cmd. "\n";
	if ($argv[1] == 'ok') exec($cmd);
	$i++;
}
print "cat fart0.b fart1.b | ./bin/append | ./bin/write_kml -f output.kml\n";

die("\n\n");

$dx = 42178.521 / 10.0;
$dy = 30542.899 / 20.0;

$dx = 314391.812 - 310880.783;
$dy = 4834628.757 - 4833361.400;

$xrange = array(293568.683, 335747.204);
$yrange = array(4826563.776, 4857106.675);

for ($x1 = $xrange[0] + $dx*4 ; $x1 < $xrange[1] - $dx*4 ; $x1 += $dx)
{
	for ($y1 = $yrange[0] + $dy*8 ; $y1 < $yrange[1] - $dy*8 ; $y1 += $dy)
	{
		$x2 = $x1 + $dx;
		$y2 = $y2 + $dy;
		
		$url = "http://gis.toronto.ca/arcgis/rest/services/primary/cot_geospatial2_mtm/MapServer/export" . 
			"?dpi=96&transparent=true&format=svg&layers=show:3".
			"&bbox=$x1,$y1,$x2,$y2".
			"&size=1000,1000&f=image";
		
		print "$url\n";
		break;
	}
	break;
}

?>
