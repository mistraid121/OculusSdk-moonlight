/************************************************************************************

Filename    :   ModelTrace.h
Content     :   Ray tracer using a KD-Tree.
Created     :   May, 2014
Authors     :   J.M.P. van Waveren

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/
#ifndef __MODELTRACE_H__
#define __MODELTRACE_H__

#include "OVR_Math.h"

#include <vector>

namespace OVR
{

const int RT_KDTREE_MAX_LEAF_TRIANGLES	= 4;

struct kdtree_header_t
{
	int			numVertices;
	int			numUvs;
	int			numIndices;
	int			numNodes;
	int			numLeafs;
	int			numOverflow;
	Bounds3f	bounds;
};

struct kdtree_node_t
{
	// bits [ 0,0] = leaf flag
	// bits [ 2,1] = split plane (0 = x, 1 = y, 2 = z, 3 = invalid)
	// bits [31,3] = index of left child (+1 = right child index), or index of leaf data
	unsigned int	data;
	float			dist;
};

struct kdtree_leaf_t
{
	int			triangles[RT_KDTREE_MAX_LEAF_TRIANGLES];
	int			ropes[6];
	Bounds3f	bounds;
};

struct traceResult_t
{
	int			triangleIndex;
	float		fraction;
	Vector2f	uv;
	Vector3f	normal;
};

class ModelTrace
{
public:
							ModelTrace() {}
							~ModelTrace() {}

	bool					Validate( const bool fullVerify ) const;

	traceResult_t			Trace( const Vector3f & start, const Vector3f & end ) const;
	traceResult_t			Trace_Exhaustive( const Vector3f & start, const Vector3f & end ) const;

	void					PrintStatsToLog() const;

public:
	kdtree_header_t					header;
	std::vector< Vector3f >			vertices;
	std::vector< Vector2f >			uvs;
	std::vector< int >				indices;
	std::vector< kdtree_node_t >	nodes;
	std::vector< kdtree_leaf_t >	leafs;
	std::vector< int >				overflow;	// this is a flat array that stores extra triangle indices for leaves with > RT_KDTREE_MAX_LEAF_TRIANGLES
};

}	// namespace OVR

#endif // !__MODELTRACE_H__
