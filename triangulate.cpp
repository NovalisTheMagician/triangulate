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
using Clipper2Lib::Clipper64;
using Clipper2Lib::ClipType;

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

static Path64 PolyToPath(const struct Polygon *polygon)
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
    return MakePath(vertices);
}

static struct Polygon* PathToPoly(const Path64 &path)
{
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

    struct Polygon *poly = (struct Polygon*)calloc(1, sizeof *poly + path.size() * sizeof *poly->vertices);
    return makePolygon(poly, path);
}

struct ClipResult clip(const struct Polygon *polygonA, const struct Polygon *polygonB)
{
    Paths64 aPath;
    aPath.push_back(PolyToPath(polygonA));

    Paths64 bPath;
    bPath.push_back(PolyToPath(polygonB));

    Clipper64 clipper;
    clipper.AddSubject(aPath);
    clipper.AddClip(bPath);

    Paths64 resPaths;
    if(!clipper.Execute(ClipType::Intersection, FillRule::NonZero, resPaths))
    {
        return (ClipResult){ 0 };
    }

    ClipResult res = { 0 };

    res.numNewPolygons = (unsigned long)resPaths.size();
    res.newPolygons = (struct Polygon**)calloc(resPaths.size(), sizeof *res.newPolygons);
    for(unsigned long i = 0; i < res.numNewPolygons; ++i)
    {
        Path64 path = resPaths[i];
        res.newPolygons[i] = PathToPoly(path);
    }

    clipper.Clear();
    clipper.AddSubject(aPath);
    clipper.AddClip(resPaths);

    Paths64 aClippedPaths;
    if(clipper.Execute(ClipType::Difference, FillRule::NonZero, aClippedPaths))
    {
        res.numAClipped = (unsigned long)aClippedPaths.size();
        res.aClipped = (struct Polygon**)calloc(aClippedPaths.size(), sizeof *res.aClipped);
        for(unsigned long i = 0; i < res.numAClipped; ++i)
        {
            Path64 path = aClippedPaths[i];
            res.aClipped[i] = PathToPoly(path);
        }
    }

    clipper.Clear();
    clipper.AddSubject(bPath);
    clipper.AddClip(resPaths);

    Paths64 bClippedPaths;
    if(clipper.Execute(ClipType::Difference, FillRule::NonZero, bClippedPaths))
    {
        res.numBClipped = (unsigned long)bClippedPaths.size();
        res.bClipped = (struct Polygon**)calloc(bClippedPaths.size(), sizeof *res.bClipped);
        for(unsigned long i = 0; i < res.numBClipped; ++i)
        {
            Path64 path = bClippedPaths[i];
            res.bClipped[i] = PathToPoly(path);
        }
    }

    return res;
}

bool clip2(const struct Polygon *editPoly, struct Polygon * const *sectPolys, unsigned long numSectPolys, struct ClipResult2 *res)
{
    assert(res);

    Paths64 subject, clip;
    for(size_t i = 0; i < numSectPolys; ++i)
    {
        subject.push_back(PolyToPath(sectPolys[i]));
    }
    clip.push_back(PolyToPath(editPoly));

    Clipper64 clipper;
    clipper.AddSubject(subject);
    clipper.AddClip(clip);

    Paths64 intersections, differences, clipDifferences;
    clipper.Execute(ClipType::Intersection, FillRule::NonZero, intersections);
    clipper.Execute(ClipType::Xor, FillRule::NonZero, differences);

    /*
    clipper.Clear();
    clipper.AddSubject(clip);
    clipper.AddClip(subject);
    clipper.Execute(ClipType::Difference, FillRule::Positive, clipDifferences);
    */

    res->numPolys = (unsigned long)(intersections.size() + differences.size() + clipDifferences.size());
    res->polygons = (struct Polygon**)calloc(res->numPolys, sizeof *res->polygons);
    unsigned long idx = 0;
    for(const auto &path : intersections)
    {
        res->polygons[idx++] = PathToPoly(path);
    }
    for(const auto &path : differences)
    {
        res->polygons[idx++] = PathToPoly(path);
    }
    for(const auto &path : clipDifferences)
    {
        res->polygons[idx++] = PathToPoly(path);
    }

    return true;
}

bool intersects(const struct Polygon *polyA, const struct Polygon *polyB)
{
    Paths64 subject, clip;
    subject.push_back(PolyToPath(polyA));
    clip.push_back(PolyToPath(polyB));

    Clipper64 clipper;
    clipper.AddSubject(subject);
    clipper.AddClip(clip);
    Paths64 solution;
    clipper.Execute(ClipType::Intersection, FillRule::NonZero, solution);
    return solution.size() > 0;
}

unsigned long makeSimple(const struct Polygon *polygon, struct Polygon ***outPolys)
{
    Paths64 path;
    path.push_back(PolyToPath(polygon));

    Clipper64 clipper;
    clipper.AddSubject(path);
    Paths64 simplePaths;
    clipper.Execute(ClipType::Union, FillRule::NonZero, simplePaths);

    unsigned long numPolys = (unsigned long)simplePaths.size();
    *outPolys = (struct Polygon**)calloc(numPolys, sizeof **outPolys);
    for(unsigned long i = 0; i < numPolys; ++i)
    {
        (*outPolys)[i] = PathToPoly(simplePaths[i]);
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
