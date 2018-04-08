#pragma once
#ifndef TEX_MATERIALS_H
#define TEX_MATERIALS_H

#if __cplusplus
extern "C"
#endif
int GetTextureMaterialProperties(char chTextureType, float* fvol, float* fvolbar,
                                  const char* rgsz[4], int* cnt, float* fattn, int iBulletType);

#if __cplusplus
extern "C"
#endif
void GetStrippedTextureName(char* szbuffer, const char* pTextureName);

#endif
