<?php

if ($_GET['command'] == 'deletepolyline') {
	mysql_connect("localhost", "root", "");
	mysql_select_db("mb");
	list ($is_opp, $geo_id) = explode("-", $_GET['id']);

	$arc_side = ($is_opp == 1) ? 'L' : 'R';

	mysql_query("DELETE FROM address_lines WHERE geo_id = $geo_id AND arc_side = \"$arc_side\""); print mysql_error();
} else if ($_GET['command'] == 'saveblacklist') {
	mysql_connect("localhost", "root", "");
	mysql_select_db("mb");

	list ($is_opp, $geo_id) = explode("-", $_GET['id']);

	foreach (explode(",", $_GET['wdays']) as $wday) {
		$q = "SELECT id FROM ticket_day_infractions_for_geo_id WHERE geo_id = $geo_id AND is_opp = \"$is_opp\" AND infraction_code = 0 AND wday = $wday LIMIT 1";
		$r = mysql_query($q); print mysql_error();
		$n = mysql_num_rows($r);
		if ($n == 0) {
			$q = "INSERT INTO ticket_day_infractions_for_geo_id (geo_id, is_opp, wday, infraction_code, tickets_all_day, tickets_half_hourly) VALUES ".
			"($geo_id, $is_opp, $wday, 0, 0, \"".mysql_real_escape_string($_GET['blacklist'])."\")";
		} else {
			list($id) = mysql_fetch_row($r);
			$q = "UPDATE ticket_day_infractions_for_geo_id SET tickets_all_day = 0, tickets_half_hourly = \"".mysql_real_escape_string($_GET['blacklist'])."\" WHERE geo_id = $geo_id AND is_opp = \"$is_opp\" AND infraction_code = 0 AND wday = $wday LIMIT 1";
		}

		mysql_query($q); print mysql_error();
	}
	file_get_contents("http://".$_SERVER["SERVER_NAME"].":4512/reload_geo_id?geo_id=$geo_id&is_opp=$is_opp");

} else if ($_GET['command'] == 'savepolyline') {
	mysql_connect("localhost", "root", "");
	mysql_select_db("mb");
	
	list ($is_opp, $geo_id) = explode("-", $_GET['id']);

	$arc_side = ($is_opp == 1) ? 'L' : 'R';

	$coords = explode(":", $_GET['coords']);
	foreach ($coords as $i => $coord) {
		$coords[$i] = explode(",", $coord);
		$coords[$i][0] = round($coords[$i][0], 6);
		$coords[$i][1] = round($coords[$i][1], 6);
	}
  $q = "SELECT id, sequence, x, y FROM address_lines WHERE geo_id = $geo_id AND arc_side = \"$arc_side\" ORDER BY sequence";
	$r = mysql_query($q); print mysql_error();
	$n = mysql_num_rows($r);
	if (count($coords) != $n) {
		print "doesn't support adding or removing points";
	} else {
		while (($row = mysql_fetch_object($r))) {
			$row->x = round($row->x, 6);
			$row->y = round($row->y, 6);
			if ($row->y != $coords[$row->sequence][0] || $row->x != $coords[$row->sequence][1]) {
				$q = "UPDATE address_lines SET y = ".$coords[$row->sequence][0].", x = ".$coords[$row->sequence][1]." WHERE id = $row->id LIMIT 1";
				mysql_query($q); print mysql_error();
			}
		}
		file_get_contents("http://".$_SERVER["SERVER_NAME"].":4512/reload_geo_id?geo_id=$geo_id&is_opp=$is_opp");
	}

} else if ($_GET['command'] == 'getstreetdata') {
	mysql_connect("localhost", "root", "");
	mysql_select_db("mb");

	list ($is_opp, $geo_id) = explode("-", $_GET['id']);

	list ($lf_name) = mysql_fetch_row(mysql_query("SELECT lf_name FROM centreline_july2013 WHERE geo_id = $geo_id LIMIT 1")); print mysql_error();
	list ($fnode) = mysql_fetch_row(mysql_query("SELECT fnode FROM centreline_july2013 WHERE geo_id = $geo_id LIMIT 1")); print mysql_error();
	list ($tnode) = mysql_fetch_row(mysql_query("SELECT tnode FROM centreline_july2013 WHERE geo_id = $geo_id LIMIT 1")); print mysql_error();
	print "<b>$lf_name</b><br /><br />";
	$lf_names = array();
	$r = mysql_query("SELECT lf_name FROM centreline_july2013 WHERE fnode = $fnode AND lf_name != \"".addslashes($lf_name)."\" UNION SELECT lf_name FROM centreline_july2013 WHERE fnode = $tnode AND lf_name != \"".addslashes($lf_name)."\" UNION SELECT lf_name FROM centreline_july2013 WHERE tnode = $fnode AND lf_name != \"".addslashes($lf_name)."\" UNION SELECT lf_name FROM centreline_july2013 WHERE tnode = $tnode AND lf_name != \"".addslashes($lf_name)."\""); print mysql_error();
	while (list($temp) = mysql_fetch_row($r)) {
		$lf_names[] = $temp;
		print "<div>$temp</div>";
	}
  print "<br />";

  $total_tickets = array();
  $r = mysql_query("SELECT SUM(tickets_all_day), wday FROM ticket_day_infractions_for_geo_id WHERE geo_id = $geo_id AND is_opp = $is_opp GROUP BY wday");
  while (list($tickets_all_day, $wday) = mysql_fetch_row($r)) {
  	$total_tickets[$wday] = $tickets_all_day;
  }

  print array_sum($total_tickets) . " total ticket(s)";
  if (array_sum($total_tickets) == 0) print " - <a href='#' onClick=\"for (var i = 0; i < polylines.length; i++) { if (polylines[i].id == '".$_GET['id']."') polylines[i].setMap(null); }; $.get('?command=deletepolyline&id=".$_GET['id']."', function (data) {}); $('#street-data').html('') \">DELETE ADDRESS LINE</a>";
  print "<br /><br />";

	$wday_names = array("Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat");
	print "<div style='margin:5px'>";
	for ($wday = 0 ; $wday < 7 ; $wday++) {
		print "<span class='wday_control' style='margin:5px;padding:4px;background-color:#ddd;cursor:pointer' name='wday_$wday' onMouseOver='$(\"[highlight=yes]\").hide();$(\".wday_$wday\").show()' onMouseOut='$(\".wday_$wday"."[highlight!=yes]\").hide();$(\"[highlight=yes]\").show()' onClick='$(\".wday_control\").css(\"font-weight\",\"normal\");$(this).css(\"font-weight\",\"bold\");$(\"[highlight=yes]\").attr(\"highlight\",\"no\").hide();$(\".wday_$wday\").attr(\"highlight\",\"yes\")'>".$wday_names[$wday]."</span>";
	}
	print "</div>";
	print "<div style='font-weight:bold;padding-top:15px;white-space:pre;font:8pt Courier New'><span style='border-bottom:1px solid #DDD'>                  tickets hourly                </span>  cnt code</div>";

	$q = "SELECT * FROM ticket_day_infractions_for_geo_id WHERE geo_id = $geo_id AND is_opp = $is_opp ORDER BY infraction_code";
	$r = mysql_query($q); print mysql_error();
	while (($row = mysql_fetch_object($r))) {
		print "<div style='display:none;white-space:pre;font:8pt Courier New' class='wday_$row->wday code_$row->infraction_code' tickets_all_day='$row->tickets_all_day' id='$row->id'><span style='border-left:1px solid red;border-right:1px solid red;border-bottom:1px solid #DDD'>$row->tickets_half_hourly</span> ".str_pad($row->tickets_all_day,4," ",STR_PAD_LEFT)." ".str_pad($row->infraction_code,3," ",STR_PAD_LEFT)."</div>";
	}

	print "<br /><br />Blacklist:<br /><span style='font-size:9pt'>";
	for ($wday = 0 ; $wday < 7 ; $wday++) {
		print $wday_names[$wday] . ":<input type='checkbox' name='selected_wdays' value='$wday' onClick=\"if (!$(this).prop('checked')) $('input[name=all_wdays]').prop('checked',false) \"> &nbsp;";
	}
	print " &nbsp; All: <input type='checkbox' name='all_wdays' onClick=\"$('input[name=selected_wdays]').prop('checked', $(this).prop('checked'))\"></span><br />";
	print "<input style='white-space:pre;font:8pt Courier New;width:350px;padding:0px' maxlength='48' type='text' value='".str_repeat(" ",48)."' onKeyUp='change_black_list(this)' id='black_list' geo_id='".$_GET['id']."'><br /><div id='black_list_text'></div><script type='text/javascript'>change_black_list($('#black_list'))</script><input type='button' value='save blacklist' onClick='saveblacklist()'> - <input type='button' onClick='$(\"#black_list\").val(\"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\");$(\"input[name=selected_wdays]\").prop(\"checked\",true)' value='pre-fill blacklist (does not save)'>";

} else if ($_GET['command'] == 'search') {
	print file_get_contents("http://".$_SERVER["SERVER_NAME"].":4512/near?" . $_SERVER["QUERY_STRING"]);
} else {
	?>
	<script type='text/javascript' src='jquery-1.10.2.min.js'></script>
	<script src="https://maps.googleapis.com/maps/api/js?v=3.exp&sensor=false"></script>
	<script type='text/javascript'>

	var MY_MAPTYPE_ID = "yay";
	var mapStyle = [
	  {
	    "featureType": "poi",
	    "stylers": [
	      { "visibility": "off" }
	    ]
	  },{
	    "featureType": "transit",
	    "stylers": [
	      { "visibility": "off" }
	    ]
	  },{
	    "featureType": "transit"  }
	];

	var map;
	var polylines = [];

	function initialize() {
	  var mapOptions = {
	    zoom: 18,
	    center: new google.maps.LatLng(43.647456373163, -79.395918558308),
	    mapTypeControlOptions: {
	      mapTypeIds: [MY_MAPTYPE_ID] // google.maps.MapTypeId.ROADMAP
	    },
	    mapTypeId: MY_MAPTYPE_ID
	  };

	  map = new google.maps.Map(document.getElementById('map-canvas'), mapOptions);

	  var styledMapOptions = {
	    name: 'Custom Style'
	  };

	  var customMapType = new google.maps.StyledMapType(mapStyle, styledMapOptions);

	  map.mapTypes.set(MY_MAPTYPE_ID, customMapType);

		google.maps.event.addDomListener(map, 'dragend', mapDragEnd);
		mapDragEnd();
	}
	function lineClick() {
		for (var i in polylines) {
			polylines[i].setEditable(false);
		}
		this.setEditable(true);
		$('#street-data').html("<b>" + this.id + "</b><br /><pre>");
		$.get("?command=getstreetdata&id="+this.id, function (data) {
			$('#street-data').append(data);
		});
	}

	function mapDragEnd() {
		var center = map.getCenter();
		var now = $("#now")[0].value;
		$.get("?command=search&v=1.0&payment=Free&lat="+center.lat()+"&lng="+center.lng()+"&now="+now, function (data) {
			eval("var data = " + data);
			$('#now-string').html(data.params.now_string);
		  for (var i = 0; i < polylines.length; i++) {
		    polylines[i].setMap(null);
		  }
		  polylines = [];
			$.each(data.address_ranges, function (index, o) {
				for (i in o.polyline) {
					o.polyline[i] = new google.maps.LatLng(o.polyline[i][0], o.polyline[i][1]);
				}
				o.polyline.parent = o;

				var value = o.tickets[$('input:radio[name=t]:checked').val()];

				var path = new google.maps.Polyline({
					path: o.polyline,
			    strokeColor: 'rgb('+(value*255)+','+(255-value*255)+',0)',
			    strokeOpacity: 1.0,
			    strokeWeight: 4
				});
				path.setMap(map);
				polylines.push(path);
				path.id = o.id;
				google.maps.event.addDomListener(path, 'click', lineClick);
				var sigh = function (e) {
					var p = path.getPath();
					coords = '';
					for (var i = 0 ; i < p.getLength() ; i++) {
						if (i != 0) coords += ':';
						coords += p.getAt(i).lat().toFixed(6) + ',' + p.getAt(i).lng().toFixed(6);
					}
					$.get("?command=savepolyline&id="+path.id+"&coords="+coords, function (data) {
						if (data != '') {
							alert(data);
						}
					});
				};
				//google.maps.event.addDomListener(path.getPath(), 'insert_at', lineModified);
				//google.maps.event.addDomListener(path.getPath(), 'remove_at', lineModified);
				google.maps.event.addDomListener(path.getPath(), 'set_at', sigh);
			});
		});
	}

	function change_black_list(o) {
		var text = $('#black_list').val();
		if (text.length != 48) {
			$('#black_list_text').html('length is: ' + text.length)
			return;
		}
		if (text == '                                                ') {
			$('#black_list_text').html('no blacklist given')
			return;
		}
		
		var first = -1;
		for (var i = 0 ; i < text.length ; i++) {
			if (text[i] != ' ') { first = i; break; }
		}
		
		var last = -1;
		for (var i = text.length-1 ; i >= 0 ; i--) {
			if (text[i] != ' ') { last = i+1; break; }
		}

		var fhour = Math.floor(first/2);
		var lhour = Math.floor(last/2);
		var fampm = fhour == 24 ? "am" : (fhour < 12 ? "am" : "pm");
		var lampm = lhour == 24 ? "am" : (lhour < 12 ? "am" : "pm");
		while (fhour > 12) fhour -= 12;
		while (lhour > 12) lhour -= 12;
		if (fhour == 0) fhour = 12;
		if (lhour == 0) lhour = 12;
		var fstring = fhour + ":" + ((Math.round(first/2) != Math.floor(first/2)) ? "30" : "00") + fampm;
		var lstring = lhour + ":" + ((Math.round(last/2) != Math.floor(last/2)) ? "30" : "00") + lampm;

		$('#black_list_text').html(fstring + " to " + lstring)
	}
	function saveblacklist() {
		var wdays = "&wdays=";
		$('input[name=selected_wdays]:checked').each(function(i, o) {
			if (wdays != '&wdays=') wdays += ",";
			wdays += o.value;
		});
		if (wdays == '&wdays=') {
			alert('at least one week day needs to be selected');
			return;
		}
		var url = "?command=saveblacklist&id=" + $('#black_list').attr('geo_id') + "&blacklist=" + encodeURI($('#black_list').val()) + wdays;
		$.get(url, function (data) {
			if (data != '') {
				alert(data);
			}
			mapDragEnd();
		});

	}

	google.maps.event.addDomListener(window, 'load', initialize);
	</script>
	<body style="padding:0px;margin:0px">
	Now: <input type='text' id='now' value='1383015600' onKeyUp='mapDragEnd()' /> <input type='radio' checked name='t' value='0' onClick='mapDragEnd()'><input type='radio' name='t' value='1' onClick='mapDragEnd()'><input type='radio' name='t' value='2' onClick='mapDragEnd()'> <span id='now-string'></span>
	<div id="map-canvas" style="height:95%;padding:0px;margin:0px;width:60%"></div>
	<div id="street-data" style="position:absolute;top:0px;right:0px;height:100%;width:450px">click a line</div>
	<?php
}

?>
