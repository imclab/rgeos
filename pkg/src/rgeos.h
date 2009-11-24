#include <R.h>
#include <Rdefines.h>

#include <geos_c.h>

#define R_OFFSET 1

GEOSCoordSeq rgeos_crdMat2CoordSeq(SEXP mat, SEXP dim);

GEOSCoordSeq rgeos_xy2CoordSeq(double x, double y);

GEOSGeom rgeos_Geom2Env(GEOSGeom Geom);


GEOSGeom rgeos_xy2Pt(double x, double y);

GEOSGeom rgeos_crdMat2LineString(SEXP mat, SEXP dim);

GEOSGeom rgeos_crdMat2LinearRing(SEXP mat, SEXP dim);

GEOSGeom rgeos_crdMat2Polygon(SEXP mat, SEXP dim);

GEOSGeom rgeos_SPoints2MP(SEXP obj);

GEOSGeom rgeos_SpatialPolygonsGC(SEXP obj);

SEXP rgeos_GCSpatialPolygons(GEOSGeom Geom);

GEOSGeom rgeos_Polygons_i_2Polygon(SEXP pls, SEXP vec);

SEXP rgeos_PolygonsContain(SEXP obj, SEXP comm);

SEXP rgeos_lineLength(SEXP mat, SEXP dim);

SEXP rgeos_PolArea(SEXP mat, SEXP dim);

SEXP rgeos_PolCentroid(SEXP mat, SEXP dim);

SEXP rgeos_Contains1Pol1pt(SEXP mat, SEXP dim, SEXP x, SEXP y);

SEXP rgeos_Within1Pol1pt(SEXP mat, SEXP dim, SEXP x, SEXP y);

SEXP rgeos_DistNpts1pt(SEXP mat, SEXP dim, SEXP x2, SEXP y2);

SEXP rgeos_Dist1LR1pt(SEXP mat, SEXP dim, SEXP x, SEXP y);


SEXP rgeos_CoordSeq2crdMat(GEOSCoordSeq s, int HasZ);

SEXP rgeos_MP2crdMat(GEOSGeom GC);

SEXP rgeos_Geom2bbox(GEOSGeom Geom);

