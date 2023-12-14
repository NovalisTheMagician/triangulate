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

unsigned long triangulate(const struct Polygon *outerPolygon, const struct Polygon **innerPolygons, unsigned long numInnerPolygons, Index **indices);

#ifdef __cplusplus
}
#endif
