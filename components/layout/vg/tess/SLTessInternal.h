/**
 Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 **/

/* ========== ORIGINAL LICENSE ========== */

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

#ifndef COMPONENTS_LAYOUT_VG_TESS_SLTESSINTERNAL_H_
#define COMPONENTS_LAYOUT_VG_TESS_SLTESSINTERNAL_H_

#ifdef __cplusplus
#error "SLTessInternal should not be included in c++ context"
#endif

#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <stdint.h>
#include <setjmp.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "SLTesselator.h"

typedef void *DictKey;
typedef struct TESSalloc TESSalloc;
typedef struct Dict Dict;
typedef struct DictNode DictNode;

Dict *dictNewDict( TESSalloc* alloc, void *frame, int (*leq)(void *frame, DictKey key1, DictKey key2) );

void dictDeleteDict( TESSalloc* alloc, Dict *dict );

/* Search returns the node with the smallest key greater than or equal
* to the given key.  If there is no such key, returns a node whose
* key is NULL.  Similarly, Succ(Max(d)) has a NULL key, etc.
*/
DictNode *dictSearch( Dict *dict, DictKey key );
DictNode *dictInsertBefore( Dict *dict, DictNode *node, DictKey key );
void dictDelete( Dict *dict, DictNode *node );

#define dictKey(n)	((n)->key)
#define dictSucc(n)	((n)->next)
#define dictPred(n)	((n)->prev)
#define dictMin(d)	((d)->head.next)
#define dictMax(d)	((d)->head.prev)
#define dictInsert(d,k) (dictInsertBefore((d),&(d)->head,(k)))


/*** Private data structures ***/

struct DictNode {
	DictKey	key;
	DictNode *next;
	DictNode *prev;
};

struct Dict {
	DictNode head;
	void *frame;
	TESSalloc *alloc;
	int (*leq)(void *frame, DictKey key1, DictKey key2);
};


/* The basic operations are insertion of a new key (pqInsert),
* and examination/extraction of a key whose value is minimum
* (pqMinimum/pqExtractMin).  Deletion is also allowed (pqDelete);
* for this purpose pqInsert returns a "handle" which is supplied
* as the argument.
*
* An initial heap may be created efficiently by calling pqInsert
* repeatedly, then calling pqInit.  In any case pqInit must be called
* before any operations other than pqInsert are used.
*
* If the heap is empty, pqMinimum/pqExtractMin will return a NULL key.
* This may also be tested with pqIsEmpty.
*/

/* Since we support deletion the data structure is a little more
* complicated than an ordinary heap.  "nodes" is the heap itself;
* active nodes are stored in the range 1..pq->size.  When the
* heap exceeds its allocated size (pq->max), its size doubles.
* The children of node i are nodes 2i and 2i+1.
*
* Each node stores an index into an array "handles".  Each handle
* stores a key, plus a pointer back to the node which currently
* represents that key (ie. nodes[handles[i].node].handle == i).
*/

typedef void *PQkey;
typedef int PQhandle;
typedef struct PriorityQHeap PriorityQHeap;

#define INV_HANDLE 0x0fffffff

typedef struct { PQhandle handle; } PQnode;
typedef struct { PQkey key; PQhandle node; } PQhandleElem;

struct PriorityQHeap {
	PQnode *nodes;
	PQhandleElem *handles;
	int size, max;
	PQhandle freeList;
	int initialized;

	int (*leq)(PQkey key1, PQkey key2);
};

typedef struct PriorityQ PriorityQ;

struct PriorityQ {
	PriorityQHeap *heap;

	PQkey *keys;
	PQkey **order;
	PQhandle size, max;
	int initialized;

	int (*leq)(PQkey key1, PQkey key2);
};

PriorityQ *pqNewPriorityQ( TESSalloc* alloc, int size, int (*leq)(PQkey key1, PQkey key2) );
void pqDeletePriorityQ( TESSalloc* alloc, PriorityQ *pq );

int pqInit( TESSalloc* alloc, PriorityQ *pq );
PQhandle pqInsert( TESSalloc* alloc, PriorityQ *pq, PQkey key );
PQkey pqExtractMin( PriorityQ *pq );
void pqDelete( PriorityQ *pq, PQhandle handle );

PQkey pqMinimum( PriorityQ *pq );
int pqIsEmpty( PriorityQ *pq );

typedef struct TESSmesh TESSmesh;
typedef struct TESSvertex TESSvertex;
typedef struct TESSface TESSface;
typedef struct TESShalfEdge TESShalfEdge;
typedef struct ActiveRegion ActiveRegion;


/* The mesh structure is similar in spirit, notation, and operations
* to the "quad-edge" structure (see L. Guibas and J. Stolfi, Primitives
* for the manipulation of general subdivisions and the computation of
* Voronoi diagrams, ACM Transactions on Graphics, 4(2):74-123, April 1985).
* For a simplified description, see the course notes for CS348a,
* "Mathematical Foundations of Computer Graphics", available at the
* Stanford bookstore (and taught during the fall quarter).
* The implementation also borrows a tiny subset of the graph-based approach
* use in Mantyla's Geometric Work Bench (see M. Mantyla, An Introduction
* to Sold Modeling, Computer Science Press, Rockville, Maryland, 1988).
*
* The fundamental data structure is the "half-edge".  Two half-edges
* go together to make an edge, but they point in opposite directions.
* Each half-edge has a pointer to its mate (the "symmetric" half-edge Sym),
* its origin vertex (Org), the face on its left side (Lface), and the
* adjacent half-edges in the CCW direction around the origin vertex
* (Onext) and around the left face (Lnext).  There is also a "next"
* pointer for the global edge list (see below).
*
* The notation used for mesh navigation:
*  Sym   = the mate of a half-edge (same edge, but opposite direction)
*  Onext = edge CCW around origin vertex (keep same origin)
*  Dnext = edge CCW around destination vertex (keep same dest)
*  Lnext = edge CCW around left face (dest becomes new origin)
*  Rnext = edge CCW around right face (origin becomes new dest)
*
* "prev" means to substitute CW for CCW in the definitions above.
*
* The mesh keeps global lists of all vertices, faces, and edges,
* stored as doubly-linked circular lists with a dummy header node.
* The mesh stores pointers to these dummy headers (vHead, fHead, eHead).
*
* The circular edge list is special; since half-edges always occur
* in pairs (e and e->Sym), each half-edge stores a pointer in only
* one direction.  Starting at eHead and following the e->next pointers
* will visit each *edge* once (ie. e or e->Sym, but not both).
* e->Sym stores a pointer in the opposite direction, thus it is
* always true that e->Sym->next->Sym->next == e.
*
* Each vertex has a pointer to next and previous vertices in the
* circular list, and a pointer to a half-edge with this vertex as
* the origin (NULL if this is the dummy header).  There is also a
* field "data" for client data.
*
* Each face has a pointer to the next and previous faces in the
* circular list, and a pointer to a half-edge with this face as
* the left face (NULL if this is the dummy header).  There is also
* a field "data" for client data.
*
* Note that what we call a "face" is really a loop; faces may consist
* of more than one loop (ie. not simply connected), but there is no
* record of this in the data structure.  The mesh may consist of
* several disconnected regions, so it may not be possible to visit
* the entire mesh by starting at a half-edge and traversing the edge
* structure.
*
* The mesh does NOT support isolated vertices; a vertex is deleted along
* with its last edge.  Similarly when two faces are merged, one of the
* faces is deleted (see tessMeshDelete below).  For mesh operations,
* all face (loop) and vertex pointers must not be NULL.  However, once
* mesh manipulation is finished, TESSmeshZapFace can be used to delete
* faces of the mesh, one at a time.  All external faces can be "zapped"
* before the mesh is returned to the client; then a NULL face indicates
* a region which is not part of the output polygon.
*/

struct TESSvertex {
	TESSvertex *next;      /* next vertex (never NULL) */
	TESSvertex *prev;      /* previous vertex (never NULL) */
	TESShalfEdge *anEdge;    /* a half-edge with this origin */

	/* Internal data (keep hidden) */
	//TESSreal coords[3];  /* vertex location in 3D */
	TESSreal s, t;       /* projection onto the sweep plane */
	int pqHandle;   /* to allow deletion from priority queue */
	TESSindex n;			/* to allow identify unique vertices */
	TESSindex idx;			/* to allow map result to original verts */
};

struct TESSface {
	TESSface *next;      /* next face (never NULL) */
	TESSface *prev;      /* previous face (never NULL) */
	TESShalfEdge *anEdge;    /* a half edge with this left face */

	/* Internal data (keep hidden) */
	TESSface *trail;     /* "stack" for conversion to strips */
	TESSindex n;		/* to allow identiy unique faces */
	char marked;     /* flag for conversion to strips */
	char inside;     /* this face is in the polygon interior */
};

struct TESShalfEdge {
	TESShalfEdge *next;      /* doubly-linked list (prev==Sym->next) */
	TESShalfEdge *Sym;       /* same edge, opposite direction */
	TESShalfEdge *Onext;     /* next edge CCW around origin */
	TESShalfEdge *Lnext;     /* next edge CCW around left face */
	TESSvertex *Org;       /* origin vertex (Overtex too long) */
	TESSface *Lface;     /* left face */

	/* Internal data (keep hidden) */
	ActiveRegion *activeRegion;  /* a region with this upper edge (sweep.c) */
	int winding;    /* change in winding number when crossing
						  from the right face to the left face */
	int degenerated;
};

#define Rface   Sym->Lface
#define Dst Sym->Org

#define Oprev   Sym->Lnext
#define Lprev   Onext->Sym
#define Dprev   Lnext->Sym
#define Rprev   Sym->Onext
#define Dnext   Rprev->Sym  /* 3 pointers */
#define Rnext   Oprev->Sym  /* 3 pointers */


struct TESSmesh {
	TESSvertex vHead;      /* dummy header for vertex list */
	TESSface fHead;      /* dummy header for face list */
	TESShalfEdge eHead;      /* dummy header for edge list */
	TESShalfEdge eHeadSym;   /* and its symmetric counterpart */

	struct TESSalloc* alloc;
};

/* The mesh operations below have three motivations: completeness,
* convenience, and efficiency.  The basic mesh operations are MakeEdge,
* Splice, and Delete.  All the other edge operations can be implemented
* in terms of these.  The other operations are provided for convenience
* and/or efficiency.
*
* When a face is split or a vertex is added, they are inserted into the
* global list *before* the existing vertex or face (ie. e->Org or e->Lface).
* This makes it easier to process all vertices or faces in the global lists
* without worrying about processing the same data twice.  As a convenience,
* when a face is split, the "inside" flag is copied from the old face.
* Other internal data (v->data, v->activeRegion, f->data, f->marked,
* f->trail, e->winding) is set to zero.
*
* ********************** Basic Edge Operations **************************
*
* tessMeshMakeEdge( mesh ) creates one edge, two vertices, and a loop.
* The loop (face) consists of the two new half-edges.
*
* tessMeshSplice( eOrg, eDst ) is the basic operation for changing the
* mesh connectivity and topology.  It changes the mesh so that
*  eOrg->Onext <- OLD( eDst->Onext )
*  eDst->Onext <- OLD( eOrg->Onext )
* where OLD(...) means the value before the meshSplice operation.
*
* This can have two effects on the vertex structure:
*  - if eOrg->Org != eDst->Org, the two vertices are merged together
*  - if eOrg->Org == eDst->Org, the origin is split into two vertices
* In both cases, eDst->Org is changed and eOrg->Org is untouched.
*
* Similarly (and independently) for the face structure,
*  - if eOrg->Lface == eDst->Lface, one loop is split into two
*  - if eOrg->Lface != eDst->Lface, two distinct loops are joined into one
* In both cases, eDst->Lface is changed and eOrg->Lface is unaffected.
*
* tessMeshDelete( eDel ) removes the edge eDel.  There are several cases:
* if (eDel->Lface != eDel->Rface), we join two loops into one; the loop
* eDel->Lface is deleted.  Otherwise, we are splitting one loop into two;
* the newly created loop will contain eDel->Dst.  If the deletion of eDel
* would create isolated vertices, those are deleted as well.
*
* ********************** Other Edge Operations **************************
*
* tessMeshAddEdgeVertex( eOrg ) creates a new edge eNew such that
* eNew == eOrg->Lnext, and eNew->Dst is a newly created vertex.
* eOrg and eNew will have the same left face.
*
* tessMeshSplitEdge( eOrg ) splits eOrg into two edges eOrg and eNew,
* such that eNew == eOrg->Lnext.  The new vertex is eOrg->Dst == eNew->Org.
* eOrg and eNew will have the same left face.
*
* tessMeshConnect( eOrg, eDst ) creates a new edge from eOrg->Dst
* to eDst->Org, and returns the corresponding half-edge eNew.
* If eOrg->Lface == eDst->Lface, this splits one loop into two,
* and the newly created loop is eNew->Lface.  Otherwise, two disjoint
* loops are merged into one, and the loop eDst->Lface is destroyed.
*
* ************************ Other Operations *****************************
*
* tessMeshNewMesh() creates a new mesh with no edges, no vertices,
* and no loops (what we usually call a "face").
*
* tessMeshUnion( mesh1, mesh2 ) forms the union of all structures in
* both meshes, and returns the new mesh (the old meshes are destroyed).
*
* tessMeshDeleteMesh( mesh ) will free all storage for any valid mesh.
*
* tessMeshZapFace( fZap ) destroys a face and removes it from the
* global face list.  All edges of fZap will have a NULL pointer as their
* left face.  Any edges which also have a NULL pointer as their right face
* are deleted entirely (along with any isolated vertices this produces).
* An entire mesh can be deleted by zapping its faces, one at a time,
* in any order.  Zapped faces cannot be used in further mesh operations!
*
* tessMeshCheckMesh( mesh ) checks a mesh for self-consistency.
*/

TESShalfEdge *tessMeshMakeEdge( TESSmesh *mesh );
int tessMeshSplice( TESSmesh *mesh, TESShalfEdge *eOrg, TESShalfEdge *eDst );
int tessMeshDelete( TESSmesh *mesh, TESShalfEdge *eDel );

TESShalfEdge *tessMeshAddEdgeVertex( TESSmesh *mesh, TESShalfEdge *eOrg );
TESShalfEdge *tessMeshSplitEdge( TESSmesh *mesh, TESShalfEdge *eOrg );
TESShalfEdge *tessMeshConnect( TESSmesh *mesh, TESShalfEdge *eOrg, TESShalfEdge *eDst );

TESSmesh *tessMeshNewMesh( TESSalloc* alloc );
TESSmesh *tessMeshUnion( TESSalloc* alloc, TESSmesh *mesh1, TESSmesh *mesh2 );
int tessMeshMergeConvexFaces( TESSmesh *mesh, int maxVertsPerFace );
void tessMeshDeleteMesh( TESSalloc* alloc, TESSmesh *mesh );
void tessMeshZapFace( TESSmesh *mesh, TESSface *fZap );

#ifdef NDEBUG
#define tessMeshCheckMesh( mesh )
#else
void tessMeshCheckMesh( TESSmesh *mesh );
#endif


struct TESStesselator {

	/*** state needed for collecting the input data ***/
	TESSmesh	*mesh;		/* stores the input contours, and eventually
						the tessellation itself */
	int outOfMemory;

	/*** state needed for projecting onto the sweep plane ***/

	TESSreal bmin[2];
	TESSreal bmax[2];

	/*** state needed for the line sweep ***/
	int	windingRule;	/* rule for determining polygon interior */

	Dict *dict;		/* edge dictionary for sweep line */
	PriorityQ *pq;		/* priority queue of vertex events */
	TESSvertex *event;		/* current sweep event being processed */

	TESSindex vertexIndexCounter;
	TESSalloc alloc;
	TESSColor color;
	TESSreal antiAliasValue;
	int plane;
	int numVertices;

	jmp_buf env;			/* place to jump to when memAllocs fail */
};


#ifdef NO_BRANCH_CONDITIONS
/* MIPS architecture has special instructions to evaluate boolean
* conditions -- more efficient than branching, IF you can get the
* compiler to generate the right instructions (SGI compiler doesn't)
*/
#define VertEq(u,v)	(((u)->s == (v)->s) & ((u)->t == (v)->t))
#define VertLeq(u,v)	(((u)->s < (v)->s) | \
	((u)->s == (v)->s & (u)->t <= (v)->t))
#else
#define VertEq(u,v) ((u)->s == (v)->s && (u)->t == (v)->t)
#define VertLeq(u,v) (((u)->s < (v)->s) || ((u)->s == (v)->s && (u)->t <= (v)->t))
#endif

#define EdgeEval(u,v,w)	tesedgeEval(u,v,w)
#define EdgeSign(u,v,w)	tesedgeSign(u,v,w)

/* Versions of VertLeq, EdgeSign, EdgeEval with s and t transposed. */

#define TransLeq(u,v) (((u)->t < (v)->t) || ((u)->t == (v)->t && (u)->s <= (v)->s))
#define TransEval(u,v,w) testransEval(u,v,w)
#define TransSign(u,v,w) testransSign(u,v,w)


#define EdgeGoesLeft(e) VertLeq( (e)->Dst, (e)->Org )
#define EdgeGoesRight(e) VertLeq( (e)->Org, (e)->Dst )

#define ABS(x) ((x) < 0 ? -(x) : (x))
#define VertL1dist(u,v) (ABS(u->s - v->s) + ABS(u->t - v->t))

#define VertCCW(u,v,w) tesvertCCW(u,v,w)

int tesvertLeq( TESSvertex *u, TESSvertex *v );
TESSreal	tesedgeEval( TESSvertex *u, TESSvertex *v, TESSvertex *w );
TESSreal	tesedgeSign( TESSvertex *u, TESSvertex *v, TESSvertex *w );
TESSreal	testransEval( TESSvertex *u, TESSvertex *v, TESSvertex *w );
TESSreal	testransSign( TESSvertex *u, TESSvertex *v, TESSvertex *w );
int tesvertCCW( TESSvertex *u, TESSvertex *v, TESSvertex *w );
void tesedgeIntersect( TESSvertex *o1, TESSvertex *d1, TESSvertex *o2, TESSvertex *d2, TESSvertex *v );

/* tessComputeInterior( tess ) computes the planar arrangement specified
* by the given contours, and further subdivides this arrangement
* into regions.  Each region is marked "inside" if it belongs
* to the polygon, according to the rule given by tess->windingRule.
* Each interior region is guaranteed be monotone.
*/
int tessComputeInterior( TESStesselator *tess );

/* For each pair of adjacent edges crossing the sweep line, there is
* an ActiveRegion to represent the region between them.  The active
* regions are kept in sorted order in a dynamic dictionary.  As the
* sweep line crosses each vertex, we update the affected regions.
*/

struct ActiveRegion {
	TESShalfEdge *eUp;		/* upper edge, directed right to left */
	DictNode *nodeUp;	/* dictionary node corresponding to eUp */
	int windingNumber;	/* used to determine which regions are
							* inside the polygon */
	int inside;		/* is this region inside the polygon? */
	int sentinel;	/* marks fake edges at t = +/-infinity */
	int dirty;		/* marks regions where the upper or lower
					* edge has changed, but we haven't checked
					* whether they intersect yet */
	int fixUpperEdge;	/* marks temporary edges introduced when
						* we process a "right vertex" (one without
						* any edges leaving to the right) */
};

#define RegionBelow(r) ((ActiveRegion *) dictKey(dictPred((r)->nodeUp)))
#define RegionAbove(r) ((ActiveRegion *) dictKey(dictSucc((r)->nodeUp)))

#define TRUE 1
#define FALSE 0

#define MATH_TOLERANCE FLT_EPSILON
#define ANTIALIAS_TOLERANCE 0.5f

#endif /* COMPONENTS_LAYOUT_VG_TESS_SLTESSINTERNAL_H_ */
