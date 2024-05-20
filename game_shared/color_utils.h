#pragma once
#ifndef COLOR_UTILS_H
#define COLOR_UTILS_H

inline void UnpackRGB( int &r, int &g, int &b, unsigned long ulRGB )
{
	r = ( ulRGB & 0xFF0000 ) >> 16;
	g = ( ulRGB & 0xFF00 ) >> 8;
	b = ulRGB & 0xFF;
}

inline int PackRGB(int r, int g, int b)
{
	return ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
}

#endif
