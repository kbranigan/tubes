#!/usr/bin/php
<?php

if ($argv[1] != 'ok')
	die(str_repeat('=',40) . "\n'".$argv[0]."' truncates address_lines, call with 'ok' as argv[1] and i'll let you pass\n".str_repeat('=',40) ."\n");

mysql_connect("localhost", "root", "");
mysql_select_db("mb");

mysql_query("truncate table address_lines"); print mysql_error();
//mysql_query("DELETE FROM address_lines where id > 22"); print mysql_error();

$double_width = array('Spadina Ave', 'University Ave', 'St Clair Ave W');
// AND LF_NAME IN ('Richmond St W', 'Brant St','Camden St')
$r_haha = mysql_query("SELECT LF_NAME FROM centreline_oct2012 WHERE LF_NAME != '' AND LF_NAME IS NOT NULL GROUP BY LF_NAME"); print mysql_error();
while ((list($lf_name) = mysql_fetch_row($r_haha)))
{
	$points = array();
	$r_points = mysql_query("SELECT x, y, GEO_ID FROM centreline_oct2012 ".
		"WHERE LF_NAME = \"".mysql_real_escape_string($lf_name)."\" ORDER BY GEO_ID, id"); print mysql_error();
	while (($point = mysql_fetch_object($r_points)))
		$points[$point->GEO_ID][] = $point;
	
	$road_width = 0.000075;
	if (in_array($lf_name, $double_width)) $road_width *= 2;
	
	foreach ($points as $geo_id => $geo)
	{
		$count = count($geo);
		foreach ($geo as $i => $point)
		{
			if ($i < $count-1)
				$angle = atan2($geo[$i]->y - $geo[$i+1]->y, $geo[$i]->x - $geo[$i+1]->x);
			
			$point->lx = $point->x + cos($angle+1.5707963268)*$road_width;
			$point->ly = $point->y + sin($angle+1.5707963268)*$road_width;
			$point->rx = $point->x + cos($angle-1.5707963268)*$road_width;
			$point->ry = $point->y + sin($angle-1.5707963268)*$road_width;
			
			if ($i == 0)
			{
				$point->lx += cos($angle+3.14159265)*($road_width);
				$point->ly += sin($angle+3.14159265)*($road_width);
				$point->rx += cos($angle-3.14159265)*($road_width);
				$point->ry += sin($angle-3.14159265)*($road_width);
			}
			else if ($i == $count-1)
			{
				$point->lx -= cos($angle+3.14159265)*($road_width);
				$point->ly -= sin($angle+3.14159265)*($road_width);
				$point->rx -= cos($angle+3.14159265)*($road_width);
				$point->ry -= sin($angle+3.14159265)*($road_width);
			}
			
			$q = "INSERT INTO address_lines (geo_id, lf_name, x, y, sequence, arc_side) values ".
				"($point->GEO_ID, \"".mysql_real_escape_string($lf_name)."\", ".$point->lx.", ".$point->ly.", $i, 'L')";
			mysql_query($q); print mysql_error();
			mysql_query("INSERT INTO address_lines (geo_id, lf_name, x, y, sequence, arc_side) values ".
				"($point->GEO_ID, \"".mysql_real_escape_string($lf_name)."\", ".$point->rx.", ".$point->ry.", $i, 'R')"); print mysql_error();
		}
	}
}
//print_r($points);

?>
