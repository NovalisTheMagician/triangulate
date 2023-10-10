#include "triangulate.h"

#include <mapbox/earcut.hpp>

#include <clipper2/clipper.h>

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

using Clipper2Lib::Paths64;
using Clipper2Lib::Path64;
using Clipper2Lib::Point64;
using Clipper2Lib::FillRule;
using Clipper2Lib::MakePath;
using Clipper2Lib::Intersect;
using Clipper2Lib::Difference;
using Clipper2Lib::Union;

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

struct ClipResult clip(const struct Polygon *polygonA, const struct Polygon *polygonB)
{
    std::vector<int64_t> aVertices((size_t)polygonA->length * 2);
    for(unsigned long i = 0; i < polygonA->length; ++i)
    {
        size_t idx = i * 2;
        int64_t x = polygonA->vertices[i][0];
        int64_t y = polygonA->vertices[i][1];
        aVertices[idx+0] = x;
        aVertices[idx+1] = y;
    }
    Paths64 aPath;
    aPath.push_back(MakePath(aVertices));

    std::vector<int64_t> bVertices((size_t)polygonB->length * 2);
    for(unsigned long i = 0; i < polygonB->length; ++i)
    {
        size_t idx = i * 2;
        int64_t x = polygonB->vertices[i][0];
        int64_t y = polygonB->vertices[i][1];
        bVertices[idx+0] = x;
        bVertices[idx+1] = y;
    }
    Paths64 bPath;
    bPath.push_back(MakePath(bVertices));

    Paths64 resPaths = Intersect(aPath, bPath, FillRule::NonZero);
    if(resPaths.size() == 0)
    {
        return (ClipResult){ 0 };
    }

    ClipResult res = { 0 };

    const auto makePolygon = [](struct Polygon *poly, Path64 path)
    {
        poly->length = (unsigned long)path.size();
        for(unsigned long i = 0; i < poly->length; ++i)
        {
            Point64 p = path[i];
            poly->vertices[i][0] = (int)p.x;
            poly->vertices[i][1] = (int)p.y;
        }
        return poly;
    };

    res.numNewPolygons = (unsigned long)resPaths.size();
    res.newPolygons = (struct Polygon**)calloc(resPaths.size(), sizeof *res.newPolygons);
    for(unsigned long i = 0; i < res.numNewPolygons; ++i)
    {
        Path64 path = resPaths[i];
        struct Polygon *poly = (struct Polygon*)calloc(1, sizeof *poly + path.size() * sizeof *poly->vertices);
        res.newPolygons[i] = makePolygon(poly, path);
    }

    Paths64 aClippedPaths = Difference(aPath, resPaths, FillRule::NonZero);
    res.numAClipped = (unsigned long)aClippedPaths.size();
    res.aClipped = (struct Polygon**)calloc(aClippedPaths.size(), sizeof *res.aClipped);
    for(unsigned long i = 0; i < res.numAClipped; ++i)
    {
        Path64 path = aClippedPaths[i];
        struct Polygon *poly = (struct Polygon*)calloc(1, sizeof *poly + path.size() * sizeof *poly->vertices);
        res.aClipped[i] = makePolygon(poly, path);
    }

    Paths64 bClippedPaths = Difference(bPath, resPaths, FillRule::NonZero);
    res.numBClipped = (unsigned long)bClippedPaths.size();
    res.bClipped = (struct Polygon**)calloc(bClippedPaths.size(), sizeof *res.bClipped);
    for(unsigned long i = 0; i < res.numBClipped; ++i)
    {
        Path64 path = bClippedPaths[i];
        struct Polygon *poly = (struct Polygon*)calloc(1, sizeof *poly + path.size() * sizeof *poly->vertices);
        res.bClipped[i] = makePolygon(poly, path);
    }

    return res;
}

unsigned long makeSimple(const struct Polygon *polygon, struct Polygon ***outPolys)
{
    std::vector<int64_t> vertices((size_t)polygon->length * 2);
    for(unsigned long i = 0; i < polygon->length; ++i)
    {
        size_t idx = i * 2;
        int64_t x = polygon->vertices[i][0];
        int64_t y = polygon->vertices[i][1];
        vertices[idx+0] = x;
        vertices[idx+1] = y;
    }
    Paths64 path;
    path.push_back(MakePath(vertices));

    Paths64 simplePaths = Union(path, FillRule::NonZero);

    const auto makePolygon = [](struct Polygon *poly, Path64 path)
    {
        poly->length = (unsigned long)path.size();
        for(unsigned long i = 0; i < poly->length; ++i)
        {
            Point64 p = path[i];
            poly->vertices[i][0] = (int)p.x;
            poly->vertices[i][1] = (int)p.y;
        }
        return poly;
    };

    unsigned long numPolys = (unsigned long)simplePaths.size();
    *outPolys = (struct Polygon**)calloc(numPolys, sizeof **outPolys);
    for(unsigned long i = 0; i < numPolys; ++i)
    {
        Path64 path = simplePaths[i];
        struct Polygon *poly = (struct Polygon*)calloc(1, sizeof *poly + path.size() * sizeof *poly->vertices);
        (*outPolys)[i] = makePolygon(poly, path);
    }

    return numPolys;
}

void freePolygons(struct Polygon **polygons, unsigned long numPolygons)
{
    for(size_t i = 0; i < numPolygons; ++i)
        free(polygons[i]);
}

void freeClipResults(struct ClipResult res)
{
    freePolygons(res.aClipped, res.numAClipped);
    free(res.aClipped);
    freePolygons(res.bClipped, res.numBClipped);
    free(res.bClipped);
    freePolygons(res.newPolygons, res.numNewPolygons);
    free(res.newPolygons);
}

#ifdef __cplusplus
}
#endif
