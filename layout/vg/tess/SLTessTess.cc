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
** Author: Eric Veach, July 1994.
*/

#include <stddef.h>
#include <assert.h>
#include <setjmp.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <float.h>

#include "SLTessGeom.h"
#include "SLTessMesh.h"
#include "SLTessSweep.h"
#include "SLTessTess.h"

//TESS_OPTIMIZE

#define TRUE 1
#define FALSE 0

#define MATH_TOLERANCE FLT_EPSILON
#define ANTIALIAS_TOLERANCE 0.5f

#define Dot(u,v)	(u[0]*v[0] + u[1]*v[1] + u[2]*v[2])

#if defined(FOR_TRITE_TEST_PROGRAM) || defined(TRUE_PROJECT)
static void Normalize( TESSreal v[3] )
{
	TESSreal len = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	assert( len > 0 );
	len = sqrtf( len );
	v[0] /= len;
	v[1] /= len;
	v[2] /= len;
}
#endif

#define ABS(x)	((x) < 0 ? -(x) : (x))

#ifdef FOR_TRITE_TEST_PROGRAM
#include <stdlib.h>
extern int RandomSweep;
#define S_UNIT_X	(RandomSweep ? (2*drand48()-1) : 1.0)
#define S_UNIT_Y	(RandomSweep ? (2*drand48()-1) : 0.0)
#else
#if defined(SLANTED_SWEEP)
/* The "feature merging" is not intended to be complete.  There are
* special cases where edges are nearly parallel to the sweep line
* which are not implemented.  The algorithm should still behave
* robustly (ie. produce a reasonable tesselation) in the presence
* of such edges, however it may miss features which could have been
* merged.  We could minimize this effect by choosing the sweep line
* direction to be something unusual (ie. not parallel to one of the
* coordinate axes).
*/
#define S_UNIT_X	(TESSreal)0.50941539564955385	/* Pre-normalized */
#define S_UNIT_Y	(TESSreal)0.86052074622010633
#else
#define S_UNIT_X	(TESSreal)1.0
#define S_UNIT_Y	(TESSreal)0.0
#endif
#endif

/* Determine the polygon normal and project vertices onto the plane
* of the polygon.
*/
void tessProjectPolygon( TESStesselator *tess )
{
	TESSvertex *v, *vHead = &tess->mesh->vHead;
	int first;

	/* Compute ST bounds. */
	first = 1;
	for( v = vHead->next; v != vHead; v = v->next )
	{
		if (first)
		{
			tess->bmin[0] = tess->bmax[0] = v->s;
			tess->bmin[1] = tess->bmax[1] = v->t;
			first = 0;
		}
		else
		{
			if (v->s < tess->bmin[0]) tess->bmin[0] = v->s;
			if (v->s > tess->bmax[0]) tess->bmax[0] = v->s;
			if (v->t < tess->bmin[1]) tess->bmin[1] = v->t;
			if (v->t > tess->bmax[1]) tess->bmax[1] = v->t;
		}
	}
}

#define AddWinding(eDst,eSrc)	(eDst->winding += eSrc->winding, \
	eDst->Sym->winding += eSrc->Sym->winding)

/* tessMeshTessellateMonoRegion( face ) tessellates a monotone region
* (what else would it do??)  The region must consist of a single
* loop of half-edges (see mesh.h) oriented CCW.  "Monotone" in this
* case means that any vertical line intersects the interior of the
* region in a single interval.
*
* Tessellation consists of adding interior edges (actually pairs of
* half-edges), to split the region into non-overlapping triangles.
*
* The basic idea is explained in Preparata and Shamos (which I don''t
* have handy right now), although their implementation is more
* complicated than this one.  The are two edge chains, an upper chain
* and a lower chain.  We process all vertices from both chains in order,
* from right to left.
*
* The algorithm ensures that the following invariant holds after each
* vertex is processed: the untessellated region consists of two
* chains, where one chain (say the upper) is a single edge, and
* the other chain is concave.  The left vertex of the single edge
* is always to the left of all vertices in the concave chain.
*
* Each step consists of adding the rightmost unprocessed vertex to one
* of the two chains, and forming a fan of triangles from the rightmost
* of two chain endpoints.  Determining whether we can add each triangle
* to the fan is a simple orientation test.  By making the fan as large
* as possible, we restore the invariant (check it yourself).
*/
int tessMeshTessellateMonoRegion( TESSmesh *mesh, TESSface *face )
{
	TESShalfEdge *up, *lo;

	/* All edges are oriented CCW around the boundary of the region.
	* First, find the half-edge whose origin vertex is rightmost.
	* Since the sweep goes from left to right, face->anEdge should
	* be close to the edge we want.
	*/
	up = face->anEdge;
	assert( up->Lnext != up && up->Lnext->Lnext != up );

	for( ; VertLeq( up->Dst, up->Org ); up = up->Lprev )
		;
	for( ; VertLeq( up->Org, up->Dst ); up = up->Lnext )
		;
	lo = up->Lprev;

	while( up->Lnext != lo ) {
		if( VertLeq( up->Dst, lo->Org )) {
			/* up->Dst is on the left.  It is safe to form triangles from lo->Org.
			* The EdgeGoesLeft test guarantees progress even when some triangles
			* are CW, given that the upper and lower chains are truly monotone.
			*/
			while( lo->Lnext != up && (EdgeGoesLeft( lo->Lnext )
				|| EdgeSign( lo->Org, lo->Dst, lo->Lnext->Dst ) <= 0 )) {
					TESShalfEdge *tempHalfEdge= tessMeshConnect( mesh, lo->Lnext, lo );
					if (tempHalfEdge == NULL) return 0;
					lo = tempHalfEdge->Sym;
			}
			lo = lo->Lprev;
		} else {
			/* lo->Org is on the left.  We can make CCW triangles from up->Dst. */
			while( lo->Lnext != up && (EdgeGoesRight( up->Lprev )
				|| EdgeSign( up->Dst, up->Org, up->Lprev->Org ) >= 0 )) {
					TESShalfEdge *tempHalfEdge= tessMeshConnect( mesh, up, up->Lprev );
					if (tempHalfEdge == NULL) return 0;
					up = tempHalfEdge->Sym;
			}
			up = up->Lnext;
		}
	}

	/* Now lo->Org == up->Dst == the leftmost vertex.  The remaining region
	* can be tessellated in a fan from this leftmost vertex.
	*/
	assert( lo->Lnext != up );
	while( lo->Lnext->Lnext != up ) {
		TESShalfEdge *tempHalfEdge= tessMeshConnect( mesh, lo->Lnext, lo );
		if (tempHalfEdge == NULL) return 0;
		lo = tempHalfEdge->Sym;
	}

	return 1;
}


/* tessMeshTessellateInterior( mesh ) tessellates each region of
* the mesh which is marked "inside" the polygon.  Each such region
* must be monotone.
*/
int tessMeshTessellateInterior( TESSmesh *mesh )
{
	TESSface *f, *next;

	/*LINTED*/
	for( f = mesh->fHead.next; f != &mesh->fHead; f = next ) {
		/* Make sure we don''t try to tessellate the new triangles. */
		next = f->next;
		if( f->inside ) {
			if ( !tessMeshTessellateMonoRegion( mesh, f ) ) return 0;
		}
	}

	return 1;
}


/* tessMeshDiscardExterior( mesh ) zaps (ie. sets to NULL) all faces
* which are not marked "inside" the polygon.  Since further mesh operations
* on NULL faces are not allowed, the main purpose is to clean up the
* mesh so that exterior loops are not represented in the data structure.
*/
void tessMeshDiscardExterior( TESSmesh *mesh )
{
	TESSface *f, *next;

	/*LINTED*/
	for( f = mesh->fHead.next; f != &mesh->fHead; f = next ) {
		/* Since f will be destroyed, save its next pointer. */
		next = f->next;
		if( ! f->inside ) {
			tessMeshZapFace( mesh, f );
		}
	}
}

/* tessMeshSetWindingNumber( mesh, value, keepOnlyBoundary ) resets the
* winding numbers on all edges so that regions marked "inside" the
* polygon have a winding number of "value", and regions outside
* have a winding number of 0.
*
* If keepOnlyBoundary is TRUE, it also deletes all edges which do not
* separate an interior region from an exterior one.
*/
int tessMeshSetWindingNumber( TESSmesh *mesh, int value,
							 int keepOnlyBoundary )
{
	TESShalfEdge *e, *eNext;

	for( e = mesh->eHead.next; e != &mesh->eHead; e = eNext ) {
		eNext = e->next;
		if( e->Rface->inside != e->Lface->inside ) {

			/* This is a boundary edge (one side is interior, one is exterior). */
			e->winding = (e->Lface->inside) ? value : -value;
		} else {

			/* Both regions are interior, or both are exterior. */
			if( ! keepOnlyBoundary ) {
				e->winding = 0;
			} else {
				if ( !tessMeshDelete( mesh, e ) ) return 0;
			}
		}
	}
	return 1;
}

void* heapAlloc( void* userData, unsigned int size )
{
	TESS_NOTUSED( userData );
	return malloc( size );
}

void heapFree( void* userData, void* ptr )
{
	TESS_NOTUSED( userData );
	free( ptr );
}

static TESSalloc defaulAlloc =
{
	heapAlloc,
	heapFree,
	0,
};

TESStesselator* tessNewTess( TESSalloc* alloc )
{
	TESStesselator* tess;

	if (alloc == NULL)
		alloc = &defaulAlloc;

	/* Only initialize fields which can be changed by the api.  Other fields
	* are initialized where they are used.
	*/

	tess = (TESStesselator *)alloc->memalloc( alloc->userData, sizeof( TESStesselator ));
	if ( tess == NULL ) {
		return 0;          /* out of memory */
	}
	tess->alloc = *alloc;

	tess->bmin[0] = 0;
	tess->bmin[1] = 0;
	tess->bmax[0] = 0;
	tess->bmax[1] = 0;

	tess->windingRule = TESS_WINDING_ODD;

	// Initialize to begin polygon.
	tess->mesh = NULL;

	tess->outOfMemory = 0;
	tess->vertexIndexCounter = 0;
	tess->antiAliasValue = 0.0f;
	tess->antiAlias = 0;
	tess->plane = 0;
	tess->color.r = 0;
	tess->color.g = 0;
	tess->color.b = 0;
	tess->color.a = 0;
	tess->numVertices = 0;

	return tess;
}

void tessDeleteTess( TESStesselator *tess )
{
	struct TESSalloc alloc = tess->alloc;

	if( tess->mesh != NULL ) {
		tessMeshDeleteMesh( &alloc, tess->mesh );
		tess->mesh = NULL;
	}

	alloc.memfree( alloc.userData, tess );
}

TESShalfEdge * tessAddContour( TESStesselator *tess, const TESSreal* vertices, int numVertices ) {
	TESShalfEdge *e = NULL;
	return tessContinueContour(tess, e, vertices, numVertices);
}

TESShalfEdge * tessContinueContour( TESStesselator *tess, TESShalfEdge * e, const TESSreal* vertices, int numVertices ) {
	const unsigned char *src = (const unsigned char*)vertices;
	int i;

	if ( tess->mesh == NULL )
	  	tess->mesh = tessMeshNewMesh( &tess->alloc );
 	if ( tess->mesh == NULL ) {
		tess->outOfMemory = 1;
		return NULL;
	}

 	tess->numVertices += numVertices;

	for( i = 0; i < numVertices; ++i ) {
		const TESSreal* coords = (const TESSreal*)src;
		src += sizeof(TESSreal) * 2;

		if( e == NULL ) {
			/* Make a self-loop (one vertex, one edge). */
			e = tessMeshMakeEdge( tess->mesh );
			if ( e == NULL ) {
				tess->outOfMemory = 1;
				return NULL;
			}
			if ( !tessMeshSplice( tess->mesh, e, e->Sym ) ) {
				tess->outOfMemory = 1;
				return NULL;
			}
		} else {
			/* Create a new vertex and edge which immediately follow e
			* in the ordering around the left face.
			*/
			if ( tessMeshSplitEdge( tess->mesh, e ) == NULL ) {
				tess->outOfMemory = 1;
				return NULL;
			}
			e = e->Lnext;
		}

		/* The new vertex is now e->Org. */
		e->Org->s = coords[0];
		e->Org->t = coords[1];
		/* Store the insertion number so that the vertex can be later recognized. */
		e->Org->idx = tess->vertexIndexCounter++;

		/* The winding of an edge says how the winding number changes as we
		* cross from the edge''s right face to its left face.  We add the
		* vertices in such an order that a CCW contour will add +1 to
		* the winding number of the region inside the contour.
		*/
		e->winding = 1;
		e->Sym->winding = -1;
	}

	return e;
}

static int prepareTess(TESStesselator *tess, int windingRule, int isContours) {
	TESSmesh *mesh;
	int rc = 1;

	tess->vertexIndexCounter = 0;
	tess->windingRule = windingRule;

	if (setjmp(tess->env) != 0) {
		/* come back here if out of memory */
		return 0;
	}

	if (!tess->mesh)
	{
		return 0;
	}

	/* Determine the polygon normal and project vertices onto the plane
	* of the polygon.
	*/
	tessProjectPolygon( tess );

	/* tessComputeInterior( tess ) computes the planar arrangement specified
	* by the given contours, and further subdivides this arrangement
	* into regions.  Each region is marked "inside" if it belongs
	* to the polygon, according to the rule given by tess->windingRule.
	* Each interior region is guaranteed be monotone.
	*/
	if ( !tessComputeInterior( tess ) ) {
		longjmp(tess->env,1);  /* could've used a label */
	}

	mesh = tess->mesh;

	/* If the user wants only the boundary contours, we throw away all edges
	* except those which separate the interior from the exterior.
	* Otherwise we tessellate all the regions marked "inside".
	*/
	if (isContours) {
		rc = tessMeshSetWindingNumber( mesh, 1, TRUE );
	} else {
		rc = tessMeshTessellateInterior( mesh );
	}
	if (rc == 0) longjmp(tess->env,1);  /* could've used a label */

	tessMeshCheckMesh( mesh );
	return 1;
}

static inline float Angle(float v1_x, float v1_y, float v2_x, float v2_y) {
	return atan2f(v1_x * v2_y - v1_y * v2_x, v1_x * v2_x + v1_y * v2_y);
}

static int OutputLineSegments(TESStesselator *tess, TESSPoint *buf, TESShalfEdge *v0, TESShalfEdge *v1, TESShalfEdge *v2, TESSreal width) {
	const float cx = v1->Org->s;
	const float cy = v1->Org->t;

	const float x0 = v0->Org->s - cx;
	const float y0 = v0->Org->t - cy;
	const float x1 = v2->Org->s - cx;
	const float y1 = v2->Org->t - cy;

	const float a = Angle(x0, y0, x1, y1); // -PI; PI

	const float n0 = sqrtf(x0 * x0 + y0 * y0);
	if (n0 < FLT_EPSILON) {
		v1->degenerated = 1;
		return 0;
	}

	const float nx = x0 / n0;
	const float ny = y0 / n0;

	const float sinAngle = sinf(a / 2.0f);
	const float cosAngle = cosf(a / 2.0f);

	const float tx = nx * cosAngle - ny * sinAngle;
	const float ty = ny * cosAngle + nx * sinAngle;

	const float offset = width / sinAngle;

	if (fabs(offset) > width * 4.0f) {
		uint8_t opacity = roundf(((width * 4.0f) / offset) * 255.0f);
		buf->x = cx + tx * width * 4; buf->y = cy + ty * width * 4; buf->c = tess->color; buf->c.a = roundf(buf->c.a * (255 - opacity) / 255.0f);
		++ buf;
		buf->x = cx - tx * width * 4; buf->y = cy - ty * width * 4; buf->c = tess->color; buf->c.a = roundf(buf->c.a * opacity / 255.0f);
		return 2;
	} else {
		buf->x = cx + tx * offset; buf->y = cy + ty * offset; buf->c = tess->color; buf->c.a = 0;
		++ buf;
		buf->x = cx - tx * offset; buf->y = cy - ty * offset; buf->c = tess->color;
		return 2;
	}
}

static void OutputVertexCount( TESSmesh *mesh, int *faceCount, int *vertexCount ) {
	int maxFaceCount = 0;
	int maxVertexCount = 0;

	// Mark unused
	for ( TESSvertex * v = mesh->vHead.next; v != &mesh->vHead; v = v->next ) {
		v->n = TESS_UNDEF;
	}

	// Create unique IDs for all vertices and faces.
	for ( TESSface * f = mesh->fHead.next; f != &mesh->fHead; f = f->next ) {
		f->n = TESS_UNDEF;
		if( !f->inside ) continue;

		TESShalfEdge * edge = f->anEdge;
		do {
			TESSvertex * v = edge->Org;
			if ( v->n == TESS_UNDEF ) {
				v->n = *vertexCount + maxVertexCount;
				maxVertexCount++;
			}
			edge = edge->Lnext;
		} while (edge != f->anEdge);

		f->n = maxFaceCount;
		++maxFaceCount;
	}

	*faceCount += maxFaceCount;
	*vertexCount += maxVertexCount;
}

static void OutputRawTrianglesVec( TESSResult *res, TESStesselator **tess, int count ) {
	int maxFaceCount = 0;
	int maxVertexCount = 0;

	TESSBuffer *buf = &res->triangles;
	TESStesselator **tessIt = tess;
	for (int i = 0; i < count; ++ i) {
		TESSmesh *mesh = (*tessIt)->mesh;
		OutputVertexCount(mesh, &maxFaceCount, &maxVertexCount);
		++ tessIt;
	}

	buf->elementCount = maxFaceCount;
	buf->elementsBuffer = (TESSshort*)(*tess)->alloc.memalloc( (*tess)->alloc.userData, sizeof(TESSshort) * maxFaceCount * 3 );
	if (!buf->elementsBuffer) {
		(*tess)->outOfMemory = 1;
		return;
	}

	buf->vertexCount = maxVertexCount;
	buf->vertexBuffer = (TESSPoint*)(*tess)->alloc.memalloc( (*tess)->alloc.userData, sizeof(TESSPoint) * maxVertexCount );
	if (!buf->vertexBuffer) {
		(*tess)->outOfMemory = 1;
		return;
	}

	TESSPoint *vertexes = buf->vertexBuffer;
	TESSshort *elements = buf->elementsBuffer;

	tessIt = tess;
	for (int i = 0; i < count; ++ i) {
		TESSmesh *mesh = (*tessIt)->mesh;
		// Output vertices.
		for ( TESSvertex * v = mesh->vHead.next; v != &mesh->vHead; v = v->next ) {
			if ( v->n != TESS_UNDEF ) {
				// Store coordinate
				TESSPoint * vert = &vertexes[v->n];
				vert->x = v->s;
				vert->y = v->t;
				vert->c = (*tessIt)->color;
			}
		}

		// Output indices.
		for ( TESSface * f = mesh->fHead.next; f != &mesh->fHead; f = f->next ) {
			if ( !f->inside ) continue;

			// Store polygon
			TESShalfEdge * edge = f->anEdge;
			int faceVerts = 0;
			do {
				TESSvertex * v = edge->Org;
				*elements++ = v->n;
				faceVerts++;
				edge = edge->Lnext;
			} while (edge != f->anEdge);
			// Fill unused.
			for (int i = faceVerts; i < 3; ++i) {
				*elements++ = TESS_UNDEF;
			}
		}
		++ tessIt;
	}
}

static void OutputRawTrianglesAntiAliasVec( TESSResult *res, TESStesselator **tess, int count ) {
	OutputRawTrianglesVec(res, tess, count);

	TESSBuffer *buf = &res->contour;
	TESShalfEdge *edge = 0;
	TESShalfEdge *start = 0;
	int countAccum = 0;

	buf->vertexCount = 0;
	buf->elementCount = 0;

	TESStesselator **tessIt = tess;
	for (int i = 0; i < count; ++ i) {
		if ((*tessIt)->antiAlias) {
			TESSmesh *mesh = (*tessIt)->mesh;
			tessMeshSetWindingNumber( mesh, 1, TRUE );

			for (TESSface * f = mesh->fHead.next; f != &mesh->fHead; f = f->next ) {
				if ( !f->inside ) continue;

				start = edge = f->anEdge;
				do {
					++buf->vertexCount;
					edge = edge->Lnext;
				} while ( edge != start );

				++buf->elementCount;
			}
		}

		++ tessIt;
	}

	buf->elementsBuffer = (TESSshort*)(*tess)->alloc.memalloc( (*tess)->alloc.userData, sizeof(TESSshort) * buf->elementCount * 2 );
	if (!buf->elementsBuffer) {
		(*tess)->outOfMemory = 1;
		return;
	}

	buf->vertexBuffer = (TESSPoint*)(*tess)->alloc.memalloc( (*tess)->alloc.userData, sizeof(TESSPoint) * (buf->vertexCount * 2 + 2 * buf->elementCount) );
	if (!buf->vertexBuffer) {
		(*tess)->outOfMemory = 1;
		return;
	}

	TESSPoint *verts = buf->vertexBuffer;
	TESSshort *elements = buf->elementsBuffer;

	TESSshort startVert = 0;
	tessIt = tess;
	for (int i = 0; i < count; ++ i) {
		if ((*tessIt)->antiAlias) {
			TESSmesh *mesh = (*tessIt)->mesh;
			for ( TESSface * f = mesh->fHead.next; f != &mesh->fHead; f = f->next ) {
				if ( !f->inside ) continue;

				TESSPoint * origVerts = verts;
				TESSshort vertCount = 0;
				start = edge = f->anEdge;
				do {
					int tmpCount = OutputLineSegments((*tessIt), verts, edge->Lprev, edge, edge->Lnext, ANTIALIAS_TOLERANCE / (*tessIt)->antiAliasValue);
					verts += tmpCount;
					vertCount += tmpCount;
					edge = edge->Lnext;
				} while ( edge != start );

				verts->x = origVerts->x; verts->y = origVerts->y; verts->c = origVerts->c;
				++ verts; ++ origVerts;
				verts->x = origVerts->x; verts->y = origVerts->y; verts->c = origVerts->c;
				++ verts;

				vertCount += 2;

				elements[0] = startVert;
				elements[1] = vertCount;
				elements += 2;

				startVert += vertCount;
				countAccum += vertCount;
			}
		}
		++ tessIt;
	}

	buf->vertexCount = countAccum;
}

TESSResult * tessVecResultTriangles(TESStesselator **tess, int count) {
	TESStesselator **tessIt = tess;
	for (int i = 0; i < count; ++ i) {
		if (prepareTess(*tessIt, (*tessIt)->windingRule, 0) == 0) {
			return NULL;
		}
		++ tessIt;
	}

	TESSResult *res = (TESSResult *)(*tess)->alloc.memalloc( (*tess)->alloc.userData, sizeof( TESSResult ));

	OutputRawTrianglesAntiAliasVec( res, tess, count );     /* output contours */

	if ((*tess)->outOfMemory) {
		return NULL;
	}

	return res;
}

TESSResult * tessResultTriangles(TESStesselator *tess, int windingRule, TESSColor color) {
	tessSetWinding(tess, windingRule);
	tessSetColor(tess, color);
	return tessVecResultTriangles(&tess, 1);
}

void tessSetWinding(TESStesselator *tess, int windingRule) {
	tess->windingRule = windingRule;
}
void tessSetColor(TESStesselator *tess, TESSColor color) {
	tess->color = color;
}
void tessSetAntiAliased(TESStesselator *tess, TESSreal val) {
	if (val != 0.0f) {
		tess->antiAlias = 1;
		tess->antiAliasValue = val;
	} else {
		tess->antiAlias = 0;
	}
}
