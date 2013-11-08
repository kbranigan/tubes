#!/usr/bin/php
<?php

date_default_timezone_set("America/Toronto");

$table_date = "july2013";//"oct2012";

/*

get street intersection names from street geo_id
SET @geo_id = 9779504; SET @lf_name = (SELECT lf_name FROM centreline_oct2012 WHERE geo_id = @geo_id LIMIT 1); SET @fnode = (SELECT fnode FROM centreline_oct2012 WHERE geo_id = @geo_id LIMIT 1); SET @tnode = (SELECT tnode FROM centreline_oct2012 WHERE geo_id = @geo_id LIMIT 1); SELECT lf_name FROM centreline_oct2012 WHERE fnode = @fnode AND lf_name != @lf_name UNION SELECT lf_name FROM centreline_oct2012 WHERE fnode = @tnode AND lf_name != @lf_name UNION SELECT lf_name FROM centreline_oct2012 WHERE tnode = @fnode AND lf_name != @lf_name UNION SELECT lf_name FROM centreline_oct2012 WHERE tnode = @tnode AND lf_name != @lf_name;
SET @geo_id = 20139610; SET @lf_name = (SELECT lf_name FROM centreline_july2013 WHERE geo_id = @geo_id LIMIT 1); SET @fnode = (SELECT fnode FROM centreline_july2013 WHERE geo_id = @geo_id LIMIT 1); SET @tnode = (SELECT tnode FROM centreline_july2013 WHERE geo_id = @geo_id LIMIT 1); SELECT lf_name FROM centreline_july2013 WHERE fnode = @fnode AND lf_name != @lf_name UNION SELECT lf_name FROM centreline_july2013 WHERE fnode = @tnode AND lf_name != @lf_name UNION SELECT lf_name FROM centreline_july2013 WHERE tnode = @fnode AND lf_name != @lf_name UNION SELECT lf_name FROM centreline_july2013 WHERE tnode = @tnode AND lf_name != @lf_name;

*/

$tests = array(
  array('lf_name'=>'Camden St', 'to'=> 'Brant St', 'from'=> 'Spadina Ave', 'now'=> strtotime('next monday 11pm'), 'tickets'=> array(0, 1.0, 1.0), 'side'=> '1'),
  array('lf_name'=>'Camden St', 'to'=> 'Brant St', 'from'=> 'Spadina Ave', 'now'=> strtotime('next monday 11pm'), 'tickets'=> array(1.0, 1.0, 1.0)),
  array('lf_name'=>'John St', 'to'=> 'Ln S Queen W John', 'from'=> 'Queen St W', 'now'=> strtotime('next monday 11pm'), 'tickets'=> array(0, 1.0, 1.0)),
  array('lf_name'=>'John St', 'to'=> 'Richmond St W', 'from'=> 'Ln S Queen W John', 'now'=> strtotime('next monday 11pm'), 'tickets'=> array(0, 1.0, 1.0)),
  array('lf_name'=>'John St', 'to'=> 'King St W', 'from'=> 'Ln N King W John', 'now'=> strtotime('next monday 11pm'), 'tickets'=> array(0, 1.0, 1.0)),
  array('lf_name'=>'Henry Lane Ter', 'from'=>'Albert Franck Pl', 'to'=>'George St S', 'now'=> strtotime('next monday 9pm'), 'tickets'=> array(0, 1.0, 1.0)),
	array('lf_name'=>'Longboat Ave', 'from'=>'Princess St', 'to'=>'Portneuf Crt', 'side'=>1, 'now'=> strtotime('next monday 12am'), 'tickets'=> array(0, 0, 1.0)),

  // lower frederick street no parking 10pm(or 12am) -2pm
  // henry lane terrance
  // Tom longboat Ln - no parking 10pm (or 12am) - 6pm

  // brant street - by the park - is free after 6pm
  // so prob..  no parking 12am - 6pm
  // stephanie street  - between john and mccaul - free from 6pm
  // wellington st , between blue jay way and the Clarence square - free from 6pm
);

$host = 'localhost';
//$host = 'mtrbtr.com';
//print strtotime('11pm');

mysql_connect('localhost', 'root', '');
mysql_select_db('mb');


$failure_count = 0;
foreach ($tests as $test_index => $test) {
	$test['side'] = round($test['side']);
	print "TESTING: side " . $test['side'] . " of " . str_pad($test['lf_name'], 25) . " at " . $test['now'] . " (" . date("Y-m-d H:i:s", $test['now']) . ")";
	if (isset($test['from'])) print " - " . str_pad($test['from'],25) . " to  " . str_pad($test['to'],25);
	print "\n";
	if (!isset($test['id'])) {
		$r = mysql_query("SELECT geo_id, fnode, tnode, x, y FROM centreline_$table_date WHERE lf_name = \"".addslashes($test['lf_name'])."\" GROUP BY fnode, tnode"); print mysql_error();
		$lf_names = array();
		while (list($geo_id, $fnode, $tnode, $x, $y) = mysql_fetch_row($r)) {
			$q = "SELECT lf_name FROM centreline_$table_date WHERE fnode = \"$fnode\" AND lf_name != \"".addslashes($test['lf_name'])."\" UNION ".
					 "SELECT lf_name FROM centreline_$table_date WHERE fnode = \"$tnode\" AND lf_name != \"".addslashes($test['lf_name'])."\" UNION ".
					 "SELECT lf_name FROM centreline_$table_date WHERE tnode = \"$fnode\" AND lf_name != \"".addslashes($test['lf_name'])."\" UNION ".
					 "SELECT lf_name FROM centreline_$table_date WHERE tnode = \"$tnode\" AND lf_name != \"".addslashes($test['lf_name'])."\"";
 		  //print $q. "\n";
			$r2 = mysql_query($q); print mysql_error();
			$lf_names = array();
			while (list($lf_name) = mysql_fetch_row($r2)) { $lf_names[] = $lf_name; }
			$lf_names = array_unique($lf_names);
			if (array_search($test['from'], $lf_names) !== false && array_search($test['to'], $lf_names) !== false) {
				$test['id'] = $test['side'] . "-" . $geo_id;
				$test['latlng'] = array($y, $x);
				//print "    FOUND YOU!!! $geo_id\n";
				break;
			}
		}
		if (!isset($test['id']) || !isset($test['from']) || !isset($test['to'])) {
			print "  FAILURE WITH \"" . $test['lf_name'] . "\" couldn't find a geo_id\n\n";
			print_r($lf_names);
			$tests[$test_index]['outcome'] = 'FAILURE';
			$failure_count++;
			continue;
		}
	}
	
	$url = 'http://'.$host.':4512/near?v=1.0&payment=Free&lat='.$test['latlng'][0].'&lng='.$test['latlng'][1].'&now='.$test['now'];
	//print "$url (at ". date("Y-m-d H:i:s", $test['now']) .")\n";
	$json = file_get_contents($url);
	$data = json_decode($json);
	//print count($data->address_ranges) . ' address_ranges in '.$data->exec_time." seconds\n";
	$found = 0;
	foreach ($data->address_ranges as $ar) {
		if ($ar->id == $test['id']) {
			if ($ar->tickets == $test['tickets']) {
				$tests[$test_index]['outcome'] = 'SUCCESS';
			} else {
				print '  FAILURE!!! "' . $test['id'] . '/' . $test['lf_name'] . '" found: (' . implode(',', $ar->tickets) . '), expected: (' . implode(',', $test['tickets']) . ") $url at (". date("Y-m-d H:i:s", $test['now']) .")\n\n";
				$tests[$test_index]['outcome'] = "FAILURE";
				$failure_count++;
			}
			$found = 1;
			continue;
		}
	}
	if ($found == 0) {
		print "  FAILURE!!! \"" . $test['id'] . '/' . $test['lf_name'] . "\" not found near '".implode(',', $test['latlng'])."' $url at (". date("Y-m-d H:i:s", $test['now']) .")\n\n";
		$tests[$test_index]['outcome'] = 'FAILURE';
		$failure_count++;
	}
}

print "failure_count = $failure_count/".count($tests)."\n";

?>
