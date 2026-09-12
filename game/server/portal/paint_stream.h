#ifndef PAINT_STREAM_H
#define PAINT_STREAM_H

#include "baseentity.h"
#include "paint_blobs_shared.h"
#include "paint_sprayer_shared.h"
#include "paint_stream_manager.h"

class IPaintStreamAutoList
{
public:
	static CUtlVector<IPaintStreamAutoList *> &AutoList() { static CUtlVector<IPaintStreamAutoList *> list; return list; }
	IPaintStreamAutoList() { AutoList().AddToTail( this ); }
	virtual ~IPaintStreamAutoList() { AutoList().FindAndFastRemove( this ); }
};

class CPaintBlob;

class CPaintStream : public CBaseEntity, public IPaintStreamAutoList
{
public:
	DECLARE_CLASS( CPaintStream, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CPaintStream() : m_nPaintType( NO_POWER ), m_nRenderMode( BLOB_RENDER_BLOBULATOR ), m_nMaxBlobCount( 128 ), m_vCachedWorldMins( vec3_origin ), m_vCachedWorldMaxs( vec3_origin ) {}
	virtual ~CPaintStream() {}
	virtual void UpdateOnRemove();
	void RemoveAllPaintBlobs();
	unsigned int GetBlobsCount() const;
	CPaintBlob *GetBlob( int id );
	void AddPaintBlob( CPaintBlob *blob ) { if ( blob ) m_blobs.AddToTail( blob ); }
	void RemoveDeadBlobs();
	void PreUpdateBlobs();
	void PostUpdateBlobs();
	void UpdateRenderBoundsAndOriginWorldspace()
	{
		if ( m_blobs.Count() == 0 ) return;
		m_vCachedWorldMins = m_blobs[0]->GetPosition();
		m_vCachedWorldMaxs = m_vCachedWorldMins;
		for ( int i = 1; i < m_blobs.Count(); ++i )
		{
			Vector p = m_blobs[i]->GetPosition();
			VectorMin( m_vCachedWorldMins, p, m_vCachedWorldMins );
			VectorMax( m_vCachedWorldMaxs, p, m_vCachedWorldMaxs );
		}
	}
	const Vector &WorldAlignMins() const;
	const Vector &WorldAlignMaxs() const;
	void QueuePaintEffect();
	void RemoveTeleportedThisFrameBlobs();
	void ResetBlobsTeleportedThisFrame();
	void DebugDrawBlobs() {}
	void AddPaintToDatabase() {}
	void UpdateBlobSharedData() {}

	int m_nPaintType;
	int m_nRenderMode;
	int m_nMaxBlobCount;
	PaintBlobVector_t m_blobs;
	TimeStampVector m_UsedChannelTimestamps;
	Vector m_vCachedWorldMins;
	Vector m_vCachedWorldMaxs;
};

#endif
