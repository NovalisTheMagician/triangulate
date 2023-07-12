#pragma once

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int Vertex[2];
typedef short Index;

int triangulate(const Vertex *outerPolygon, size_t numOuterVertices, const Vertex **innerPolygons, size_t *numInnerVertices, size_t numInnerPolygons, Index **indices);

#ifdef __cplusplus
}
#endif
