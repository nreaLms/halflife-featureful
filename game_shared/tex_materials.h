#pragma once
#ifndef TEX_MATERIALS_H
#define TEX_MATERIALS_H

int GetTextureMaterialProperties(char chTextureType, float* fvol, float* fvolbar, const char* rgsz[4], int* cnt, float* fattn, int iBulletType);

void GetStrippedTextureName(char* szbuffer, const char* pTextureName);

#endif
