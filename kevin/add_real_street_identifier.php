#!/usr/bin/php
<?php

mysql_connect("localhost", "root", "");
mysql_select_db("mb");

function distance($long_1, $lat_1, $long_2, $lat_2)
{
	$earth_radius = 6378100; // 3963.1676; // in miles
	$lat_1 = deg2rad($lat_1); $long_1 = deg2rad($long_1);
	$lat_2 = deg2rad($lat_2); $long_2 = deg2rad($long_2);
	
	return acos(sin($lat_1)*sin($lat_2) + 
                  cos($lat_1)*cos($lat_2) *
                  cos($long_2-$long_1)) * $earth_radius;
}

//$ids = array();
//$first_node = 13466756;
//$lf_name = "RICHMOND ST W";

$q = "SELECT MAX(kevins_id)+1 FROM centreline_oct2012";
list($kevins_id) = mysql_fetch_row(mysql_query($q));
if ($kevins_id == null) $kevins_id = 0;

//$q = "select count(*) from (select count(*), lf_name from centreline_oct2012 group by lf_name) e";
//list($num_streets) = mysql_fetch_row(mysql_query($q));
//print "$num_streets streets left\n";

$skipcount = 0;
$skipstreets = array();

$q = "SELECT COUNT(*), lf_name from centreline_oct2012 WHERE lf_name != '' AND ".
			"lf_name IS NOT NULL ".
			"AND FCODE >= 201200 AND FCODE <= 201700 AND kevins_id IS NULL GROUP BY lf_name ";//LIMIT 5";
$r_streets = mysql_query($q); if (mysql_error()) die("$q\n".mysql_error());
while ((list ($count, $lf_name) = mysql_fetch_row($r_streets)))
{
	$r_first_node = mysql_query("SELECT fnode FROM centreline_oct2012 WHERE LF_NAME = \"".mysql_real_escape_string($lf_name)."\" ".
															"AND kevins_id IS NULL ORDER BY id LIMIT 1");
	list($first_node) = mysql_fetch_row($r_first_node);
	
	//print "first_node = $first_node\n";
	
	$skip = 0;
	
	$ids = array();
	$xys = array();
	
	$nodes = array($first_node);
	do
	{
		if (count($nodes) == 0) break;
		$q = "SELECT tnode FROM centreline_oct2012 ".
				"WHERE LF_NAME = \"".mysql_real_escape_string($lf_name)."\" AND fnode IN (".implode(",",$nodes).") AND kevins_id IS NULL GROUP BY tnode LIMIT 1";
		$r = mysql_query($q); if (mysql_error()) die("$q\n".mysql_error());
		list($tnode) = mysql_fetch_row($r);
		if (!$tnode) continue;
		
		$q = "SELECT id, x, y, GEO_ID, fnode, tnode, lf_name, lfn_id FROM centreline_oct2012 ".
					"WHERE LF_NAME = \"".mysql_real_escape_string($lf_name)."\" ".
					"AND fnode IN (".implode(",",$nodes).") AND kevins_id IS NULL AND tnode = $tnode ORDER BY id ASC";
		$r = mysql_query($q); if (mysql_error()) die("$q\n".mysql_error());
		//print "$q\n";
		
		$nodes = array();
		while (($row = mysql_fetch_object($r)))
		{
			if (in_array($row->id, $ids)) continue;
			
			$ids[] = $row->id;// ." ". $row->y ." ". $row->x;
			if (!in_array($row->tnode, $nodes)) $nodes[] = $row->tnode;
			$xys[$row->id] = array($row->x, $row->y);
		}
	} while (mysql_num_rows($r) > 0);
	
	$nodes = array($first_node);
	do
	{
		if (count($nodes) == 0) break;
		$q = "SELECT fnode FROM centreline_oct2012 ".
				"WHERE LF_NAME = \"".mysql_real_escape_string($lf_name)."\" ".
				"AND tnode IN (".implode(",",$nodes).") AND kevins_id IS NULL GROUP BY fnode ORDER BY id LIMIT 1";
		$r = mysql_query($q); if (mysql_error()) die("$q\n".mysql_error());
		list($fnode) = mysql_fetch_row($r);
		if (!$fnode) continue;
		
		$q = "SELECT id, x, y, GEO_ID, fnode, tnode, lf_name, lfn_id FROM centreline_oct2012 ".
					"WHERE LF_NAME = \"".mysql_real_escape_string($lf_name)."\" ".
					"AND tnode IN (".implode(",",$nodes).") AND kevins_id IS NULL AND fnode = $fnode ORDER BY id DESC";
		$r = mysql_query($q); if (mysql_error()) die("$q\n".mysql_error());
		//print "$q\n";
		
		$nodes = array();
		while (($row = mysql_fetch_object($r)))
		{
			if (in_array($row->id, $ids)) continue;
			
			array_unshift($ids, $row->id);// ." ". $row->y ." ". $row->x);
			if (!in_array($row->fnode, $nodes)) $nodes[] = $row->fnode;
			$xys[$row->id] = array($row->x, $row->y);
		}
	} while (mysql_num_rows($r) > 0);
	
	print ".";
	//print "$lf_name (".count($ids)." nodes) \n";
	
	if (count($ids) > 0)
	{
		//print_r($ids);
		$q = "UPDATE centreline_oct2012 SET kevins_id = $kevins_id WHERE id IN (".implode(",",$ids).")";
		//print $q."\n";
		mysql_query($q); if (mysql_error()) die("$q\n".mysql_error());
		
		$prevxy = null;
		$distance = 0.0;
		foreach ($ids as $kevins_sequence => $id)
		{
			$xy = $xys[$id];
			if ($prevxy != null) $distance += distance($xy[1], $xy[0], $prevxy[1], $prevxy[0]);
			
			$q = "UPDATE centreline_oct2012 SET kevins_distance = $distance, kevins_sequence = $kevins_sequence WHERE id = $id";
			//print $q."\n";
			mysql_query($q); if (mysql_error()) die("$q\n".mysql_error());
			
			$prevxy = $xy;
		}
		$kevins_id++;
	}
}


?>