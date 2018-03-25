#pragma once
#ifndef HL_EVENTS_H
#define HL_EVENTS_H

#include "mod_features.h"

extern "C"
{
// HLDM
void EV_FireGlock1( struct event_args_s *args );
void EV_FireGlock2( struct event_args_s *args );
void EV_FireShotGunSingle( struct event_args_s *args );
void EV_FireShotGunDouble( struct event_args_s *args );
void EV_FireMP5( struct event_args_s *args );
void EV_FireMP52( struct event_args_s *args );
void EV_FirePython( struct event_args_s *args );
void EV_FireGauss( struct event_args_s *args );
void EV_SpinGauss( struct event_args_s *args );
void EV_Crowbar( struct event_args_s *args );
void EV_FireCrossbow( struct event_args_s *args );
void EV_FireCrossbow2( struct event_args_s *args );
void EV_FireRpg( struct event_args_s *args );
void EV_EgonFire( struct event_args_s *args );
void EV_EgonStop( struct event_args_s *args );
void EV_HornetGunFire( struct event_args_s *args );
void EV_TripmineFire( struct event_args_s *args );
void EV_SnarkFire( struct event_args_s *args );

void EV_TrainPitchAdjust( struct event_args_s *args );

#if FEATURE_DESERT_EAGLE
void EV_FireEagle( struct event_args_s *args );
#endif
#if FEATURE_PIPEWRENCH
void EV_PipeWrench( struct event_args_s *args );
#endif
#if FEATURE_KNIFE
void EV_Knife( struct event_args_s *args );
#endif
#if FEATURE_M249
void EV_FireM249( struct event_args_s *args );
#endif
#if FEATURE_SNIPERRIFLE
void EV_FireSniper( struct event_args_s *args );
#endif
#if FEATURE_DISPLACER
void EV_Displacer( struct event_args_s *args );
#endif
#if FEATURE_SHOCKRIFLE
void EV_ShockFire( struct event_args_s *args );
#endif
#if FEATURE_SPORELAUNCHER
void EV_SporeFire( struct event_args_s *args );
#endif
#if FEATURE_MEDKIT
void EV_MedkitFire( struct event_args_s *args );
#endif
}

#endif
