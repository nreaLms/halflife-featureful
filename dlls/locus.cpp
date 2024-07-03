//=========================================
// NEW file for Spirit of Half-Life 0.7
// Created 14/01/02
//=========================================

// Spirit of Half-Life's particle system uses "locus triggers" to tell
// entities where to perform their actions.

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "locus.h"
#include "effects.h"
#include "decals.h"

bool IsLikelyNumber(const char* szText)
{
	return (*szText >= '0' && *szText <= '9') || *szText == '-';
}

bool TryCalcLocus_Position( CBaseEntity *pEntity, CBaseEntity *pLocus, const char *szText, Vector& result )
{
	if (IsLikelyNumber(szText))
	{ // it's a vector
		Vector tmp;
		UTIL_StringToRandomVector( (float *)tmp, szText );
		result = tmp;
		return true;
	}

	CBaseEntity *pCalc = UTIL_FindEntityByTargetname(NULL, szText, pLocus);
	if (pCalc != NULL)
	{
		return pCalc->CalcPosition( pLocus, &result );
	}

	const char* requesterClassname = pEntity ? STRING(pEntity->pev->classname) : "";
	const char* requesterTargetname = pEntity ? STRING(pEntity->pev->targetname) : "";
	ALERT(at_error, "%s \"%s\" has bad or missing calc_position value \"%s\"\n", requesterClassname, requesterTargetname, szText);
	return false;
}

bool TryCalcLocus_Velocity(CBaseEntity *pEntity, CBaseEntity *pLocus, const char *szText , Vector& result)
{
	if (IsLikelyNumber(szText))
	{ // it's a vector
		Vector tmp;
		UTIL_StringToRandomVector( (float *)tmp, szText );
		result = tmp;
		return true;
	}

	CBaseEntity *pCalc = UTIL_FindEntityByTargetname(NULL, szText, pLocus);
	if (pCalc != NULL)
	{
		return pCalc->CalcVelocity( pLocus, &result );
	}

	const char* requesterClassname = pEntity ? STRING(pEntity->pev->classname) : "";
	const char* requesterTargetname = pEntity ? STRING(pEntity->pev->targetname) : "";
	ALERT(at_error, "%s \"%s\" has bad or missing calc_velocity value \"%s\"\n", requesterClassname, requesterTargetname, szText);
	return false;
}

bool TryCalcLocus_Ratio(CBaseEntity *pLocus, const char *szText , float& result)
{
	if (IsLikelyNumber(szText))
	{ // assume it's a float
		result = atof( szText );
		return true;
	}

	CBaseEntity *pCalc = UTIL_FindEntityByTargetname(NULL, szText, pLocus);

	if (pCalc != NULL)
	{
		return pCalc->CalcRatio( pLocus, &result );
	}

	ALERT(at_error, "Bad or missing calc_ratio entity \"%s\"\n", szText);
	return false;
}

bool TryCalcLocus_Color(CBaseEntity *pEntity, CBaseEntity *pLocus, const char *szText , Vector& result)
{
	if (IsLikelyNumber(szText))
	{ // it's a vector
		Vector tmp;
		UTIL_StringToRandomVector( (float *)tmp, szText );
		result = tmp;
		return true;
	}

	CBaseEntity *pCalc = UTIL_FindEntityByTargetname(NULL, szText, pLocus);
	if (pCalc != NULL)
	{
		result = pCalc->pev->rendercolor;
		return true;
	}

	const char* requesterClassname = pEntity ? STRING(pEntity->pev->classname) : "";
	const char* requesterTargetname = pEntity ? STRING(pEntity->pev->targetname) : "";
	ALERT(at_error, "%s \"%s\" has bad or missing color value \"%s\"\n", requesterClassname, requesterTargetname, szText);
	return false;
}

//=============================================
//locus_x effects
//=============================================
#if 0
// Entity variable
class CLocusAlias : public CBaseAlias
{
public:
	void	PostSpawn( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	CBaseEntity		*FollowAlias( CBaseEntity *pFrom );
	void	FlushChanges( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	EHANDLE	m_hValue;
	EHANDLE m_hChangeTo;
};

TYPEDESCRIPTION	CLocusAlias::m_SaveData[] = 
{
	DEFINE_FIELD( CLocusAlias, m_hValue, FIELD_EHANDLE),
	DEFINE_FIELD( CLocusAlias, m_hChangeTo, FIELD_EHANDLE),
};

LINK_ENTITY_TO_CLASS( locus_alias, CLocusAlias );
IMPLEMENT_SAVERESTORE( CLocusAlias, CBaseAlias );

void CLocusAlias::PostSpawn( void )
{
	m_hValue = UTIL_FindEntityByTargetname( NULL, STRING(pev->netname) );
}

void CLocusAlias::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_hChangeTo = pActivator;
	UTIL_AddToAliasList( this );
}

void CLocusAlias::FlushChanges( void )
{
	m_hValue = m_hChangeTo;
	m_hChangeTo = NULL;
}

CBaseEntity *CLocusAlias::FollowAlias( CBaseEntity *pFrom )
{
	if (m_hValue == 0)
		return NULL;
	else if ( pFrom == NULL || (OFFSET(m_hValue->pev) > OFFSET(pFrom->pev)) )
	{
//		ALERT(at_console, "LocusAlias returns %s:  %f %f %f\n", STRING(m_pValue->pev->targetname), m_pValue->pev->origin.x, m_pValue->pev->origin.y, m_pValue->pev->origin.z);
		return m_hValue;
	}
	else
		return NULL;
}
#endif



// Beam maker
#define BEAM_FSINE		0x10
#define BEAM_FSOLID		0x20
#define BEAM_FSHADEIN	0x40
#define BEAM_FSHADEOUT	0x80

#define SF_LBEAM_SHADEIN	128
#define SF_LBEAM_SHADEOUT	256
#define SF_LBEAM_SOLID		512
#define SF_LBEAM_SINE		1024

class CLocusBeam : public CPointEntity
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void KeyValue( KeyValueData *pkvd );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	string_t m_iszSprite;
	string_t m_iszTargetName;
	string_t m_iszStart;
	string_t m_iszEnd;
	int		m_iWidth;
	int		m_iDistortion;
	float	m_fFrame;
	int		m_iScrollRate;
	float	m_fDuration;
	float	m_fDamage;
	int		m_iDamageType;
	int		m_iFlags;
	byte	m_iStartAttachment;
	byte	m_iEndAttachment;
};

TYPEDESCRIPTION	CLocusBeam::m_SaveData[] = 
{
	DEFINE_FIELD( CLocusBeam, m_iszSprite, FIELD_STRING),
	DEFINE_FIELD( CLocusBeam, m_iszTargetName, FIELD_STRING),
	DEFINE_FIELD( CLocusBeam, m_iszStart, FIELD_STRING),
	DEFINE_FIELD( CLocusBeam, m_iszEnd, FIELD_STRING),
	DEFINE_FIELD( CLocusBeam, m_iWidth, FIELD_INTEGER),
	DEFINE_FIELD( CLocusBeam, m_iDistortion, FIELD_INTEGER),
	DEFINE_FIELD( CLocusBeam, m_fFrame, FIELD_FLOAT),
	DEFINE_FIELD( CLocusBeam, m_iScrollRate, FIELD_INTEGER),
	DEFINE_FIELD( CLocusBeam, m_fDuration, FIELD_FLOAT),
	DEFINE_FIELD( CLocusBeam, m_fDamage, FIELD_FLOAT),
	DEFINE_FIELD( CLocusBeam, m_iDamageType, FIELD_INTEGER),
	DEFINE_FIELD( CLocusBeam, m_iFlags, FIELD_INTEGER),
	DEFINE_FIELD( CLocusBeam, m_iStartAttachment, FIELD_CHARACTER),
	DEFINE_FIELD( CLocusBeam, m_iEndAttachment, FIELD_CHARACTER),
};

LINK_ENTITY_TO_CLASS( locus_beam, CLocusBeam )
IMPLEMENT_SAVERESTORE(CLocusBeam, CPointEntity)

void CLocusBeam::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iszSprite"))
	{
		m_iszSprite = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszTargetName"))
	{
		m_iszTargetName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszStart"))
	{
		m_iszStart = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszEnd"))
	{
		m_iszEnd = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iWidth"))
	{
		m_iWidth = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iDistortion"))
	{
		m_iDistortion = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fFrame"))
	{
		m_fFrame = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iScrollRate"))
	{
		m_iScrollRate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fDuration"))
	{
		m_fDuration = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fDamage"))
	{
		m_fDamage = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iDamageType"))
	{
		m_iDamageType = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iStartAttachment"))
	{
		m_iStartAttachment = atoi(pkvd->szValue);
		if (m_iStartAttachment > 4 || m_iStartAttachment < 0)
			m_iStartAttachment = 0;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iEndAttachment"))
	{
		m_iEndAttachment = atoi(pkvd->szValue);
		if (m_iEndAttachment > 4 || m_iEndAttachment < 0)
			m_iEndAttachment = 0;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CLocusBeam::Precache ( void )
{
	PRECACHE_MODEL ( STRING(m_iszSprite) );
}

void CLocusBeam::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pStartEnt;
	CBaseEntity *pEndEnt;
	Vector vecStartPos;
	Vector vecEndPos;
	CBeam *pBeam;

	switch(pev->impulse)
	{
	case 0: // ents
		pStartEnt = UTIL_FindEntityByTargetname(NULL, STRING(m_iszStart), pActivator);
		pEndEnt = UTIL_FindEntityByTargetname(NULL, STRING(m_iszEnd), pActivator);

		if (pStartEnt == NULL || pEndEnt == NULL)
			return;
		pBeam = CBeam::BeamCreate( STRING(m_iszSprite), m_iWidth );
		pBeam->EntsInit( pStartEnt->entindex(), pEndEnt->entindex(), m_iStartAttachment, m_iEndAttachment );
		break;

	case 1: // pointent
		if (!TryCalcLocus_Position( this, pActivator, STRING(m_iszStart), vecStartPos ))
			return;

		pEndEnt = UTIL_FindEntityByTargetname(NULL, STRING(m_iszEnd), pActivator);

		if (pEndEnt == NULL)
			return;
		pBeam = CBeam::BeamCreate( STRING(m_iszSprite), m_iWidth );
		pBeam->PointEntInit( vecStartPos, pEndEnt->entindex(), m_iEndAttachment );
		break;
	case 2: // points
		if (!TryCalcLocus_Position( this, pActivator, STRING(m_iszStart), vecStartPos ) || !TryCalcLocus_Position( this, pActivator, STRING(m_iszEnd), vecEndPos )) {
			return;
		}

		pBeam = CBeam::BeamCreate( STRING(m_iszSprite), m_iWidth );
		pBeam->PointsInit( vecStartPos, vecEndPos );
		break;
	case 3: // point & offset
		if (!TryCalcLocus_Position( this, pActivator, STRING(m_iszStart), vecStartPos ) || !TryCalcLocus_Velocity( this, pActivator, STRING(m_iszEnd), vecEndPos ))
			return;

		pBeam = CBeam::BeamCreate( STRING(m_iszSprite), m_iWidth );
		pBeam->PointsInit( vecStartPos, vecStartPos + vecEndPos );
		break;
	}
	pBeam->SetColor( pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z );
	pBeam->SetBrightness( pev->renderamt );
	pBeam->SetNoise( m_iDistortion );
	pBeam->SetFrame( m_fFrame );
	pBeam->SetScrollRate( m_iScrollRate );
	pBeam->SetFlags( m_iFlags );
	pBeam->pev->dmg = m_fDamage;
	pBeam->pev->frags = m_iDamageType;
	pBeam->pev->spawnflags |= pev->spawnflags & (SF_BEAM_RING |
			SF_BEAM_SPARKSTART | SF_BEAM_SPARKEND | SF_BEAM_DECALS);
	if (m_fDuration)
	{
		pBeam->SetThink(&CBeam:: SUB_Remove );
		pBeam->pev->nextthink = gpGlobals->time + m_fDuration;
	}
	pBeam->pev->targetname = m_iszTargetName;

	if (pev->target)
	{
		FireTargets( STRING(pev->target), pBeam, this );
	}
}

void CLocusBeam::Spawn( void )
{
	Precache();
	m_iFlags = 0;
	if (pev->spawnflags & SF_LBEAM_SHADEIN)
		m_iFlags |= BEAM_FSHADEIN;
	if (pev->spawnflags & SF_LBEAM_SHADEOUT)
		m_iFlags |= BEAM_FSHADEOUT;
	if (pev->spawnflags & SF_LBEAM_SINE)
		m_iFlags |= BEAM_FSINE;
	if (pev->spawnflags & SF_LBEAM_SOLID)
		m_iFlags |= BEAM_FSOLID;
}

//=============================================
//calc_x entities
//=============================================

class CCalcPosition : public CPointEntity
{
public:
	bool CalcPosition( CBaseEntity *pLocus, Vector* outVector );
};

LINK_ENTITY_TO_CLASS( calc_position, CCalcPosition )

bool CCalcPosition::CalcPosition( CBaseEntity *pLocus, Vector* outVector )
{
	CBaseEntity *pSubject = UTIL_FindEntityByTargetname(NULL, STRING(pev->netname), pLocus);
	if ( !pSubject )
	{
		ALERT(at_console, "%s \"%s\" failed to find target entity \"%s\"\n", STRING(pev->classname), STRING(pev->targetname), STRING(pev->netname) );
		return false;
	}

	Vector vecOffset = g_vecZero;

	if (pev->message) {
		if (!TryCalcLocus_Velocity( this, pLocus, STRING(pev->message), vecOffset)) {
			ALERT(at_console, "%s \"%s\" failed, bad LV \"%s\"\n", STRING(pev->classname), STRING(pev->targetname), STRING(pev->message) );
			return false;
		}
	}

	Vector vecPosition;
	Vector vecJunk;

	switch (pev->impulse)
	{
	case 1: //eyes
		*outVector = vecOffset + pSubject->EyePosition();
		//ALERT(at_console, "calc_subpos returns %f %f %f\n", vecResult.x, vecResult.y, vecResult.z);
		break;
		//return vecOffset + pLocus->EyePosition();
	case 2: // top
		*outVector = vecOffset + pSubject->pev->origin + Vector(
			(pSubject->pev->mins.x + pSubject->pev->maxs.x)/2,
			(pSubject->pev->mins.y + pSubject->pev->maxs.y)/2,
			pSubject->pev->maxs.z
		);
		break;
	case 3: // centre
		*outVector = vecOffset + pSubject->pev->origin + Vector(
			(pSubject->pev->mins.x + pSubject->pev->maxs.x)/2,
			(pSubject->pev->mins.y + pSubject->pev->maxs.y)/2,
			(pSubject->pev->mins.z + pSubject->pev->maxs.z)/2
		);
		break;
	case 4: // bottom
		*outVector = vecOffset + pSubject->pev->origin + Vector(
			(pSubject->pev->mins.x + pSubject->pev->maxs.x)/2,
			(pSubject->pev->mins.y + pSubject->pev->maxs.y)/2,
			pSubject->pev->mins.z
		);
		break;
	case 5:
		// this could cause problems.
		// is there a good way to check whether it's really a CBaseAnimating?
		((CBaseAnimating*)pSubject)->GetAttachment( 0, vecPosition, vecJunk );
		*outVector = vecOffset + vecPosition;
		break;
	case 6:
		((CBaseAnimating*)pSubject)->GetAttachment( 1, vecPosition, vecJunk );
		*outVector = vecOffset + vecPosition;
		break;
	case 7:
		((CBaseAnimating*)pSubject)->GetAttachment( 2, vecPosition, vecJunk );
		*outVector = vecOffset + vecPosition;
		break;
	case 8:
		((CBaseAnimating*)pSubject)->GetAttachment( 3, vecPosition, vecJunk );
		*outVector = vecOffset + vecPosition;
		break;
	case 9:
		*outVector = vecOffset + pSubject->pev->origin + Vector(
			RANDOM_FLOAT(pSubject->pev->mins.x, pSubject->pev->maxs.x),
			RANDOM_FLOAT(pSubject->pev->mins.y, pSubject->pev->maxs.y),
			RANDOM_FLOAT(pSubject->pev->mins.z, pSubject->pev->maxs.z)
		);
		break;
	default:
		*outVector = vecOffset + pSubject->pev->origin;
		break;
	}
	return true;
}

//=======================================================

enum
{
	CALCRATIO_TRANSFORM_NONE = 0,
	CALCRATIO_TRANSFORM_REVERSED,
	CALCRATIO_TRANSFORM_NEGATIVE,
	CALCRATIO_TRANSFORM_RECIPROCAL,
	CALCRATIO_TRANSFORM_SQUARE,
	CALCRATIO_TRANSFORM_INVERSESQUARE,
	CALCRATIO_TRANSFORM_SQUAREROOT,
	CALCRATIO_TRANSFORM_COSINE,
	CALCRATIO_TRANSFORM_SINE,
	CALCRATIO_TRANSFORM_TANGENT,
	CALCRATIO_TRANSFORM_ACOSINE,
	CALCRATIO_TRANSFORM_ASINE,
	CALCRATIO_TRANSFORM_ATANGENT,
};

class CCalcRatio : public CPointEntity
{
public:
	bool CalcRatio( CBaseEntity *pLocus, float* outResult );

	void Spawn();
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszMin;
	string_t m_iszMax;
};

LINK_ENTITY_TO_CLASS( calc_ratio, CCalcRatio )

TYPEDESCRIPTION	CCalcRatio::m_SaveData[] =
{
	DEFINE_FIELD( CCalcRatio, m_iszMin, FIELD_STRING ),
	DEFINE_FIELD( CCalcRatio, m_iszMax, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CCalcRatio, CPointEntity )

void CCalcRatio::Spawn()
{
	m_iszMin = pev->noise;
	pev->noise = iStringNull;
	m_iszMax = pev->noise1;
	pev->noise1 = iStringNull;
}

bool CCalcRatio::CalcRatio( CBaseEntity *pLocus, float* outResult )
{
	float fBasis = 0;
	if (!TryCalcLocus_Ratio( pLocus, STRING(pev->target), fBasis)) {
		return false;
	}

	switch (pev->impulse)
	{
	case CALCRATIO_TRANSFORM_REVERSED:	fBasis = 1-fBasis; break;
	case CALCRATIO_TRANSFORM_NEGATIVE:	fBasis = -fBasis; break;
	case CALCRATIO_TRANSFORM_RECIPROCAL: fBasis = 1/fBasis; break;
	case CALCRATIO_TRANSFORM_SQUARE: fBasis = fBasis * fBasis; break;
	case CALCRATIO_TRANSFORM_INVERSESQUARE: fBasis = 1 / (fBasis * fBasis); break;
	case CALCRATIO_TRANSFORM_SQUAREROOT: fBasis = sqrt(fBasis); break;
	case CALCRATIO_TRANSFORM_COSINE: fBasis = cos(fBasis * (M_PI / 180.0f)); break;
	case CALCRATIO_TRANSFORM_SINE: fBasis = sin(fBasis * (M_PI / 180.0f)); break;
	case CALCRATIO_TRANSFORM_TANGENT: fBasis = tan(fBasis * (M_PI / 180.0f)); break;
	case CALCRATIO_TRANSFORM_ACOSINE: fBasis = acos(fBasis) * (180.0f / M_PI); break;
	case CALCRATIO_TRANSFORM_ASINE: fBasis = asin(fBasis) * (180.0f / M_PI); break;
	case CALCRATIO_TRANSFORM_ATANGENT: fBasis = atan(fBasis) * (180.0f / M_PI); break;
	}

	if (!FStringNull(pev->netname)) {
		float fTmp = 0;
		if (!TryCalcLocus_Ratio( pLocus, STRING(pev->netname), fTmp))
			return false;
		fBasis += fTmp;
	}
	if (!FStringNull(pev->message)) {
		float fTmp = 1;
		if (!TryCalcLocus_Ratio( pLocus, STRING(pev->message), fTmp))
			return false;
		fBasis = fBasis * fTmp;
	}

	if (!FStringNull(m_iszMin))
	{
		float fMin = 0;
		if (!TryCalcLocus_Ratio( pLocus, STRING(m_iszMin), fMin))
			return false;

		if (!FStringNull(m_iszMax))
		{
			float fMax = 0;
			if (!TryCalcLocus_Ratio( pLocus, STRING(m_iszMax), fMax))
				return false;
			
			if (fBasis >= fMin && fBasis <= fMax) {
				*outResult = fBasis;
				return true;
			}
			switch ((int)pev->frags)
			{
			case 0:
				if (fBasis < fMin)
					*outResult = fMin;
				else
					*outResult = fMax;
				return true;
			case 1:
				while (fBasis < fMin)
					fBasis += fMax - fMin;
				while (fBasis > fMax)
					fBasis -= fMax - fMin;
				*outResult = fBasis;
				return true;
			case 2:
				while (fBasis < fMin || fBasis > fMax)
				{
					if (fBasis < fMin)
						fBasis = fMin + fMax - fBasis;
					else
						fBasis = fMax + fMax - fBasis;
				}
				*outResult = fBasis;
				return true;
			}
		}
		
		if (fBasis > fMin)
			*outResult = fBasis;
		else
			*outResult = fMin; // crop to nearest value
	}
	else if (!FStringNull(m_iszMax))
	{
		float fMax = 0;
		if (!TryCalcLocus_Ratio( pLocus, STRING(m_iszMax), fMax))
			return false;

		if (fBasis < fMax)
			*outResult = fBasis;
		else
			*outResult = fMax; // crop to nearest value
	}
	else
		*outResult = fBasis;
	return true;
}

enum
{
	CALCNUMFROM_VELOCITY = 0,
	CALCNUMFROM_POSITION = 1,
	CALCNUMFROM_COLOR = 2,
};

class CCalcNumFromVec : public CPointEntity
{
public:
	void KeyValue( KeyValueData *pkvd )
	{
		if (FStrEq(pkvd->szKeyName, "vector_type"))
		{
			pev->weapons = atoi(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else
			CPointEntity::KeyValue(pkvd);
	}
	bool CalcRatio( CBaseEntity *pLocus, float* outResult );
};

LINK_ENTITY_TO_CLASS( calc_numfromvec, CCalcNumFromVec )

bool CCalcNumFromVec::CalcRatio( CBaseEntity *pLocus, float* outResult )
{
	if ( FStringNull(pev->target) )
	{
		ALERT(at_error, "No base vector given for %s \"%s\"\n", STRING(pev->classname), STRING(pev->targetname));
		return false;
	}

	Vector vecA;
	switch (pev->weapons) {
	case CALCNUMFROM_POSITION:
		if (!TryCalcLocus_Position( this, pLocus, STRING(pev->target), vecA ))
		{
			return false;
		}
		break;
	case CALCNUMFROM_COLOR:
		if (!TryCalcLocus_Color( this, pLocus, STRING(pev->target), vecA ))
		{
			return false;
		}
		break;
	case CALCNUMFROM_VELOCITY:
	default:
		if (!TryCalcLocus_Velocity( this, pLocus, STRING(pev->target), vecA ))
		{
			return false;
		}
		break;
	}

	switch(pev->impulse)
	{
	case 0: // X
		*outResult = vecA.x; return true;
	case 1: // Y
		*outResult = vecA.y; return true;
	case 2: // Z
		*outResult = vecA.z; return true;
	case 3: // Length
		*outResult = vecA.Length(); return true;
	case 4: // Pitch
		{
			Vector ang = UTIL_VecToAngles(vecA);
			*outResult = ang.x;
			return true;
		}
	case 5: // Yaw
		{
			Vector ang = UTIL_VecToAngles(vecA);
			*outResult = ang.y;
			return true;
		}
	}

	ALERT(at_console, "%s \"%s\" doesn't understand mode %d\n", STRING(pev->classname), STRING(pev->targetname), pev->impulse);
	return false;
}


//=======================================================
#define SF_CALCVELOCITY_NORMALIZE 1
#define SF_CALCVELOCITY_SWAPZ 2
#define SF_CALCVELOCITY_DISCARDX 4
#define SF_CALCVELOCITY_DISCARDY 8
#define SF_CALCVELOCITY_DISCARDZ 16
class CCalcSubVelocity : public CPointEntity
{
	bool Convert( CBaseEntity *pLocus, Vector vecVel, Vector* outVector );
	bool ConvertAngles( CBaseEntity *pLocus, Vector vecAngles, Vector* outVector );
public:
	bool CalcVelocity( CBaseEntity *pLocus, Vector* outResult );

	void Spawn();
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszFactor;
};

LINK_ENTITY_TO_CLASS( calc_subvelocity, CCalcSubVelocity )

TYPEDESCRIPTION	CCalcSubVelocity::m_SaveData[] =
{
	DEFINE_FIELD( CCalcSubVelocity, m_iszFactor, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CCalcSubVelocity, CPointEntity )

void CCalcSubVelocity::Spawn()
{
	m_iszFactor = pev->noise;
	pev->noise = iStringNull;
}

bool CCalcSubVelocity::CalcVelocity( CBaseEntity *pLocus, Vector* outResult )
{
	pLocus = UTIL_FindEntityByTargetname( NULL, STRING(pev->netname), pLocus );
	if ( !pLocus )
	{
		ALERT(at_console, "%s \"%s\" failed to find target entity \"%s\"\n", STRING(pev->classname), STRING(pev->targetname), STRING(pev->netname) );
		return false;
	}

	Vector vecAngles;
	Vector vecJunk;

	switch (pev->impulse)
	{
	case 1: //angles
		return ConvertAngles( pLocus, pLocus->pev->angles, outResult );
	case 2: //v_angle
		return ConvertAngles( pLocus, pLocus->pev->v_angle, outResult );
	case 5:
		// this could cause problems.
		// is there a good way to check whether it's really a CBaseAnimating?
		((CBaseAnimating*)pLocus)->GetAttachment( 0, vecJunk, vecAngles );
		return ConvertAngles( pLocus, vecAngles, outResult );
	case 6:
		((CBaseAnimating*)pLocus)->GetAttachment( 1, vecJunk, vecAngles );
		return ConvertAngles( pLocus, vecAngles, outResult );
	case 7:
		((CBaseAnimating*)pLocus)->GetAttachment( 2, vecJunk, vecAngles );
		return ConvertAngles( pLocus, vecAngles, outResult );
	case 8:
		((CBaseAnimating*)pLocus)->GetAttachment( 3, vecJunk, vecAngles );
		return ConvertAngles( pLocus, vecAngles, outResult );
	default:
		return Convert( pLocus, pLocus->pev->velocity, outResult );
	}
}

bool CCalcSubVelocity::Convert( CBaseEntity *pLocus, Vector vecDir, Vector* outVector )
{
	if (pev->spawnflags & SF_CALCVELOCITY_NORMALIZE)
		vecDir = vecDir.Normalize();
	
	float fRatio = 1;
	if (m_iszFactor) {
		if (!TryCalcLocus_Ratio( pLocus, STRING(m_iszFactor), fRatio ))
			return false;
	}
	Vector vecOffset = g_vecZero;

	if (pev->message) {
		if (!TryCalcLocus_Velocity( this, pLocus, STRING(pev->message), vecOffset))
			return false;
	}

	Vector vecResult = vecOffset + (vecDir*fRatio);

	if (pev->spawnflags & SF_CALCVELOCITY_DISCARDX)
		vecResult.x = 0;
	if (pev->spawnflags & SF_CALCVELOCITY_DISCARDY)
		vecResult.y = 0;
	if (pev->spawnflags & SF_CALCVELOCITY_DISCARDZ)
		vecResult.z = 0;
	if (pev->spawnflags & SF_CALCVELOCITY_SWAPZ)
		vecResult.z = -vecResult.z;
//	ALERT(at_console, "calc_subvel returns (%f %f %f) = (%f %f %f) + ((%f %f %f) * %f)\n", vecResult.x, vecResult.y, vecResult.z, vecOffset.x, vecOffset.y, vecOffset.z, vecDir.x, vecDir.y, vecDir.z, fRatio);
	*outVector = vecResult;
	return true;
}

bool CCalcSubVelocity::ConvertAngles( CBaseEntity *pLocus, Vector vecAngles, Vector* outVector )
{
	UTIL_MakeVectors( vecAngles );
	return Convert( pLocus, gpGlobals->v_forward, outVector );
}



//=======================================================
class CCalcVelocityPath : public CPointEntity
{
public:
	bool CalcVelocity( CBaseEntity *pLocus, Vector* outVector );

	void Spawn();
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszFactor;
};

LINK_ENTITY_TO_CLASS( calc_velocity_path, CCalcVelocityPath )

TYPEDESCRIPTION	CCalcVelocityPath::m_SaveData[] =
{
	DEFINE_FIELD( CCalcVelocityPath, m_iszFactor, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CCalcVelocityPath, CPointEntity )

void CCalcVelocityPath::Spawn()
{
	m_iszFactor = pev->noise;
	pev->noise = iStringNull;
}

bool CCalcVelocityPath::CalcVelocity( CBaseEntity *pLocus, Vector* outVector )
{
	Vector vecStart = pev->origin;
	if ( !FStringNull(pev->target) )
	{
		if ( !TryCalcLocus_Position( this, pLocus, STRING(pev->target), vecStart ) )
		{
			//ALERT(at_console, "%s \"%s\" failed: bad LP \"%s\"\n", STRING(pev->classname), STRING(pev->targetname), STRING(pev->target));
			return false;
		}
	}

//	ALERT(at_console, "vecStart %f %f %f\n", vecStart.x, vecStart.y, vecStart.z);
	Vector vecOffs = g_vecZero;
	float fFactor = 1;
	if (m_iszFactor) {
		if (!TryCalcLocus_Ratio( pLocus, STRING(m_iszFactor), fFactor ))
			return false;
	}

	switch ((int)pev->armorvalue)
	{
	case 0:
		if (!TryCalcLocus_Position( this, pLocus, STRING(pev->netname), vecOffs ))
			return false;
		vecOffs = vecOffs - vecStart;
		break;
	case 1:
		if (!TryCalcLocus_Velocity( this, pLocus, STRING(pev->netname), vecOffs ))
			return false;
		break;
	}
//	ALERT(at_console, "vecOffs %f %f %f\n", vecOffs.x, vecOffs.y, vecOffs.z);

	if (pev->health)
	{
		float len = vecOffs.Length();
		switch ((int)pev->health)
		{
		case 1:
			vecOffs = vecOffs/len;
			break;
		case 2:
			vecOffs = vecOffs/(len*len);
			break;
		case 3:
			vecOffs = vecOffs/(len*len*len);
			break;
		case 4:
			vecOffs = vecOffs*len;
			break;
		}
	}

	vecOffs = vecOffs * fFactor;

	if (pev->frags)
	{
		TraceResult tr;
		IGNORE_GLASS iIgnoreGlass = ignore_glass;
		IGNORE_MONSTERS iIgnoreMonsters = ignore_monsters;

		switch ((int)pev->frags)
		{
		case 2:
			iIgnoreGlass = dont_ignore_glass;
			break;
		case 4:
			iIgnoreGlass = dont_ignore_glass;
			// fall through
		case 3:
			iIgnoreMonsters = dont_ignore_monsters;
			break;
		}

		UTIL_TraceLine( vecStart, vecStart+vecOffs, iIgnoreMonsters, iIgnoreGlass, NULL, &tr );
		vecOffs = tr.vecEndPos - vecStart;
	}

//	ALERT(at_console, "path: %f %f %f\n", vecOffs.x, vecOffs.y, vecOffs.z);
	*outVector = vecOffs;
	return true;
}


//=======================================================
class CCalcVelocityPolar : public CPointEntity
{
public:
	bool CalcVelocity( CBaseEntity *pLocus, Vector* outResult );

	void Spawn();
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszFactor;
};

LINK_ENTITY_TO_CLASS( calc_velocity_polar, CCalcVelocityPolar )

TYPEDESCRIPTION	CCalcVelocityPolar::m_SaveData[] =
{
	DEFINE_FIELD( CCalcVelocityPolar, m_iszFactor, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CCalcVelocityPolar, CPointEntity )

void CCalcVelocityPolar::Spawn()
{
	m_iszFactor = pev->noise;
	pev->noise = iStringNull;
}

bool CCalcVelocityPolar::CalcVelocity( CBaseEntity *pLocus, Vector* outResult )
{
	Vector vecBasis = g_vecZero;
	TryCalcLocus_Velocity( this, pLocus, STRING(pev->netname), vecBasis );
	Vector vecAngles = UTIL_VecToAngles( vecBasis ) + pev->angles;
	Vector vecOffset = g_vecZero;
	if (pev->message) {
		if (!TryCalcLocus_Velocity( this, pLocus, STRING(pev->message), vecOffset ))
			return false;
	}

	float fFactor = 1;
	if (m_iszFactor) {
		if (!TryCalcLocus_Ratio( pLocus, STRING(m_iszFactor), fFactor ))
			return false;
	}

	if (!(pev->spawnflags & SF_CALCVELOCITY_NORMALIZE))
		fFactor = fFactor * vecBasis.Length();

	UTIL_MakeVectors( vecAngles );
	*outResult = (gpGlobals->v_forward * fFactor) + vecOffset;
	return true;
}

//=======================================================

// Position marker
class CMark : public CPointEntity
{
public:
	bool	CalcVelocity(CBaseEntity *pLocus, Vector* outVector) { *outVector = pev->movedir; return true; }
	bool	CalcRatio(CBaseEntity *pLocus, float* outResult) { *outResult = pev->frags; return true; }
	void	Think( void ) { SUB_Remove(); }
};

class CLocusVariable : public CPointEntity
{
public:
	void	Spawn( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	bool	CalcVelocity(CBaseEntity *pLocus, Vector* outVector) { *outVector = pev->movedir; return true; }
	bool	CalcRatio(CBaseEntity *pLocus, float* outResult) { *outResult = pev->frags; return true; }

	void KeyValue( KeyValueData *pkvd );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	string_t m_iszPosition;
	string_t m_iszVelocity;
	string_t m_iszRatio;
	string_t m_iszTargetName;
	string_t m_iszFireOnSpawn;
	float m_fDuration;
};

TYPEDESCRIPTION	CLocusVariable::m_SaveData[] = 
{
	DEFINE_FIELD( CLocusVariable, m_iszPosition, FIELD_STRING),
	DEFINE_FIELD( CLocusVariable, m_iszVelocity, FIELD_STRING),
	DEFINE_FIELD( CLocusVariable, m_iszRatio, FIELD_STRING),
	DEFINE_FIELD( CLocusVariable, m_iszTargetName, FIELD_STRING),
	DEFINE_FIELD( CLocusVariable, m_iszFireOnSpawn, FIELD_STRING),
	DEFINE_FIELD( CLocusVariable, m_fDuration, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE( CLocusVariable, CPointEntity )
LINK_ENTITY_TO_CLASS( locus_variable, CLocusVariable )

void CLocusVariable :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iszPosition"))
	{
		m_iszPosition = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszVelocity"))
	{
		m_iszVelocity = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszRatio"))
	{
		m_iszRatio = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszTargetName"))
	{
		m_iszTargetName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszFireOnSpawn"))
	{
		m_iszFireOnSpawn = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fDuration"))
	{
		m_fDuration = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CLocusVariable::Spawn( void )
{
	SetMovedir(pev);
}

void CLocusVariable::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Vector vecPos = g_vecZero;
	Vector vecDir = g_vecZero;
	float fRatio = 0;
	if (m_iszPosition)
		TryCalcLocus_Position(this, pActivator, STRING(m_iszPosition), vecPos);
	if (m_iszVelocity)
		TryCalcLocus_Velocity(this, pActivator, STRING(m_iszVelocity), vecDir);
	if (m_iszRatio)
		TryCalcLocus_Ratio(pActivator, STRING(m_iszRatio), fRatio);

	if (m_iszTargetName)
	{
		CMark *pMark = GetClassPtr( (CMark*)NULL );
		pMark->pev->classname = MAKE_STRING("mark");
		pMark->pev->origin = vecPos;
		pMark->pev->movedir = vecDir;
		pMark->pev->frags = fRatio;
		pMark->pev->targetname = m_iszTargetName;
		pMark->pev->nextthink = gpGlobals->time + m_fDuration;

		FireTargets(STRING(m_iszFireOnSpawn), pMark, this);
	}
	else
	{
		pev->origin = vecPos;
		pev->movedir = vecDir;
		pev->frags = fRatio;

		FireTargets(STRING(m_iszFireOnSpawn), this, this);
	}
}

class CCalcEvalNumber : public CPointEntity
{
public:
	enum {
		EVAL_ADD = 0,
		EVAL_SUBSTRUCT,
		EVAL_MULTIPLY,
		EVAL_DIVIDE,
	};

	void KeyValue( KeyValueData *pkvd );
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	bool CalcRatio(CBaseEntity *pLocus, float *outResult)
	{
		bool success;
		*outResult = CalcEvalNumber(pLocus, false, success);
		return success;
	}

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

protected:
	float CalcEvalNumber(CBaseEntity* pActivator, bool isUse, bool& success);
	float DoOperation(float leftValue, float rightValue, int operationId, bool& success);

	string_t m_left;
	string_t m_right;
	int m_operation;
	string_t m_storeIn;
};

TYPEDESCRIPTION CCalcEvalNumber::m_SaveData[] =
{
	DEFINE_FIELD( CCalcEvalNumber, m_left, FIELD_STRING ),
	DEFINE_FIELD( CCalcEvalNumber, m_right, FIELD_STRING ),
	DEFINE_FIELD( CCalcEvalNumber, m_operation, FIELD_INTEGER ),
	DEFINE_FIELD( CCalcEvalNumber, m_storeIn, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CCalcEvalNumber, CPointEntity )

LINK_ENTITY_TO_CLASS( calc_eval_number, CCalcEvalNumber )

void CCalcEvalNumber::KeyValue(KeyValueData *pkvd)
{
	if(strcmp(pkvd->szKeyName, "operation") == 0)
	{
		m_operation = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if(strcmp(pkvd->szKeyName, "left_operand") == 0)
	{
		m_left = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if(strcmp(pkvd->szKeyName, "right_operand") == 0)
	{
		m_right = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if(strcmp(pkvd->szKeyName, "store_result") == 0)
	{
		m_storeIn = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CCalcEvalNumber::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	bool success;
	CalcEvalNumber(pActivator, true, success);
}

float CCalcEvalNumber::CalcEvalNumber(CBaseEntity *pActivator, bool isUse, bool &success)
{
	if (FStringNull(m_left) || FStringNull(m_right))
	{
		ALERT(at_error, "%s needs both left and right operands defined\n", STRING(pev->classname));
		success = false;
		return 0.0f;
	}

	float leftValue = 0;
	float rightValue = 0;

	const bool leftSuccess = TryCalcLocus_Ratio(pActivator, STRING(m_left), leftValue);
	const bool rightSuccess = TryCalcLocus_Ratio(pActivator, STRING(m_right), rightValue);

	if (!leftSuccess || !rightSuccess) {
		success = false;
		return 0.0f;
	}

	const float result = DoOperation(leftValue, rightValue, m_operation, success);

	if (isUse)
	{
		if (m_storeIn)
			FireTargets(STRING(m_storeIn), pActivator, this, USE_SET, result);
		if (pev->target)
			FireTargets(STRING(pev->target), pActivator, this);
	}

	return result;
}

float CCalcEvalNumber::DoOperation(float leftValue, float rightValue, int operationId, bool &success)
{
	success = true;
	switch (operationId) {
	case EVAL_ADD:
		return leftValue + rightValue;
	case EVAL_SUBSTRUCT:
		return leftValue - rightValue;
	case EVAL_MULTIPLY:
		return leftValue * rightValue;
	case EVAL_DIVIDE:
		if (rightValue == 0)
		{
			success = false;
			return 0.0f;
		}
		return leftValue / rightValue;
	default:
		ALERT(at_error, "%s: unknown operation id %d\n", STRING(pev->classname), operationId);
		success = false;
		return 0.0f;
	}
}
