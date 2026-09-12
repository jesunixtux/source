#ifndef PAINT_SPRAYER_H
#define PAINT_SPRAYER_H

#include "baseentity.h"
#include "paint_stream.h"

#ifndef PAINT_SPRAYER_SOUND_DRIP
#define PAINT_SPRAYER_SOUND_DRIP 1
#endif

class CPaintSprayer : public CBaseEntity
{
public:
	DECLARE_CLASS( CPaintSprayer, CBaseEntity );
	void SprayPaint( float flDeltaTime );
	CHandle<CPaintStream> m_hPaintStream;
	int m_PaintPowerType = NO_POWER, m_nAmbientSound = 0, m_nBlobRandomSeed = 0, m_nMaxBlobCount = 128;
	float m_flAccumulatedTime = 0, m_flBlobsPerSecond = 40, m_flBlobSpreadRadius = 0, m_flBlobSpreadAngle = 10;
	float m_flMinSpeed = 950, m_flMaxSpeed = 1050, m_flStreakPercentage = 0;
	float m_flMinStreakTime = 0, m_flMaxStreakTime = 0, m_flMinStreakSpeedDampen = 0, m_flMaxStreakSpeedDampen = 0;
	float m_flNoisyBlobPercentage = 0, m_flPercentageSinceLastNoisyBlob = 0;
	bool m_bSilent = false, m_bDrawOnly = false;
};

#endif
