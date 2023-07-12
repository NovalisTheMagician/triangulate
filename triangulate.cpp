#include "triangulate.h"

#include <mapbox/earcut.hpp>

#include <vector>
#include <array>
#include <algorithm>

namespace mapbox::util {
    template <>
    struct nth<0, Vertex> {
        inline static auto get(const Vertex &t) {
            return t[0];
        };
    };
    template <>
    struct nth<1, Vertex> {
        inline static auto get(const Vertex &t) {
            return t[1];
        };
    };
}

using Vertex_Intern = std::array<int, 2>;

#ifdef __cplusplus
extern "C" {
#endif

int triangulate(const Vertex *outerPolygon, size_t numOuterVertices, const Vertex **innerPolygons, size_t *numInnerVertices, size_t numInnerPolygons, Index **indices)
{
    if(!outerPolygon || numOuterVertices < 3 || !indices || (numInnerPolygons > 0 && (!numInnerVertices || !innerPolygons)))
        return 0;

    const auto convVert = [](const Vertex &v) -> Vertex_Intern { return { v[0], v[1] }; };

    std::vector<std::vector<Vertex_Intern>> polygons;
    polygons.emplace_back(numOuterVertices);
    std::transform(outerPolygon, outerPolygon + numOuterVertices, polygons[0].begin(), convVert);
    for (size_t i = 0; i < numInnerPolygons; ++i)
    {
        polygons.emplace_back(numInnerVertices[i]);
        std::transform(innerPolygons[i], innerPolygons[i]+numInnerVertices[i], polygons[i+1].begin(), convVert);
    }

    std::vector<Index> ind = mapbox::earcut<Index>(polygons);
    *indices = (Index*)malloc(ind.size() * sizeof **indices);
    std::copy(ind.begin(), ind.end(), *indices);

    return 1;
}

#ifdef __cplusplus
}
#endif
