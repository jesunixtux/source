#ifndef CPAINTBLOB_H
#define CPAINTBLOB_H

#include "paint_blobs_shared.h"

class CPaintBlob : public CBasePaintBlob
{
public:
	CPaintBlob() : CBasePaintBlob() {}
	virtual ~CPaintBlob() {}
	virtual void PaintBlobPaint( const trace_t & ) {}
};

#endif
