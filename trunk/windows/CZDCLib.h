#include "../client/Util.h"
#include <math.h>

#ifndef __CZDCLIB_H
#define __CZDCLIB_H

typedef struct tagHLSTRIPLE {
	DOUBLE hlstHue;
	DOUBLE hlstLightness;
	DOUBLE hlstSaturation;
} HLSTRIPLE;

class CZDCLib {

public:
	static bool shutDown(int action);
	static bool isXp();

	static int getFirstSelectedIndex(CListViewCtrl& list);
	static int setButtonPressed(int nID, bool bPressed = true);
	static void CalcTextSize(const string& text, HFONT font, LPSIZE size);

private:
	static bool bIsXP;
	static bool bGotXP;
	static int iWinVerMajor;
};

const MAX_SHADE = 44;
const SHADE_LEVEL = 90;
const int blend_vector[MAX_SHADE] = {0, 4, 8, 10, 5, 2, 0, -1, -2, -3, -5, -6, -7, -8, -7, -6, -5, -4, -3, -2, -1, 0, 
1, 2, 3, 4, 5, 6, 7, 8, 7, 6, 5, 3, 2, 1, 0, -2, -5, -10, -8, -4, 0};

class OperaColors {
public:
	static inline BYTE getRValue(const COLORREF& cr) { return (BYTE)(cr & 0xFF); }
	static inline BYTE getGValue(const COLORREF& cr) { return (BYTE)(((cr & 0xFF00) >> 8) & 0xFF); }
	static inline BYTE getBValue(const COLORREF& cr) { return (BYTE)(((cr & 0xFF0000) >> 16) & 0xFF); }
	static double RGB2HUE(double red, double green, double blue);
	static RGBTRIPLE HUE2RGB(double m0, double m2, double h);
	static HLSTRIPLE RGB2HLS(BYTE red, BYTE green, BYTE blue);
	static inline HLSTRIPLE RGB2HLS(COLORREF c) { return RGB2HLS(getRValue(c), getGValue(c), getBValue(c)); }
	static RGBTRIPLE HLS2RGB(double hue, double lightness, double saturation);
	static inline RGBTRIPLE HLS2RGB(HLSTRIPLE hls) { return HLS2RGB(hls.hlstHue, hls.hlstLightness, hls.hlstSaturation); }
	static inline COLORREF RGB2REF(const RGBTRIPLE& c) { return RGB(c.rgbtRed, c.rgbtGreen, c.rgbtBlue); }
	static inline COLORREF blendColors(const COLORREF& cr1, const COLORREF& cr2, double balance = 0.5) {
		BYTE r1 = getRValue(cr1);
		BYTE g1 = getGValue(cr1);
		BYTE b1 = getBValue(cr1);
		BYTE r2 = getRValue(cr2);
		BYTE g2 = getGValue(cr2);
		BYTE b2 = getBValue(cr2);
		return RGB(
			(r1*(balance*2) + (r2*((1-balance)*2))) / 2,
			(g1*(balance*2) + (g2*((1-balance)*2))) / 2,
			(b1*(balance*2) + (b2*((1-balance)*2))) / 2
			);
	}
	static inline COLORREF brightenColor(const COLORREF& c, double brightness = 0) {
		if (brightness == 0) {
			return c;
		} else if (brightness > 0) {
			BYTE r = getRValue(c);
			BYTE g = getGValue(c);
			BYTE b = getBValue(c);
			return RGB(
				(r+((255-r)*brightness)),
				(g+((255-g)*brightness)),
				(b+((255-b)*brightness))
				);
		} else {
			return RGB(
				(getRValue(c)*(1+brightness)),
				(getGValue(c)*(1+brightness)),
				(getBValue(c)*(1+brightness))
				);
		}
	}
	static inline void FloodFill(CDC& hDC, int x1, int y1, int x2, int y2, COLORREF c1, COLORREF c2, bool light = true) {
		if (x2 <= x1 || y2 <= y1)
			return;

		if (!light) {
			for (int _x = x1; _x <= x2; ++_x) {
				CRect r(_x, y1, _x + 1, y2);
				hDC.FillSolidRect(&r, blendColors(c2, c1, (double)(_x - x1) / (double)(x2 - x1)));
			}
		} else {
			int height = y2 - y1;
			size_t ci_1;
			// Allocate shade-constants
			double* c = new double[height];
			// Calculate constants
			for (int i = 0; i < height; ++i) {
				ci_1 = (size_t)floor(((double)(i + 1) / height) * MAX_SHADE - 1);
				c[i] = (double)(blend_vector[ci_1] + blend_vector[ci_1]) / (double)(SHADE_LEVEL);
			}
			int delta_x = x2 - x1;
			for (int _x = x1; _x <= x2; ++_x) {
				COLORREF cr = blendColors(c2, c1, (double)(_x - x1) / (double)(delta_x));
				for (int _y = y1; _y < y2; ++_y) {
					hDC.SetPixelV(_x, _y, brightenColor(cr, c[_y - y1]));
				}
			}
			delete[] c;
		}
	}
	static void EnlightenFlood(const COLORREF& clr, COLORREF& a, COLORREF& b);
	static COLORREF TextFromBackground(COLORREF bg);

private:
};

#endif // __CZDCLIB_H