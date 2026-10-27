#ifndef CPAINTBLOB_H
#define CPAINTBLOB_H

#include "paint_blobs_shared.h"

class CPaintBlob : public CBasePaintBlob
{
public:
	CPaintBlob() : CBasePaintBlob() {}
	virtual ~CPaintBlob() {}
	#ifdef PORTAL2
	void PaintBlobPaint( const trace_t &trace ) override;
	#else
	virtual void PaintBlobPaint( const trace_t & ) {}
	#endif
};

#endif
