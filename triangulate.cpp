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

using Vertex_Intern = std::array<float, 2>;

#ifdef __cplusplus
extern "C" {
#endif

unsigned long triangulate(const struct Polygon *outerPolygon, const struct Polygon **innerPolygons, unsigned long numInnerPolygons, Index **indices)
{
    if(!outerPolygon || outerPolygon->length < 3 || !indices || (numInnerPolygons > 0 && !innerPolygons))
        return 0;

    const auto convVert = [](const Vertex &v) -> Vertex_Intern { return { v[0], v[1] }; };

    std::vector<std::vector<Vertex_Intern>> polygons;
    polygons.emplace_back(outerPolygon->length);
    std::transform(outerPolygon->vertices, outerPolygon->vertices + outerPolygon->length, polygons[0].begin(), convVert);
    for (size_t i = 0; i < numInnerPolygons; ++i)
    {
        const struct Polygon *innerPolygon = innerPolygons[i];
        polygons.emplace_back(innerPolygon->length);
        std::transform(innerPolygon->vertices, innerPolygon->vertices+innerPolygon->length, polygons[i+1].begin(), convVert);
    }

    std::vector<Index> ind = mapbox::earcut<Index>(polygons);
    *indices = (Index*)malloc(ind.size() * sizeof **indices);
    std::copy(ind.begin(), ind.end(), *indices);

    return (unsigned long)ind.size();
}

#ifdef __cplusplus
}
#endif
