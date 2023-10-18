#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef int Vertex[2];
typedef unsigned int Index;

struct Polygon
{
    void *user;
    unsigned long length;
    Vertex vertices[];
};

struct ClipResult
{
    unsigned long numAClipped;
    struct Polygon **aClipped;

    unsigned long numBClipped;
    struct Polygon **bClipped;

    unsigned long numNewPolygons;
    struct Polygon **newPolygons;
};

struct ClipResult2
{
    unsigned long numPolys;
    struct Polygon **polygons;
};

unsigned long triangulate(const struct Polygon *outerPolygon, const struct Polygon **innerPolygons, unsigned long numInnerPolygons, Index **indices);

struct ClipResult clip(const struct Polygon *polygonA, const struct Polygon *polygonB);
bool clip2(const struct Polygon *editPoly, struct Polygon * const *sectPolys, unsigned long numSectPolys, struct ClipResult2 *res);
bool intersects(const struct Polygon *polyA, const struct Polygon *polyB);

unsigned long makeSimple(const struct Polygon *polygon, struct Polygon ***outPolys);

void freePolygons(struct Polygon **polygons, unsigned long numPolygons);
void freeClipResults(struct ClipResult res);

#ifdef __cplusplus
}
#endif
