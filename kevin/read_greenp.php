#!/usr/bin/php
<?php

$locations = json_decode(file_get_contents("data/greenp.locations.ugly.json"), true);

mysql_connect("localhost", "root", "");
mysql_select_db("mb");

$addresses_table = "addresses_oct2012";

//print(count($locations['carparks']));
//print_r($locations['carparks'][0]);

$max_lengths = array();
$is_int = array();
$is_float = array();

foreach ($locations['carparks'] as $i => $carpark)
{
	$fields = array();
	$values = array();
	
	foreach ($carpark as $field => $value)
	{
		$value = mysql_real_escape_string($value);
		
		$fields[] = $field;
		$values[] = $value;
		if (!isset($max_lengths[$field]) || $max_lengths[$field] < strlen($value)) $max_lengths[$field] = strlen($value);
		if (!isset($is_int[$field])) $is_int[$field] = 1;
		if (!is_numeric($value)) $is_int[$field] = 0;
		if (strpos($value,'.')!==false) $is_int[$field] = 0;
		if (!isset($is_float[$field])) $is_float[$field] = 1;
		if (!is_numeric($value)) $is_float[$field] = 0;
		if (strpos($value,'.')===false) $is_float[$field] = 0;
	}
}

mysql_query("DROP TABLE IF EXISTS carparks;\n"); print mysql_error();
$q = "";
foreach ($max_lengths as $field => $length) {
	if ($field == 'id')
	{
		$q .= "  id INT PRIMARY KEY AUTO_INCREMENT, `address_id` INT";
	}
	else
	{
		if ($is_int[$field])				$q .= ", `$field` INT(".($length+1).")";
		else if ($is_float[$field])	$q .= ", `$field` FLOAT(11, 6)";
		else												$q .= ", `$field` VARCHAR(".($length+1).")";
	}
}
mysql_query("CREATE TABLE carparks (" . $q . ")"); print mysql_error();

foreach ($locations['carparks'] as $i => $carpark)
{
	$fields = array();
	$values = array();
	
	foreach ($carpark as $field => $value)
	{
		$value = mysql_real_escape_string($value);
		$fields[] = $field;
		if (!$is_int[$field] && !$is_float[$field])
			$values[] = "\"$value\"";
		else
			$values[] = $value;
		
		if ($field == 'address' && substr($value,0,3) != 'TTC')
		{
			$tail_replacements = array(
				"Street" => "St",
				"Street East" => "St E",
				"Street West" => "St W",
				"." => "",
				"Avenue" => "Ave",
				"Avenue East" => "Ave E",
				"Avenue West" => "Ave W",
				"Place" => "Pl",
				"Boulevard" => "Blvd",
				"Road" => "Rd",
				"Drive" => "Dr",
				"Park" => "Pk",
				"Cresent" => "Cres",
				"Gardens" => "Gdns",
				"Circle" => "Crcl",
				"Blvd. E. (Parks)" => "Blvd E",
				"Blvd. W. (Parks)" => "Blvd W",
				"Blvd. West (Parks)" => "Blvd W",
				"Blvd West (Parks)" => "Blvd W",
				"Road (Bluffer\\'s Park)" => "Rd",
				"Street - Toronto Coach Terminal" => "St",
				"Lakeshore Blvd. West (Parks)" => "Lake Shore Blvd W",
				"Lakeshore Blvd. W. (Parks)" => "Lake Shore Blvd W",
				"Lakeshore Blvd West (Parks)" => "Lake Shore Blvd W",
			);
			
			if ($value == "65 Colonel Samuel Smith Park")							$value = "65 Colonel Samuel Smith Park Dr";
			if ($value == "20 St. Andrew Street (Kensington Garage)")	$value = "20 St Andrew St";
			if ($value == "110 Queen Street West (Nathan Phillips Square Garage)") $value = "110 Queen St W";
			if ($value == "2 Church Street (St. Lawrence Garage)")		$value = "2 Church St";
			if ($value == "23 Bedford Park Avenue West")							$value = "23 Bedford Pk Ave W";
			if ($value == "9 Bedford Road (Hotel Intercontinental)")	$value = "9 Bedford Rd";
			if ($value == "11 Finch West")														$value = "11 Finch Ave W";
			if ($value == "23 Bedford Park Avenue West")							$value = "23 Bedford Park Ave";
			
			if ($value == "1675 Lakeshore Blvd. E. (Parks)")					$value = "1675 Lake Shore Blvd E";
			if ($value == "20 Ashbridges Bay Park (Parks)")						$value = "20 Ashbridges Bay Park Rd";
			if ($value == "711 Lakeshore Blvd W")											$value = "711 Lake Shore Blvd W";
			if ($value == "31 Blackthorne Ave.")											$value = "31 Blackthorn Ave";
			if ($value == "745 Ossington Avenue, 16 Carling Avenue")	$value = "745 Ossington Ave";
			
			if ($value == "700 St. Clair Ave")												$value = "700 St Clair Ave W";
			if ($value == "23 Bedford Park Avenue West")							$value = "23 Bedford Park Ave";
			
			$value = str_replace(" St. ", " St ", $value); // St. Patrick, St. Clair
			$value = str_replace(" Mt. ", " Mount ", $value); // Mt. Pleasant
			
			//print "$value\n";
			$q = "SELECT id FROM $addresses_table WHERE FULL_ADDRESS = \"" . $value . "\"";
			if ($debug) print "$q\n";
			list($address_id) = mysql_fetch_row(mysql_query($q)); print mysql_error();
			
			if ($address_id == NULL)
			{
				$qs = array();
				foreach ($tail_replacements as $tail => $replace)
				{
					$tempvalue = $value;
					if (substr($value, -strlen($tail)) == $tail) $tempvalue = substr($tempvalue, 0, -strlen($tail)) . $replace;
					$q = "SELECT id FROM $addresses_table WHERE FULL_ADDRESS = \"" . $tempvalue . "\"";
					if ($tempvalue == $value) continue;
					if ($debug) print "$q\n";
					$qs[] = $q;
					list($address_id) = mysql_fetch_row(mysql_query($q)); print mysql_error();
					
					if ($address_id != NULL) break;
				}
				foreach ($tail_replacements as $tail => $replace)
				{
					$tempvalue = $value;
					if (substr($value, -strlen($tail)) == $tail) $tempvalue = substr($tempvalue, 0, -strlen($tail)) . $replace;
					if ($tempvalue == $value) continue;
					$q = "SELECT id FROM $addresses_table WHERE FULL_ADDRESS LIKE \"" . $tempvalue . "%\"";
					if ($debug) print "$q\n";
					$qs[] = $q;
					list($address_id) = mysql_fetch_row(mysql_query($q)); print mysql_error();
					
					if ($address_id != NULL) break;
				}
				if ($address_id == NULL && count($qs) > 0) { print_r($qs); }
			}
			
			if ($address_id != NULL)
			{
				$fields[] = "address_id";
				$values[] = $address_id;
				if ($debug) print("address_id = $address_id\nhuh\n");
			}
		}
	}
	
	$q = "INSERT INTO carparks (`".implode("`,`", $fields)."`) VALUES (".implode(",", $values).")";
	if ($debug) print "$q\n";
	mysql_query($q); print mysql_error();
}














?>

