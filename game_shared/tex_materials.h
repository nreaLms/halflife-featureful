#pragma once
#ifndef TEX_MATERIALS_H
#define TEX_MATERIALS_H

bool GetTextureMaterialProperties(char chTextureType, float& fvol, float& fvolbar,
                                  const char* rgsz[4], int& cnt, float &fattn, int iBulletType);

void StrippedTextureName(char* szbuffer, const char* pTextureName);

#endif
