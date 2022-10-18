
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "coordinate_helpers.h"

#define PI 3.14159265
#define DEG2RAD (PI/180.0)
#define RAD2DEG (180.0/PI)

// geo->xTM: val_ll3() then calc_utm()
// xTM->geo: val_utm()

void get_geo_constants(struct ellipsoid * e, int ellipsoid_id, int xtm)
{
  // returns ellipsoid values
  double ellipsoid_axis[] = {6377563.396, 6377340.189, 6378160, 6377397.155, 6378206.4, 6378249.145, 6377276.345, 6377304.063, 6378166, 6378150, 6378155, 6378160, 6378137, 6378200, 6378270, 6378388, 6378245, 6378160, 6378165, 6378145, 6378135, 6378137};
  double ellipsoid_eccen[] = {0.00667054,0.00667054,0.006694542,0.006674372,0.006768658,0.006803511,0.00637847,0.006637847,0.006693422,0.006693422,0.006693422,0.006694605,0.00669438,0.006693422,0.006693422,0.00672267,0.006693422,0.006694542,0.006693422,0.006694542,0.006694318,0.00669438};
  /* airy, mod airy, aust national, bessel 1841, clarke 1866 == NAD 27 (TBC), clarke 1880, everest, mod everest, fischer 1960, fischer 1968, mod fischer, grs 1967, grs 1980, helmert 1906, hough, int24, krassovsky, s america, wgs-60, wgs-66, wgs-72, wgs-84 */
  
  if (ellipsoid_id == 0) ellipsoid_id = 22;
  
  --ellipsoid_id; // table indexed differently
  
  assert(ellipsoid_id < sizeof(ellipsoid_axis) / sizeof(double));
  assert(ellipsoid_id < sizeof(ellipsoid_eccen) / sizeof(double));
  assert(xtm == 0 || xtm == 1);
  
  e->axis = ellipsoid_axis[ellipsoid_id];
  e->scaleTm = (xtm == 1) ? 0.9999 : 0.9996;
  e->eastingOrg = (xtm == 1) ? 304800. : 500000.;
  e->eccentricity = ellipsoid_eccen[ellipsoid_id];
}

// function adapted from NSRUG  (caution: longitudes are +ve here)
struct mtmZoner gsrugZoner(double mtmLat, double mtmLong, int mtmZone)
{
  // Adapted from FORTRAN: SUBROUTINE ZONER(SAT,SLG,mtmZone)
  // Downloaded via below URL for Online Geodetic Tools:
  //    http://www.geod.rncan.gc.ca/tools-outils/index_e.php
  //    http://www.geod.rncan.gc.ca/tools-outils/tools_info_e.php?apps=gsrug
  // Here: C:/Docume~1/pilewis/Desktop/Lew/GPS72/GSRUG/GSRUG.FOR
  
  // Includes the official NSRUG Jan2008 fixes (see emails from Pat Legree)
  
  // bounds for MTM zones 14 to 32
  double mtmDegs[] = {85.5,  88.5,  91.5, 94.5,  97.5,  100.5, 103.5, 106.5, 109.5, 112.5, 115.5, 118.5, 121.5, 124.5, 127.5, 130.5, 133.5, 136.5, 139.5, 142.5};
  
  // MTM zone to reference meridian
  // last was 142 ?!! I think it should be 141.
  // ? matches http://www.posc.org/Epicentre.2_2/DataModel/LogicalDictionary/StandardValues/coordinate_transformation.html
  double mtmSmers[] = {0., 53., 56., 58.5, 61.5, 64.5, 67.5, 70.5, 73.5, 76.5, 79.5, 82.5, 81., 84., 87., 90., 93., 96., 99., 102., 105., 108., 111., 114., 117., 120., 123., 126., 129., 132., 135., 138., 141.};
  
  // determine zone from lat/lon
  if (mtmZone == 0)
  {
    if (mtmLong > 51.5 && mtmLong <= 54.5) mtmZone = 1;
    if (mtmLong > 54.5 && mtmLong <= 57.5) mtmZone = 2;
    if (mtmLat <= 46.5 && mtmLong <= 59.5 && mtmLong > 57.5 || mtmLat > 46.5 && mtmLong <= 60. && mtmLong > 57.5) mtmZone = 3;
    if (mtmLat < 46.5 && mtmLong <= 63. && mtmLong > 59.5 || mtmLat >= 46.5 && mtmLong <= 63. && mtmLong > 60.) mtmZone = 4;
    if (mtmLong > 63. && mtmLong <= 66.5 && mtmLat <= 44.75 || mtmLong > 63. && mtmLat > 44.75 && mtmLong <= 66.) mtmZone = 5;
    if (mtmLong > 66. && mtmLat > 44.75 && mtmLong <= 69. || mtmLong > 66.5 && mtmLat <= 44.75 && mtmLong <= 69.) mtmZone = 6;
    if (mtmLong > 69. && mtmLong <= 72.) mtmZone = 7;
    if (mtmLong > 72. && mtmLong <= 75.) mtmZone = 8;
    if (mtmLong > 75. && mtmLong <= 78.) mtmZone = 9;
    if (mtmLat >  47. && mtmLong > 78. && mtmLong <= 79.5 || mtmLat <= 47. && mtmLat  > 46. && mtmLong > 78. && mtmLong <= 80.25 || mtmLat <= 46. && mtmLong > 78. && mtmLong <= 81.) mtmZone = 10;
    if (mtmLong > 81. && mtmLong <= 84. && mtmLat <= 46.) mtmZone = 11;
    if (mtmLong > 79.5  && mtmLong <= 82.5 && mtmLat > 47. || mtmLong > 80.25 && mtmLong <= 82.5 && mtmLat <= 47. && mtmLat > 46.) mtmZone = 12;
    // if (mtmLong > 82.5 && mtmLong <= 85.5 && mtmLat > 46. || mtmLong > 84. && mtmLong <= 85.5 && mtmLat <= 46.) mtmZone = 13;
    if (mtmLong > 82.5 && mtmLong <= 85.5 && mtmLat > 46.) mtmZone = 13;

    if (mtmZone == 0) // still not found, try regular Western Canada
    {
      int z;
      for (z = 0; z <= 18; ++z)
      {
        if (mtmLong > mtmDegs[z] && mtmLong <= mtmDegs[z+1])
        {
          mtmZone = z+14;
          break;
        }
      }
    }
  }

  if (mtmZone < 1 || mtmZone > 32)
  {
    fprintf(stderr, "Cannot figure out MTM zone -- outside Canada, lat=%f, lon=%f\n", mtmLat, mtmLong);
    struct mtmZoner mtmZoner;
    mtmZoner.zonenumber = 0;
    mtmZoner.refMeridian = -mtmLong;
    
    return mtmZoner;
    //var mtmZoner = {zone: 0 , refMeridian: -mtmLong};  // return something not totally insane
    //return 0;
  }
  else
  {
    struct mtmZoner mtmZoner;
    mtmZoner.zonenumber = mtmZone;
    mtmZoner.refMeridian = -mtmSmers[mtmZone];
    return mtmZoner;
    //var mtmZoner = {zone: mtmZone , refMeridian: -mtmSmers[Number(mtmZone)]};
    //return mtmZoner;
  }
}

char get_zoneletter(double lat)
{
  if ((84 >= lat) && (lat >= 72)) return 'X';
  else if ((72 > lat) && (lat >= 64)) return 'W';
  else if ((64 > lat) && (lat >= 56)) return 'V';
  else if ((56 > lat) && (lat >= 48)) return 'U';
  else if ((48 > lat) && (lat >= 40)) return 'T';
  else if ((40 > lat) && (lat >= 32)) return 'S';
  else if ((32 > lat) && (lat >= 24)) return 'R';
  else if ((24 > lat) && (lat >= 16)) return 'Q';
  else if ((16 > lat) && (lat >= 8)) return 'P';
  else if (( 8 > lat) && (lat >= 0)) return 'N';
  else if (( 0 > lat) && (lat >= -8)) return 'M';
  else if ((-8> lat) && (lat >= -16)) return 'L';
  else if ((-16 > lat) && (lat >= -24)) return 'K';
  else if ((-24 > lat) && (lat >= -32)) return 'J';
  else if ((-32 > lat) && (lat >= -40)) return 'H';
  else if ((-40 > lat) && (lat >= -48)) return 'G';
  else if ((-48 > lat) && (lat >= -56)) return 'F';
  else if ((-56 > lat) && (lat >= -64)) return 'E';
  else if ((-64 > lat) && (lat >= -72)) return 'D';
  else if ((-72 > lat) && (lat >= -80)) return 'C';
  else return 'Z'; // bad
}

struct xtmcoord get_xtm(struct latlon ll, int xtm, struct ellipsoid e, double ref_meridian)
{
  //struct ellipsoid e = geo_constants(22, xtm);
  
  //double mrm = sanitize_refMeridian();
  struct mtmZoner zonerResult;
  
  // printf("TMP calc_utm()");
  double k0 = e.scaleTm;
  double latrad = ll.lat * DEG2RAD;
  double longrad = ll.lon * DEG2RAD;
  double zonenum = floor((ll.lon + 180) / 6) + 1;
  // @dc (180 - (-70.5))/3 - 76
  if (e.scaleTm == 0.9999)
  {
    //zonenum = floor((180.0 - ll.lon) / 3.0) - 76;  // MTM, only in Quebec
    //if (zonenum < 3 || zonenum > 10)
    //  printf("MTM zone numbers only confirmed for 3-10, province of Quebec\nContinuing anyway\n");
    if (ref_meridian == 0)
    {
      zonerResult = gsrugZoner(ll.lat, -ll.lon, 0);
      zonenum = zonerResult.zonenumber;
    }
    //else
    //  zonenum = "x";
  }
  
  if (ll.lat >= 56.0 && ll.lat < 64.0 && ll.lon >= 3.0 && ll.lon < 12.0) zonenum = 32;
  // Special zones for Svalbard
  if( ll.lat >= 72.0 && ll.lat < 84.0)
  {
    if (ll.lon >= 0.0 && ll.lon < 9.0) zonenum = 31;
    else if (ll.lon >= 9.0  && ll.lon < 21.0) zonenum = 33;
    else if (ll.lon >= 21.0 && ll.lon < 33.0) zonenum = 35;
    else if (ll.lon >= 33.0 && ll.lon < 42.0) zonenum = 37;
  }

  double lonorig = (zonenum - 1) * 6 - 180 + 3;  //+3 puts origin in middle of zone
  // @dc 180 - (7+76) * 3 - 1.5
  if (e.scaleTm == 0.9999)
  {
    // lonorig = 180 - (zonenum + 76) * 3 - 1.5;
    if (ref_meridian == 0)
    {
      zonerResult = gsrugZoner(ll.lat, -ll.lon, zonenum);
      lonorig = zonerResult.refMeridian;
    }
    else
    {
      lonorig = ref_meridian;
      if (fabs(ll.lon - lonorig) > 4.)
        fprintf(stderr, "MTM ref meridian more than 4 degrees away from longitude\nContinuing anyway\n");
    }
  }
  
  double lonorigrad = lonorig * DEG2RAD;

  double eccPrimeSquared = (e.eccentricity) / (1 - e.eccentricity);

  //calculate
  double N = e.axis / sqrt(1 - e.eccentricity * sin(latrad) * sin(latrad));
  double T = tan(latrad) * tan(latrad);
  double C = eccPrimeSquared * cos(latrad) * cos(latrad);
  double A = cos(latrad) * (longrad - lonorigrad);
  double M = e.axis * ((1 - e.eccentricity / 4 - 3 * e.eccentricity * e.eccentricity / 64 - 5 * e.eccentricity * e.eccentricity * e.eccentricity / 256) * latrad - (3 * e.eccentricity / 8 + 3 * e.eccentricity * e.eccentricity / 32 + 45 * e.eccentricity * e.eccentricity * e.eccentricity / 1024) * sin(2 * latrad) + (15 * e.eccentricity * e.eccentricity / 256 + 45 * e.eccentricity * e.eccentricity * e.eccentricity / 1024) * sin(4 * latrad) - (35 * e.eccentricity * e.eccentricity * e.eccentricity / 3072) * sin(6 * latrad));

  double easting = (k0 * N * (A + (1 - T + C) * A * A * A / 6 + (5 - 18 * T + T * T + 72 * C - 58 * eccPrimeSquared) * A * A * A * A * A / 120) + e.eastingOrg);
  double northing = (k0 * (M + N * tan(latrad) * (A * A / 2 + (5 - T + 9 * C + 4 * C * C) * A * A * A * A / 24 + (61 - 58 * T + T * T + 600 * C - 330 * eccPrimeSquared) * A * A * A * A * A * A / 720)));
  if (ll.lat < 0)
    northing += 10000000.0; //10000000 meter offset for southern hemisphere
  
  easting = easting;
  northing = northing;
  
  struct xtmcoord c;
  c.easting = easting;
  c.northing = northing;
  c.zonenumber = zonenum;
  c.zoneletter = get_zoneletter(ll.lat);
  c.xtm = xtm;
  c.ref_meridian = ref_meridian;
  
  return c;
  
  //printf("%f %f %f %f\n", lat, lon, zonenum, zonerResult.refMeridian);
  //printf("easting = %f\n", easting);
  //printf("northing = %f\n", northing);
  //printf("zone = %f\n", zonenum);
}

void get_latlon(struct latlon * ll, struct xtmcoord c, struct ellipsoid e) //double easting, double northing, double zone, int xtm, double mrm)
{
  if (c.zonenumber < 0 || c.zonenumber > 60)
  {
    fprintf(stderr, "Zone number is out of range.\nMust be between 1 and 60.\n");
    return;
  }
  if (c.northing < 0 || c.northing > 10000000)
  {
    fprintf(stderr, "Northing value is out of range (%f)\n", c.northing);
    return;
  }
  if (c.ref_meridian == 0 && (c.easting < 160000 || c.easting > 834000))
  {
    fprintf(stderr, "Easting value is out of range\n");
    return;
  }
  
  //struct ellipsoid e = geo_constants(22, c.xtm);
  
  /*if (zone == "" && ref_meridian == 0 || (zl == 0 && ellipsoid.eastingOrg != 304800.) )  // MTM does not need zone letters
  {
    printf("You must enter a zone number and letter");
    return;
  }*/
  if (e.axis == 0)
  {
    fprintf(stderr, "Ellipsoid must be entered\n");
    return;
  }
  if (c.xtm == 1 && c.ref_meridian == 0 && e.scaleTm == 0.9999 && (c.zonenumber < 1 || c.zonenumber > 32))
    fprintf(stderr, "MTM zone numbers only confirmed for 1-32, in Canada\nContinuing anyway\n");
  
  double axis = e.axis;
  double eccent = e.eccentricity;
  double scaleTm = e.scaleTm;
  double eastingOrg = e.eastingOrg;
  double k0 = scaleTm;

  double e1 = (1 - sqrt(1 - eccent)) / (1 + sqrt(1 - eccent));
  double x = c.easting - eastingOrg; //remove 500,000 meter offset for longitude
  double y = c.northing;
  
  if (c.zoneletter < 'N' && c.zoneletter != ' ' && c.xtm == 0)
    y -= 10000000.0; //remove 10,000,000 meter offset used for southern hemisphere
  
  struct mtmZoner zonerResult;
  
  double longorig = (c.zonenumber - 1) * 6 - 180 + 3;  //+3 puts origin in middle of zone
  // @dc 180 - (7+76) * 3 - 1.5
  if (scaleTm == 0.9999)
  {
    // longorig = 180 - (zone*1 + 76) * 3 - 1.5;  // without hack, did string concat !!!
    if (c.ref_meridian == 0)
    {
      zonerResult = gsrugZoner(0., 0., c.zonenumber);
      longorig = zonerResult.refMeridian;
    }
    else
      longorig = c.ref_meridian;
  }

  // printf("zone = " + (zone*1 + 76));
  // printf("longorig = " + longorig);

  double eccPrimeSquared = (eccent) / (1-eccent);
  double M = y / k0;
  double mu = M / (axis * (1 - eccent / 4 - 3 * eccent * eccent / 64 - 5 * eccent * eccent * eccent / 256));
  double phi1Rad = mu + (3 * e1 / 2 - 27 * e1 * e1 * e1 / 32) * sin(2 * mu) + (21 * e1 * e1 / 16 - 55 * e1 * e1 * e1 * e1 / 32) * sin(4 * mu) + (151 * e1 * e1 * e1 / 96) * sin(6 * mu);
  double phi1 = phi1Rad / 3.14159265 * 180.0;
  double N1 = axis / sqrt(1 - eccent * sin(phi1Rad) * sin(phi1Rad));

  double T1 = tan(phi1Rad) * tan(phi1Rad);
  double C1 = eccPrimeSquared * cos(phi1Rad) * cos(phi1Rad);
  double R1 = axis * (1 - eccent) / pow(1-eccent * sin(phi1Rad) * sin(phi1Rad), 1.5);
  double D = x / (N1 * k0);
  double lat = phi1Rad - (N1 * tan(phi1Rad) / R1) * (D * D / 2 - (5 + 3 * T1 + 10 * C1 - 4 * C1 * C1 - 9 * eccPrimeSquared) * D * D * D * D / 24 + (61 + 90 * T1 + 298 * C1 + 45 * T1 * T1 - 252 * eccPrimeSquared - 3 * C1 * C1) * D * D * D * D * D * D / 720);
  lat = lat * RAD2DEG;
  double lon = (D - (1 + 2 * T1 + C1) * D * D * D / 6 + (5 - 2 * C1 + 28 * T1 - 3 * C1 * C1 + 8 * eccPrimeSquared + 24 * T1 * T1) * D * D * D * D * D / 120) / cos(phi1Rad);
  lon = longorig + lon * RAD2DEG;
  
  ll->lat = lon; // kbfu
  ll->lon = lat; // kbfu
}
