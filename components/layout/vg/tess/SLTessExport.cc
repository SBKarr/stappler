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

#include "SLTessInternal.h"

static int prepareTess(TESStesselator *tess, int windingRule, int isContours);
static void OutputVertexCount(TESSmesh *mesh, int *faceCount, int *vertexCount);

int tessExport(TESStesselator *tess, TESSResultInterface *interface) {
	int maxFaceCount = 0;
	int maxVertexCount = 0;
	TESSmesh *mesh = tess->mesh;

	tess->windingRule = interface->windingRule;

	if (prepareTess(tess, tess->windingRule, 0) == 0) {
		return -1;
	}

	OutputVertexCount(tess->mesh, &maxFaceCount, &maxVertexCount);

	interface->setVertexCount(interface->target, maxVertexCount, maxFaceCount);

	// Output vertices.
	for ( TESSvertex * v = mesh->vHead.next; v != &mesh->vHead; v = v->next ) {
		if ( v->n != TESS_UNDEF ) {
			interface->pushVertex(interface->target, v->s, v->t, 1.0f);
		}
	}

	// Output indices.
	for ( TESSface * f = mesh->fHead.next; f != &mesh->fHead; f = f->next ) {
		if ( !f->inside ) continue;

		TESSshort values[3] = { 0 };

		// Store polygon
		TESShalfEdge * edge = f->anEdge;
		int faceVerts = 0;
		do {
			TESSvertex * v = edge->Org;
			values[faceVerts++] = v->n;
			edge = edge->Lnext;
		} while (edge != f->anEdge && faceVerts < 3);

		if (faceVerts == 3) {
			interface->pushTriangle(interface->target, values[0], values[1], values[2]);
		}
	}
	return 0;
}
