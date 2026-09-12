// Wrapper that maps Portal 2's "mesh.h" include to the base material system
// mesh interface header.
#ifndef MESH_H
#define MESH_H
#ifdef _WIN32
#pragma once
#endif

#include "materialsystem/imesh.h"

// Portal 2's collision code uses a small CPU-side triangle mesh while it
// clips a brush against a set of planes.  This is deliberately separate from
// the material-system CMeshBuilder/IMesh types.
class CMesh
{
public:
	CMesh() : m_nVertexCount( 0 ), m_nIndexCount( 0 ), m_pIndices( NULL ) {}

	int TriangleCount() const { return m_nIndexCount / 3; }
	const float *GetVertex( int i ) const { return m_Vertices[i].Base(); }
	float *GetVertex( int i ) { return m_Vertices[i].Base(); }

	void Clear()
	{
		m_Vertices.RemoveAll();
		m_Indices.RemoveAll();
		m_nVertexCount = 0;
		m_nIndexCount = 0;
		m_pIndices = NULL;
	}

	CUtlVector<Vector4D> m_Vertices;
	CUtlVector<unsigned short> m_Indices;
	int m_nVertexCount;
	int m_nIndexCount;
	unsigned short *m_pIndices;
};

#endif // MESH_H
