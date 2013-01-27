#!/usr/bin/php
<?php


mysql_connect("localhost", "root", "");
mysql_select_db("mb");

/*

CREATE TABLE IF NOT EXISTS ticket_day_infractions (id int primary key auto_increment, 
address_id int, 
is_opp int, 
wday int, 
infraction_code int, 
tickets_all_day int, 
tickets_half_hourly varchar(48));

*/

$infraction_codes = array(5,8,9,29,264,    1,207,210,312); // kbfu
//$infraction_codes_result = mysql_query('select infraction_code from (select count(*) c, infraction_code from tickets group by infraction_code) e where c > 1000');
//while ((list($code)=mysql_fetch_row($infraction_codes_result))) $infraction_codes[] = $code;
//$infraction_codes[] = implode(',',$infraction_codes); // catch all

$minute_ranges = array();
for ($i = 0 ; $i < 48 ; $i++)
{
	$minute_start = $i*30;
	$minute_stop = $i*30+30;
	$minute_start = floor($minute_start/60)*100 + ($minute_start%60);
	$minute_stop = floor($minute_stop/60)*100 + ($minute_stop%60) - ((($minute_stop%60)%100)==0?41:1);
	$minute_ranges[$i] = array(str_pad($minute_start, 2, '0', STR_PAD_LEFT), str_pad($minute_stop, 2, '0', STR_PAD_LEFT));
}

$symbols = " .,-=+*%0#";
//$symbols = "0123456789";

print "truncate ticket_day_infractions;\n";

$insert_header = 'INSERT INTO ticket_day_infractions (address_id, is_opp, wday, infraction_code, tickets_all_day, tickets_half_hourly) VALUES ';
//$insert = $insert_header;
//$insert_count = 0;
$r = mysql_query('SELECT id FROM addresses');// WHERE LF_NAME = "CAMDEN ST"');// AND ROUND(FULL_ADDRESS)%2=0'); // kbfu
while ((list($address_id) = mysql_fetch_row($r)))
{
	list($c) = mysql_fetch_row(mysql_query('select count(*) from tickets where address_id = '.$address_id)); print mysql_error();
	if ($c == 0) continue;
	for ($wday = 0 ; $wday <= 6 ; $wday++)
	{
		//if ($wday != 6) continue; // kbfu
		
		$q = 'select count(*) from tickets where address_id = '.$address_id.' and wday = '.$wday;
		list($c) = mysql_fetch_row(mysql_query($q)); print mysql_error();
		if ($c == 0) continue;
		foreach ($infraction_codes as $code)
		{
			//if ($code != 210) continue; // kbfu
			
			for ($is_opp = 0 ; $is_opp <= 1 ; $is_opp++)
			{
				$q = 'select count(*) from tickets where address_id = '.$address_id.
							' and wday = '.$wday.' and infraction_code in ('.$code.') and is_opp = '.$is_opp;
				list($c) = mysql_fetch_row(mysql_query($q)); print mysql_error();
				if ($c == 0) continue;
				
				//if ($insert_count != 1) $insert .= ',';
				//$insert_count++;
				
				$tickets_all_day = 0;
				$tickets_half_hourly = array();
				$max_tickets_half_hourly = 0;
				for ($i = 0 ; $i < 48 ; $i++)
				{
					$q = 'select count(*) from tickets where address_id = '.$address_id.
								' and wday = '.$wday.' and infraction_code in ('.$code.') and is_opp = '.$is_opp.
								' and time_of_infraction >= '.$minute_ranges[$i][0].' and time_of_infraction < '.$minute_ranges[$i][1];
					list($c) = mysql_fetch_row(mysql_query($q)); print mysql_error();
					
					if ($c > $max_tickets_half_hourly) $max_tickets_half_hourly = $c;
					$tickets_all_day += (int)$c;
					$tickets_half_hourly[$i] = (int)($c);
				}
				
				//$relative = "000000000000000000000000000000000000000000000000";
				$relative = "                                                ";
				
				if ($tickets_all_day > 0)
				{
					for ($i = 0 ; $i < 48 ; $i++)
					{
						if ($tickets_half_hourly[$i] > 0)
						{
							/*print $max_tickets_half_hourly . ' ' . 
								$max_tickets_half_hourly / $tickets_half_hourly[$i] . ' ' . 
								floor($tickets_half_hourly[$i] / $max_tickets_half_hourly * 9) . 
								' "' . $symbols[floor($tickets_half_hourly[$i] / $max_tickets_half_hourly * 9)] . "\"\n";*/
							$relative[$i] = $symbols[floor($tickets_half_hourly[$i] / $max_tickets_half_hourly * 9)];
						}
					}
					print $insert_header . '(' . $address_id . ',' . $is_opp . ',' . $wday . ',' . $code . ',' . $tickets_all_day . ',"' . mysql_escape_string($relative) . '");' . "\n";
				}
				//die();
				
				//print "$is_opp $tickets_all_day $absolute\n";
			}
		}
	}
}
$insert .= ";";

//print $insert;

?>
