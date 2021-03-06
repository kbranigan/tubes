
#include "block.h"

#include "coordinate_helpers.h"

int main(int argc, char ** argv)
{
	if (stdout_is_piped()) // other wise you don't see the seg fault
		setup_segfault_handling(argv);
	
	assert_stdin_is_piped();
	assert_stdout_is_piped();
	//assert_stdin_or_out_is_piped();
	
	int zoneletter = 0;
	int zonenumber = 10;
	int xtm = 1; // 0=utm, 1=mtm
	char ellipsoid_string[10] = "nad27"; // silly toronto
	int ref_meridian = 0;
	
	struct ellipsoid e;
	//get_geo_constants(&e, 5, xtm); // set by nad27 or wgs84
	
	struct Params * params = NULL;
	params = add_string_param(params, "ellipsoid", 'e', ellipsoid_string, 0);
	params = add_int_param(params, "letter", 'l', &zoneletter, 0);
	params = add_int_param(params, "number", 'n', &zonenumber, 0);
	params = add_int_param(params, "xtm", 'x', &xtm, 0);
	params = add_int_param(params, "meridian", 'm', &ref_meridian, 0);
	eval_params(params, argc, argv);
	
	if (strcmp(ellipsoid_string, "nad27") == 0 || strcmp(ellipsoid_string, "NAD27") == 0) get_geo_constants(&e, 5, xtm);
	else if (strcmp(ellipsoid_string, "nad83") == 0 || strcmp(ellipsoid_string, "NAD83") == 0) get_geo_constants(&e, 22, xtm);
	else if (strcmp(ellipsoid_string, "wgs84") == 0 || strcmp(ellipsoid_string, "WGS84") == 0) get_geo_constants(&e, 22, xtm);
	else
	{
		fprintf(stderr, "invalid ellipsoid, second option should be 'nad27' or 'wgs84' (was %s)\n", ellipsoid_string);
		exit(1);
	}
	
	struct Block * block = NULL;
	while ((block = read_block(stdin)))
	{
		int i;
		for (i = 0 ; i < block->num_rows ; i++)
		{
			double x = get_x(block, i);
			double y = get_y(block, i);
			
			struct xtmcoord c;
			c.easting = x;
			c.northing = y;
			c.zonenumber = zonenumber;
			c.zoneletter = zoneletter;
			c.xtm = xtm;
			c.ref_meridian = ref_meridian;
			
			struct latlon ll;
			get_latlon(&ll, c, e);
			
			set_xy(block, i, ll.lat, ll.lon);
			
			//fprintf(stderr, "%f,%f = %f,%f\n", x, y, ll.lat, ll.lon);
			//vertex[0] = ll.lat;
			//vertex[1] = ll.lon;
			
			//ll.lat = vertex[0];
			//ll.lon = vertex[1];
			
			//struct xtmcoord c = get_xtm(ll, xtm, e, ref_meridian);
			//vertex[0] = c.easting;
			//vertex[1] = c.northing;
		}
		
		write_block(stdout, block);
		free_block(block);
	}
}
