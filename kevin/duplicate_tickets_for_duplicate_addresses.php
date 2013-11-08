#!/usr/bin/php
<?php

mysql_connect("localhost", "root", "");
mysql_select_db("mb");


$table_date = "july2013"; // "oct2012";

$i_full_addresses = 0;
$q = "SELECT FULL_ADDRESS FROM (SELECT FULL_ADDRESS, COUNT(*) c FROM addresses_$table_date WHERE lf_name != 'JOHN ST' GROUP BY FULL_ADDRESS) e WHERE c > 1";
$r_full_addresses = mysql_query($q); print mysql_error();
$n_full_addresses = mysql_num_rows($r_full_addresses);
print "n_full_addresses = $n_full_addresses\n";
while (list($full_address) = mysql_fetch_row($r_full_addresses)) {
	$q = "SELECT id FROM addresses_$table_date WHERE FULL_ADDRESS = \"" . addslashes($full_address) . "\"";
	$r_ids = mysql_query($q); print mysql_error();
	$ids = array();
	while (list($id) = mysql_fetch_row($r_ids)) { $ids[] = $id; }
	//print_r($ids);
	print ".";

	foreach ($ids as $id) {
		$q = "SELECT address_id,is_opp,tag_number_masked,location1,location2,location3,location4,province,date_of_infraction,wday,infraction_code,set_fine_amount,time_of_infraction FROM tickets WHERE address_id = $id AND manually_duplicated IS NULL";
		$r_tickets = mysql_query($q); print mysql_error();
		$n_tickets = mysql_num_rows($r_tickets);
		if ($n_tickets > 100) print $n_tickets . " ";

		while (($ticket = mysql_fetch_assoc($r_tickets))) {
			$ticket['manually_duplicated'] = 1;
			foreach ($ticket as $key => $value) {
				$ticket[$key] = addslashes($value);
			}

			foreach ($ids as $id_temp) {
				if ($id == $id_temp) continue;
				$ticket['address_id'] = $id_temp;
				if ($id_temp == 130610) continue;
				$q = "INSERT INTO tickets (".implode(',', array_keys($ticket)).") VALUES (\"".implode('","', array_values($ticket))."\")";
				mysql_query($q); print mysql_error();
				//print $q. "\n";
			}
		}
	}
	$i_full_addresses++;
	if ($i_full_addresses % 100 == 0) print "\ndone $i_full_addresses\n";
	//break;
}

?>
