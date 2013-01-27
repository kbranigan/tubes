#!/usr/bin/php
<?php

if ($argv[1] != 'ok')
	die(str_repeat('=',40) . "\n'".$argv[0]."' truncates ticket_day_infractions_for_geo_id, call with 'ok' as argv[1] and i'll let you pass\n".str_repeat('=',40) ."\n");

mysql_connect("localhost", "root", "");
mysql_select_db("mb");

$codes = array();//1,5,8,9,29,207,210,264,312);

// meter based infractions
$codes = array(39,40,46,261,301,302,303,304,305,306,307,308,309,310,311,312,313,314);

if (count($codes) == 0)
{
	$r_codes = mysql_query("SELECT infraction_code FROM mb.infraction_codes GROUP BY infraction_code");
	while (list($code)=mysql_fetch_row($r_codes)) $codes[] = $code;
}

//$street_restriction = " AND lf_name = 'Camden St' ";

function add_ticket_day_infractions($one, $two) // kbfu, not great
{
  $ret = $one;
  for ($i = 0 ; $i < 48 ; $i++)
    if ($two[$i] != ' ') $ret[$i] = $two[$i];
  
  return $ret;
}

function address_sort($a, $b)
{
  return ($a->x < $b->x) ? -1 : 1;
}

function time_to_offset($time_in)
{
	$time = str_pad($time_in, 4, "0", STR_PAD_LEFT);
	$h = substr($time, 0, 2);
	$m = substr($time, 2, 4);
	$min = $h*60+$m;
	//print "$time_in $time = $min\n";
	return floor($min / 30);
}

mysql_query("truncate table ticket_day_infractions_for_geo_id");

$count = 0;
$r_streets = mysql_query("SELECT lf_name FROM mb.centreline_oct2012 WHERE lf_name IS NOT NULL $street_restriction GROUP BY lf_name");
$r_streets_count = mysql_num_rows($r_streets);
print "r_street_count = $r_streets_count\n";
while (list($lf_name) = mysql_fetch_array($r_streets))
{
	$r_geo_ids = mysql_query("SELECT geo_id FROM mb.centreline_oct2012 WHERE LF_NAME = '".mysql_real_escape_string($lf_name)."' GROUP BY geo_id");
	print mysql_error();
	$r_count_geo_ids = mysql_num_rows($r_geo_ids);
	while (list($geo_id) = mysql_fetch_array($r_geo_ids))
	{
		$addresses = array();
		
		$request_addresses = mysql_query("SELECT id FROM mb.addresses_oct2012 WHERE ".
			"LF_NAME = '".mysql_real_escape_string($lf_name)."' AND street_geo_id = $geo_id AND ARC_SIDE = 'L'"); print mysql_error();
		while (($address = mysql_fetch_object($request_addresses)))
			$addresses['left'][] = $address->id;
		
		$request_addresses = mysql_query("SELECT id FROM mb.addresses_oct2012 WHERE ".
			"LF_NAME = '".mysql_real_escape_string($lf_name)."' AND street_geo_id = $geo_id AND ARC_SIDE = 'R'"); print mysql_error();
		while (($address = mysql_fetch_object($request_addresses)))
			$addresses['right'][] = $address->id;
		
		for ($is_opp = 0 ; $is_opp <= 1 ; $is_opp++)
		{
			$tickets_all_day = array();
			$infractions = array();
			/*$tickets_query = "SELECT address_id, is_opp, date_of_infraction, time_of_infraction, wday, infraction_code, set_fine_amount FROM tickets WHERE ".
			" (".
			($addresses['left'] ? "(address_id in (".implode(",",$addresses['left']).") AND is_opp = ".$is_opp.")" : "0").
			" OR ".
			($addresses['right'] ? "(address_id in (".implode(",",$addresses['right']).") AND is_opp = ".round(!$is_opp).")" : "0").
			") AND infraction_code IN (".implode(",",$codes).")";
			$tickets_request = mysql_query($tickets_query); print mysql_error();
			//print $tickets_query ."\n". mysql_num_rows($tickets_request) . "\n";
			while (($ticket = mysql_fetch_object($tickets_request)))
			{
				$wday = $ticket->wday;
				$code = $ticket->infraction_code;
				$offset = time_to_offset($ticket->time_of_infraction);
				
				if (!isset($infractions[$code][$wday]))
					$infractions[$code][$wday] = str_pad("", 48, " ");
				
				if (ord($infractions[$code][$wday][$offset]) < 126)
					$infractions[$code][$wday][$offset] = chr(ord($infractions[$code][$wday][$offset]) + 1);
				
				$tickets_all_day[$code][$wday]++;
			}*/
			
			foreach (array(0,1,2,3,4,5,6) as $wday)
			{
				/*foreach ($codes as $code)
				{
					if ($addresses['left'])
					foreach ($addresses['left'] as $address_id)
					{
						$tickets_query = "SELECT address_id, is_opp, date_of_infraction, time_of_infraction, wday, infraction_code, set_fine_amount FROM tickets USE INDEX (loverboy) ".
							" WHERE address_id = $address_id AND is_opp = $is_opp AND infraction_code = $code AND wday = $wday";
						$tickets_request = mysql_query($tickets_query); print mysql_error();
						
						while (($ticket = mysql_fetch_object($tickets_request)))
						{
							//$wday = $ticket->wday;
							//$code = $ticket->infraction_code;
							$offset = time_to_offset($ticket->time_of_infraction);
					
							if (!isset($infractions[$code][$wday]))
								$infractions[$code][$wday] = str_pad("", 48, " ");
					
							if (ord($infractions[$code][$wday][$offset]) < 126)
								$infractions[$code][$wday][$offset] = chr(ord($infractions[$code][$wday][$offset]) + 1);
					
							$tickets_all_day[$code][$wday]++;
						}
					}
					
					if ($addresses['right'])
					foreach ($addresses['right'] as $address_id)
					{
						$tickets_query = "SELECT address_id, is_opp, date_of_infraction, time_of_infraction, wday, infraction_code, set_fine_amount FROM tickets USE INDEX (loverboy) ".
							" WHERE address_id = $address_id AND is_opp = ".round(!$is_opp)." AND infraction_code = $code AND wday = $wday";
						$tickets_request = mysql_query($tickets_query); print mysql_error();
						
						while (($ticket = mysql_fetch_object($tickets_request)))
						{
							//$wday = $ticket->wday;
							//$code = $ticket->infraction_code;
							$offset = time_to_offset($ticket->time_of_infraction);
					
							if (!isset($infractions[$code][$wday]))
								$infractions[$code][$wday] = str_pad("", 48, " ");
					
							if (ord($infractions[$code][$wday][$offset]) < 126)
								$infractions[$code][$wday][$offset] = chr(ord($infractions[$code][$wday][$offset]) + 1);
					
							$tickets_all_day[$code][$wday]++;
						}
					}
				}*/
				
				if ($addresses['left'])
				{
					$tickets_query = "SELECT address_id, is_opp, date_of_infraction, time_of_infraction, wday, infraction_code, set_fine_amount FROM tickets USE INDEX (loverboy) WHERE ".
						" " . ($addresses['left'] ? "(address_id in (".implode(",",$addresses['left']).") AND is_opp = ".round($is_opp).")" : "0").
						" AND infraction_code IN (".implode(",",$codes).") AND wday = $wday";
					$tickets_request = mysql_query($tickets_query); print mysql_error();
					//print $tickets_query ."\n". mysql_num_rows($tickets_request) . "\n";
					while (($ticket = mysql_fetch_object($tickets_request)))
					{
						$wday = $ticket->wday;
						$code = $ticket->infraction_code;
						$offset = time_to_offset($ticket->time_of_infraction);
					
						if (!isset($infractions[$code][$wday]))
							$infractions[$code][$wday] = str_pad("", 48, " ");
					
						if (ord($infractions[$code][$wday][$offset]) < 126)
							$infractions[$code][$wday][$offset] = chr(ord($infractions[$code][$wday][$offset]) + 1);
					
						$tickets_all_day[$code][$wday]++;
					}
				}
				
				if ($addresses['right'])
				{
					$tickets_query = "SELECT address_id, is_opp, date_of_infraction, time_of_infraction, wday, infraction_code, set_fine_amount FROM tickets USE INDEX (loverboy) WHERE ".
						" " . ($addresses['right'] ? "(address_id in (".implode(",",$addresses['right']).") AND is_opp = ".round(!$is_opp).")" : "0").
						" AND infraction_code IN (".implode(",",$codes).") AND wday = $wday";
					$tickets_request = mysql_query($tickets_query); print mysql_error();
					//print $tickets_query ."\n". mysql_num_rows($tickets_request) . "\n";
					while (($ticket = mysql_fetch_object($tickets_request)))
					{
						$wday = $ticket->wday;
						$code = $ticket->infraction_code;
						$offset = time_to_offset($ticket->time_of_infraction);
					
						if (!isset($infractions[$code][$wday]))
							$infractions[$code][$wday] = str_pad("", 48, " ");
					
						if (ord($infractions[$code][$wday][$offset]) < 126)
							$infractions[$code][$wday][$offset] = chr(ord($infractions[$code][$wday][$offset]) + 1);
					
						$tickets_all_day[$code][$wday]++;
					}
				}
			}
			//print_r($tickets);
			//print_r($infractions);
			//print_r($tickets_all_day);
			
			foreach ($codes as $code)
			{
				if (!isset($tickets_all_day[$code])) continue;
				for ($wday = 0 ; $wday <= 6 ; $wday++)
				{
					if (!isset($tickets_all_day[$code][$wday])) continue;
					mysql_query("INSERT INTO ticket_day_infractions_for_geo_id ".
						"(geo_id, is_opp, wday, infraction_code, tickets_all_day, tickets_half_hourly) VALUES ".
						"($geo_id, $is_opp, $wday, $code, ".$tickets_all_day[$code][$wday].", \"".mysql_real_escape_string($infractions[$code][$wday])."\")\n");
						print mysql_error();
				}
			}
		}
	}
	if ($count > $r_streets_count / 100) { print ","; $count = 0; }
	print ".";
	$count++;
}


die();
/*
$wdays = array(0, 1, 2, 3, 4, 5, 6);
$codes = array(5, 29, 207);

$r_streets = mysql_query("SELECT geo_id, lf_name FROM mb.centreline_oct2012 WHERE LF_NAME = 'RICHMOND ST W' GROUP BY geo_id");
while ((list($street_geo_id, $lf_name) = mysql_fetch_row($r_streets)))
{
	$points = array();
	$r_points = mysql_query("SELECT x, y FROM mb.centreline_oct2012 WHERE geo_id = $street_geo_id ORDER BY id ASC");
	while (($point = mysql_fetch_object($r_points))) $points[] = $point;
	
	foreach ($points as $i => $point)
	{
		if (!$points[$i+1]) break;
		$angle = atan2($points[$i]->y - $points[$i+1]->y, $points[$i]->x - $points[$i+1]->x);
		print $angle . "\n";
	}
	print $angle . "\n";
	
	print_r($points);
	break;
	*/
	/*$addresses = array();
	$q = "SELECT id, ADDRESS, street_x, street_y, arc_side, kevins_distance, num_tickets ".
				" FROM mb.addresses where lf_name = \"".mysql_real_escape_string($lf_name)."\" ".
				" ORDER BY kevins_id, kevins_distance";
  $r_addresses = mysql_query($q); print mysql_error();
  while (($address = mysql_fetch_object($r_addresses)))
	{
		$addresses[] = $address;
	}
	
	$data = array();
	foreach($addresses as $i => $address)
	{
		$addresses[$i]->tickets = 
			mysql_fetch_row(
				mysql_query(
					"SELECT tickets_all_day, tickets_half_hourly FROM ticket_day_infractions ".
					"WHERE address_id = $address->id AND is_opp = 1 AND wday = 2 AND infraction_code = 29"));
					print mysql_error();
		
	}*/
	/*
	
	
	//print_r($data);
  //print_r($addresses);
}

die();*/
/*
{
  $r_addresses = mysql_query(
      "SELECT *, FLOOR(FULL_ADDRESS)%2 AS side "
     ." FROM mb.addresses "
     ." WHERE lf_name = \"".mysql_real_escape_string($lf_name)."\" AND FLOOR(FULL_ADDRESS)%2 = 0 "
     ." ORDER BY numeric_address LIMIT 1"
    );
  $first_address = mysql_fetch_object($r_addresses); print mysql_error();
  
  $addresses = array($first_address);
  $num_tickets = $first_address->num_tickets;
  $left_address_id = $first_address->left_address_id;
  
  while ($num_tickets < 1680) // 48(halfhours) * 7(wdays) * 5(infractions)
  {
    $r = mysql_query("SELECT * FROM mb.addresses WHERE id = $left_address_id LIMIT 1"); print mysql_error();
    if (mysql_num_rows($r) == 0) break;
    $next_address = mysql_fetch_object($r);
    if (!$next_address) break;
    $addresses[] = $next_address;
    $num_tickets += $next_address->num_tickets;
    $left_address_id = $next_address->left_address_id;
    if (!$next_address->left_address_id) break;
  }
  
  $num_tickets_total = 0;
  $tickets_all_day = array();
  
  $blank_day = str_repeat(' ', 48);
  
  foreach ($addresses as $address)
  {
    for ($wday = 0 ; $wday <= 6 ; $wday++)
    {
      foreach (array(1, 5, 8, 9, 29, 207, 210, 264, 312) as $infraction_code)
      {
        if (!isset($tickets_all_day[$wday." ".str_pad($infraction_code,3)])) $tickets_all_day[$wday." ".str_pad($infraction_code,3)] = $blank_day;
        
        $q = "SELECT * FROM ticket_day_infractions WHERE address_id = $address->id AND is_opp = 0 AND wday = $wday AND infraction_code = $infraction_code";
        $r = mysql_query($q); print mysql_error();
        while (($tdi = mysql_fetch_object($r)))
        {
          $num_tickets_total += $tdi->tickets_all_day;
          $tickets_all_day[$wday." ".str_pad($infraction_code,3)]
            = add_ticket_day_infractions($tickets_all_day[$wday." ".str_pad($infraction_code,3)], $tdi->tickets_half_hourly);
          //print_r($tdi->tickets_all_day . " '".$tdi->tickets_half_hourly."'\n");
        }
      }
    }
    //break;
  }
  
  function fuck ($a, $b)
  {
    $a2 = explode(" ", $a);
    $b2 = explode(" ", $b);
    if ($a2[1] == $b2[1]) 
         return $a2[0] < $b2[0] ? -1 : 1;
    else return $a2[1] < $b2[1] ? -1 : 1;
  }
  
  uksort($tickets_all_day, "fuck");
  
  foreach ($tickets_all_day as $key => $tad)
  {
    $tickets_all_day[$key] = $tickets_all_day[$key] . "|" . str_pad(48-strlen(ltrim($tickets_all_day[$key])),2) . "|" . strlen(rtrim($tickets_all_day[$key]));
  }
  
  print "num_tickets_total = $num_tickets_total\n";
  print count($addresses) . "\n";
  print_r($tickets_all_day);
  
  die();
}
*/



























?>