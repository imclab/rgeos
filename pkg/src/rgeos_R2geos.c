#include "rgeos.h"
#include <string.h>

SEXP rgeos_double_translate(SEXP env, SEXP obj, SEXP id) {
    
    GEOSContextHandle_t GEOShandle = getContextHandle(env);
    
    GEOSGeom geom = rgeos_convert_R2geos( env, obj);
    SEXP p4s = GET_SLOT(obj, install("proj4string"));
    
    SEXP ans = rgeos_convert_geos2R(env, geom, p4s, id); 
    
    return(ans);
}

GEOSGeom rgeos_convert_R2geos(SEXP env, SEXP obj) {
    
    char classbuf[BUFSIZ];
    strcpy(classbuf, CHAR( STRING_ELT(GET_CLASS(obj), 0) ));
    
    //TODO - handle DataFrame classes gracefully
    GEOSGeom ans;
    if ( !strcmp( classbuf,"SpatialPoints") || !strcmp(classbuf,"SpatialPointsDataFrame") ) {
        ans = rgeos_SpatialPoints2geospoint( env, obj);
    } else if ( !strcmp(classbuf,"SpatialLines") || !strcmp(classbuf,"SpatialLinesDataFrame") ) {
        ans = rgeos_SpatialLines2geosline( env, obj);
    } else if ( !strcmp(classbuf,"SpatialRings") || !strcmp(classbuf,"SpatialRingsDataFrame") ) {
        ans = rgeos_SpatialRings2geosring( env, obj);
    } else if ( !strcmp(classbuf,"SpatialPolygons") || !strcmp(classbuf,"SpatialPolygonsDataFrame") ) {
        ans = rgeos_SpatialPolygons2geospolygon( env, obj);
    } else {
        error("rgeos_convert_R2geos: invalid R class, unable to convert");
    }
    
    return(ans);
} 

// Spatial Points to geometry collection (Multipoints)
GEOSGeom rgeos_SpatialPoints2geospoint(SEXP env, SEXP obj) {
    
    GEOSContextHandle_t GEOShandle = getContextHandle(env);

    SEXP crds = GET_SLOT(obj, install("coords")); 
    SEXP dim = getAttrib(crds, install("dim")); 
    int n = INTEGER_POINTER(dim)[0];
    
    GEOSGeom GC;
    if ( n == 1 ){ 
        GC = rgeos_xy2Pt(env, NUMERIC_POINTER(crds)[0], NUMERIC_POINTER(crds)[1]);
    } else if ( n != 1 ) {
        int pc = 0;
        int *unique  = (int *) R_alloc((size_t) n, sizeof(int));
        int *unqcnt  = (int *) R_alloc((size_t) n, sizeof(int));
        int *whichid = (int *) R_alloc((size_t) n, sizeof(int));
        
        SEXP ids;
        PROTECT(ids = VECTOR_ELT( getAttrib(crds, R_DimNamesSymbol), 0 ));pc++;
        
        unique[0] = 0;
        unqcnt[0] = 1;
        whichid[0] = 0;
        
        int i,j,nunq = 1;
        for (i=1; i<n; i++) {
            
            int match = 0;
            for (j=0; j<nunq; j++) {
                match = !strcmp( CHAR(STRING_ELT(ids, i)), CHAR(STRING_ELT(ids, unique[j])) );
                if (match) break;
            }
            
            if (!match) {
                unique[nunq] = i;
                unqcnt[nunq] = 0;
                nunq++;
            }
            unqcnt[j]++;
            whichid[i] = j;
        }
                
        GEOSGeom *geoms = (GEOSGeom *) R_alloc((size_t) nunq, sizeof(GEOSGeom));
        
        
        for (j=0; j<nunq; j++) {

            GEOSGeom *subgeoms = (GEOSGeom *) R_alloc((size_t) unqcnt[j], sizeof(GEOSGeom));
            int k=0;
            for (i=0; i<n; i++) {
                if (whichid[i] == j) {
                    subgeoms[k] = rgeos_xy2Pt(env, NUMERIC_POINTER(crds)[i], NUMERIC_POINTER(crds)[i+n]);
                    k++;
                }
            }
            if (k == 1 ){
                geoms[j] = subgeoms[0];
            } else {
                geoms[j] = GEOSGeom_createCollection_r(GEOShandle, GEOS_MULTIPOINT, subgeoms, unqcnt[j]);
            }
            if (geoms[j] == NULL) error("rgeos_SpatialPoints2geospoint: collection not created");
        }

        if (nunq == 1) {
            GC = geoms[0];
        } else {
            GC = GEOSGeom_createCollection_r(GEOShandle, GEOS_GEOMETRYCOLLECTION, geoms, nunq);
        }
        if (GC == NULL) error("rgeos_SpatialPoints2geospoint: collection not created");
        UNPROTECT(pc);
    } else {
        error("rgeos_SpatialPoints2geospoint: invalid dim");
    }
    
    return(GC);
}

// SpatialLines class to geometry collection
GEOSGeom rgeos_SpatialLines2geosline(SEXP env, SEXP obj) {

    GEOSContextHandle_t GEOShandle = getContextHandle(env);

    int pc = 0;
    SEXP lines;
    PROTECT(lines = GET_SLOT(obj, install("lines"))); pc++;
    int nlines = length(lines);

    GEOSGeom *geoms = (GEOSGeom *) R_alloc((size_t) nlines, sizeof(GEOSGeom));

    for (int i=0; i<nlines; i++) {
        SEXP Lines = VECTOR_ELT(lines, i);
        geoms[i] = rgeos_Lines2geosline(env, Lines);
    }
    
    // If there is only one line collection return multiline not GC
    GEOSGeom GC = geoms[0];
    if (nlines != 1) {
        GC = GEOSGeom_createCollection_r(GEOShandle, GEOS_GEOMETRYCOLLECTION, geoms, nlines);
        if (GC == NULL) error("Lines2GC: collection not created");
    }

    UNPROTECT(pc);
    return(GC);
}

// Lines class to geometry collection (Multilinestring)
GEOSGeom rgeos_Lines2geosline(SEXP env, SEXP obj) {

    GEOSContextHandle_t GEOShandle = getContextHandle(env);
    
    int pc=0;
    SEXP lns;
    PROTECT(lns = GET_SLOT(obj, install("Lines"))); pc++;
    int nlns = length(lns);

    GEOSGeom *geoms = (GEOSGeom *) R_alloc((size_t) nlns, sizeof(GEOSGeom));

    double scale = getScale(env);
    for (int i=0; i<nlns; i++) {
        SEXP crdMat = GET_SLOT(VECTOR_ELT(lns, i), install("coords"));
        
        if (crdMat == R_NilValue) {
            geoms[i] = GEOSGeom_createLineString_r(GEOShandle, NULL);
        } else {
            SEXP dim = getAttrib(crdMat, R_DimSymbol);
            geoms[i] = rgeos_crdMat2LineString(env, crdMat, dim);
        }
    }
    
    GEOSGeom GC = geoms[0];
    if (nlns != 1) {
        GC = GEOSGeom_createCollection_r(GEOShandle, GEOS_MULTILINESTRING, geoms, nlns);
    }
    if (GC == NULL) error("Lines2geosline: collection not created");

    UNPROTECT(pc);
    return(GC);
}


// Spatial polygons to geometry collection (multipolygon)
GEOSGeom rgeos_SpatialPolygons2geospolygon(SEXP env, SEXP obj) {

    GEOSContextHandle_t GEOShandle = getContextHandle(env);

    int pc=0;
    SEXP pls;
    PROTECT(pls = GET_SLOT(obj, install("polygons"))); pc++;
    int npls = length(pls);

    GEOSGeom *geoms = (GEOSGeom *) R_alloc((size_t) npls, sizeof(GEOSGeom));

    for (int i=0; i<npls; i++)
        geoms[i] = rgeos_Polygons2geospolygon(env, VECTOR_ELT(pls, i));
    
    GEOSGeom GC = geoms[0];
    if (npls != 1) {
        GC = GEOSGeom_createCollection_r(GEOShandle, GEOS_GEOMETRYCOLLECTION, geoms, npls);
    } 
    if (GC == NULL) error("rgeos_SpatialPolygons2geospolygon: collection not created");

    UNPROTECT(pc);
    return(GC);
}


GEOSGeom rgeos_Polygons2geospolygon(SEXP env, SEXP obj) {
    
    GEOSContextHandle_t GEOShandle = getContextHandle(env);
    
    int pc=0;
    SEXP pls;
    PROTECT(pls = GET_SLOT(obj, install("Polygons"))); pc++;
    int npls = length(pls);
    
    SEXP comm;
    PROTECT(comm = SP_PREFIX(comment2comm)(obj)); pc++;

    GEOSGeom GC;
    if (comm == R_NilValue) {

        GEOSGeom *geoms = (GEOSGeom *) R_alloc((size_t) npls, sizeof(GEOSGeom));

        for (int i=0; i<npls; i++) {
            SEXP crdMat = GET_SLOT(VECTOR_ELT(pls, i), install("coords"));
            
            if (crdMat == R_NilValue) {
                geoms[i] = GEOSGeom_createPolygon_r(GEOShandle, NULL, NULL, (unsigned int) 0);
            } else {
                geoms[i] = rgeos_crdMat2Polygon(env, crdMat, getAttrib(crdMat, R_DimSymbol));
            }
        }
        
        GC = geoms[0];
        if (npls != 1)
            GC = GEOSGeom_createCollection_r(GEOShandle, GEOS_MULTIPOLYGON, geoms, npls);
        
        if (GC == NULL) error("Polygons2GC: collection not created");
        
    } else {

        int nErings = length(comm);
        GEOSGeom *geoms = (GEOSGeom *) R_alloc((size_t) nErings, sizeof(GEOSGeom));
        
        for (int i=0; i<nErings; i++) {
            geoms[i] = rgeos_Polygons_i_2Polygon(env, pls, VECTOR_ELT(comm, i));
        }
        
        GC = geoms[0];
        if (nErings != 1)
            GC = GEOSGeom_createCollection_r(GEOShandle, GEOS_MULTIPOLYGON, geoms, nErings);
        
        if (GC == NULL) error("Polygons2GC: collection not created");
    }

    UNPROTECT(pc);
    return(GC);
}



GEOSGeom rgeos_Polygons_i_2Polygon(SEXP env, SEXP pls, SEXP vec) {

    GEOSContextHandle_t GEOShandle = getContextHandle(env);

    int n = length(vec);
    int i = INTEGER_POINTER(vec)[0]-R_OFFSET;

    GEOSGeom pol;
    SEXP mat = GET_SLOT(VECTOR_ELT(pls, i), install("coords"));
    if (mat == R_NilValue) {
        if (n != 1) error("Empty polygons should not have holes");
        pol = GEOSGeom_createLinearRing_r(GEOShandle, NULL);
    } else {
        pol = rgeos_crdMat2LinearRing(env, mat, getAttrib(mat, R_DimSymbol));
    }
    
    GEOSGeom res;
    if (n == 1) {
        res = GEOSGeom_createPolygon_r(GEOShandle, pol, NULL, (unsigned int) 0);
    } else {
        GEOSGeom *holes = (GEOSGeom *) R_alloc((size_t) (n-1), sizeof(GEOSGeom));
        for (int j=1; j<n; j++) {
            i = INTEGER_POINTER(vec)[j]-R_OFFSET;
            mat = GET_SLOT(VECTOR_ELT(pls, i), install("coords"));
            
            if (mat == R_NilValue) {
                holes[j-1] = GEOSGeom_createLinearRing_r(GEOShandle, NULL);
            } else {
                holes[j-1] = rgeos_crdMat2LinearRing(env, mat, getAttrib(mat, R_DimSymbol));
            }
        }
        res = GEOSGeom_createPolygon_r(GEOShandle, pol, holes,(unsigned int) (n-1));
    }
    
    if (res == NULL) {
        GEOSGeom_destroy_r(GEOShandle, pol);
        error("rgeos_Polygons_i_2Polygon: Polygon not created");
    }
    
    return(res);
}



// Spatial polygons to fish soup geometry collection (multipoint) 
GEOSGeom rgeos_Polygons2MP(SEXP env, SEXP obj) {
    
    GEOSContextHandle_t GEOShandle = getContextHandle(env);
    
    int pc=0;
    SEXP pls;
    PROTECT(pls = GET_SLOT(obj, install("Polygons"))); pc++;
    int npls = length(pls);
    
    int nn = 0;
    for (int i=0; i<npls; i++) {
        SEXP crdMat = GET_SLOT(VECTOR_ELT(pls, i), install("coords"));
        SEXP dim = getAttrib(crdMat, R_DimSymbol);
        nn += (INTEGER_POINTER(dim)[0]-1);
    }

    GEOSGeom *geoms = (GEOSGeom *) R_alloc((size_t) nn, sizeof(GEOSGeom));

    for (int i=0, ii=0; i<npls; i++) {
        SEXP crdMat = GET_SLOT(VECTOR_ELT(pls, i), install("coords"));
        SEXP dim = getAttrib(crdMat, R_DimSymbol);
        int n = INTEGER_POINTER(dim)[0];
        for (int j=0; j<(n-1); j++) {
            GEOSGeom pt = rgeos_xy2Pt(env, NUMERIC_POINTER(crdMat)[j],NUMERIC_POINTER(crdMat)[j+n]);
            geoms[ii] = pt;
            ii++;
        }
    }

    GEOSGeom GC = GEOSGeom_createCollection_r(GEOShandle, GEOS_MULTIPOINT, geoms, nn);
    if (GC == NULL) {
        error("rgeos_Polygons2MP: collection not created");
    }

    UNPROTECT(pc);
    return(GC);
}



// SpatialRings class to geometry collection
GEOSGeom rgeos_SpatialRings2geosring(SEXP env, SEXP obj) {

    GEOSContextHandle_t GEOShandle = getContextHandle(env);

    int pc = 0;
    SEXP rings;
    PROTECT(rings = GET_SLOT(obj, install("rings"))); pc++;
    int nrings = length(rings);

    GEOSGeom *geoms = (GEOSGeom *) R_alloc((size_t) nrings, sizeof(GEOSGeom));
    for (int i=0; i<nrings; i++) {    
        SEXP crdMat = GET_SLOT(VECTOR_ELT(rings, i), install("coords"));
        
        if (crdMat == R_NilValue) {
            geoms[i] = GEOSGeom_createLinearRing_r(GEOShandle, NULL);
        } else {
            SEXP dim = getAttrib(crdMat, R_DimSymbol);
            geoms[i] = rgeos_crdMat2LinearRing(env, crdMat, dim);
        }
    }
    
    GEOSGeom GC = geoms[0];
    if (nrings != 1) {
        GC = GEOSGeom_createCollection_r(GEOShandle, GEOS_GEOMETRYCOLLECTION, geoms, nrings);
        if (GC == NULL) error("rgeos_SpatialRings2geosring: collection not created");
    }

    UNPROTECT(pc);
    return(GC);
}

