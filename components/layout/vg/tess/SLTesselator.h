/*
** SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
** Copyright (C) [dates of first publication] Silicon Graphics, Inc.
** All Rights Reserved.
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
** of the Software, and to permit persons to whom the Software is furnished to do so,
** subject to the following conditions:
**
** The above copyright notice including the dates of first publication and either this
** permission notice or a reference to http://oss.sgi.com/projects/FreeB/ shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
** INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
** PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL SILICON GRAPHICS, INC.
** BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
** OR OTHER DEALINGS IN THE SOFTWARE.
**
** Except as contained in this notice, the name of Silicon Graphics, Inc. shall not
** be used in advertising or otherwise to promote the sale, use or other dealings in
** this Software without prior written authorization from Silicon Graphics, Inc.
*/
/*
** Author: Mikko Mononen, July 2009.
*/

// Modified for Stappler Layout Engine

#ifndef LAYOUT_VG_TESS_SLTESSELATOR_H_
#define LAYOUT_VG_TESS_SLTESSELATOR_H_

#ifdef __cplusplus
extern "C" {
#endif

// See OpenGL Red Book for description of the winding rules
// http://www.glprogramming.com/red/chapter11.html
enum TessWindingRule
{
	TESS_WINDING_ODD,
	TESS_WINDING_NONZERO,
	TESS_WINDING_POSITIVE,
	TESS_WINDING_NEGATIVE,
	TESS_WINDING_ABS_GEQ_TWO,
};

enum TessElementType
{
	TESS_POLYGONS,
	TESS_CONNECTED_POLYGONS,
	TESS_BOUNDARY_CONTOURS,
};

typedef float TESSreal;
typedef int TESSindex;
typedef uint16_t TESSshort;
typedef struct TESStesselator TESStesselator;
typedef struct TESSalloc TESSalloc;
typedef struct TESShalfEdge TESShalfEdge;

typedef struct TESSVec2 {
	TESSreal x;
	TESSreal y;
} TESSVec2;

#define TESS_UNDEF (~(TESSshort)0)

#define TESS_NOTUSED(v) do { (void)(1 ? (void)0 : ( (void)(v) ) ); } while(0)

#ifndef __clang__
#define TESS_OPTIMIZE _Pragma( "GCC optimize (\"O2\")" )
//#define TESS_OPTIMIZE
#else
#define TESS_OPTIMIZE
#endif

#define TESSassert(expr, message) if (!(expr)) { tessLog(__FILE__ ": assert " #expr ": " message); assert(expr); }

// Custom memory allocator interface.
// The internal memory allocator allocates mesh edges, vertices and faces
// as well as dictionary nodes and active regions in buckets and uses simple
// freelist to speed up the allocation. The bucket size should roughly match your
// expected input data. For example if you process only hundreds of vertices,
// a bucket size of 128 might be ok, where as when processing thousands of vertices
// bucket size of 1024 might be approproate. The bucket size is a compromise between
// how often to allocate memory from the system versus how much extra space the system
// should allocate. Reasonable defaults are show in commects below, they will be used if
// the bucket sizes are zero.
//
// The use may left the memrealloc to be null. In that case, the tesselator will not try to
// dynamically grow int's internal arrays. The tesselator only needs the reallocation when it
// has found intersecting segments and needs to add new vertex. This defency can be cured by
// allocating some extra vertices beforehand. The 'extraVertices' variable allows to specify
// number of expected extra vertices.
struct TESSalloc
{
	void *(*memalloc)( void *userData, unsigned int size );
	void (*memfree)( void *userData, void *ptr );
	void* userData;				// User data passed to the allocator functions.
};

//
// Example use:
//
//
//
//

// tessNewTess() - Creates a new tesselator.
// Use tessDeleteTess() to delete the tesselator.
// Parameters:
//   alloc - pointer to a filled TESSalloc struct or NULL to use default malloc based allocator.
// Returns:
//   new tesselator object.
TESStesselator* tessNewTess( TESSalloc* alloc );

// tessDeleteTess() - Deletes a tesselator.
// Parameters:
//   tess - pointer to tesselator object to be deleted.
void tessDeleteTess( TESStesselator *tess );

// tessAddContour() - Adds a contour to be tesselated.
// The type of the vertex coordinates is assumed to be TESSreal.
// Parameters:
//   tess - pointer to tesselator object.
//   size - number of coordinates per vertex. Must be 2 or 3.
//   pointer - pointer to the first coordinate of the first vertex in the array.
//   stride - defines offset in bytes between consecutive vertices.
//   count - number of vertices in contour.
TESShalfEdge * tessAddContour( TESStesselator *tess, const TESSVec2* pointer, int count );
TESShalfEdge * tessContinueContour( TESStesselator *tess, TESShalfEdge * e, const TESSVec2* pointer, int count );

typedef struct TESSColor {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} TESSColor;

typedef struct TESSPoint {
	TESSreal x;
	TESSreal y;
	TESSColor c;
} TESSPoint;

typedef struct TESSBuffer {
	TESSPoint *vertexBuffer;
	int vertexCount;

	TESSshort *elementsBuffer;
	int elementCount;
} TESSBuffer;

typedef struct TESSResult {
	TESSBuffer triangles;
	TESSBuffer contour;
} TESSResult;

void tessSetWinding(TESStesselator *tess, int windingRule);
void tessSetColor(TESStesselator *tess, TESSColor color);
void tessSetAntiAliased(TESStesselator *tess, TESSreal val);

TESSResult * tessVecResultTriangles(TESStesselator **tess, int count);
TESSResult * tessResultTriangles(TESStesselator *tess, int windingRule, TESSColor color);

void tessLog(const char *);

#ifdef __cplusplus
};
#endif

#endif // TESSELATOR_H
