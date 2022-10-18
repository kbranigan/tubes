
#ifndef COORDINATE_HELPERS_H
#define COORDINATE_HELPERS_H

struct ellipsoid {
  double axis;
  double scaleTm;
  double eastingOrg;
  double eccentricity;
};

struct latlon {
  double lat;
  double lon;
};

struct xtmcoord {
  double easting;
  double northing;
  int xtm;             // 0 = utm, 1 = mtm
  int zonenumber;      // zone number - http://www.geod.rncan.gc.ca/images/utm.jpg or http://www.geod.rncan.gc.ca/images/mtm.jpg
  char zoneletter;     // only for utm (UTM Zone Letter, T for toronto area - http://www.dmap.co.uk/utmworld.htm)
  double ref_meridian; // only for mtm (Reference Meridian)
};

struct mtmZoner {
  int zonenumber;
  double refMeridian;
};

void get_geo_constants(struct ellipsoid * e, int ellipsoid_id, int xtm);
struct mtmZoner gsrugZoner(double mtmLat, double mtmLong, int mtmZone);
char get_zoneletter(double lat);
void get_latlon(struct latlon * ll, struct xtmcoord c, struct ellipsoid e);

#endif
