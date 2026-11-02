//NONPORTABLE: the interface is portable, but the implementation sure isn't
#ifndef _CANVAS_H
#define _CANVAS_H

#if defined _WIN64 || defined _WIN32
#include <ddraw.h>
#endif

//#include <bfc/common.h>
#include <tataki/export.h>

class Canvas;
class MemCanvasBmp;
class api_region;
class SkinBitmap;

#include <bfc/stack.h>
#include <bfc/dispatch.h>

enum {
#ifdef WIN32
  PENSTYLE_SOLID = PS_SOLID,
  PENSTYLE_DASH = PS_DASH,
  PENSTYLE_DOT = PS_DOT,
#else
  PENSTYLE_SOLID = LineSolid,
  PENSTYLE_DASH = LineDoubleDash,
  PENSTYLE_DOT = LineDoubleDash,
#endif
};

#include <tataki/canvas/ifc_canvas.h>
class ifc_canvas;
class RegionI;
typedef struct
{
	int style;
	int width;
	COLORREF color;
	HPEN hpen;
}
penstruct;

class TATAKIAPI NOVTABLE Canvas : public ifc_canvas
{
protected:
	Canvas();
public:
	virtual ~Canvas();

// ifc_canvas stuff
	HDC getHDC();
	void *getBits();
	void getOffsets(int *x, int *y);
	bool isFixedCoords();
	bool getDim(int *w, int *h, int *p);
// end ifc_canvas stuff

	// graphics commands
	void fillRect(const RECT *r, COLORREF color);
	void fillRectAlpha(const RECT *r, COLORREF color, int alpha);
	void fillRgn(RegionI *r, COLORREF color);
	void drawRect(const RECT *r, int solid, COLORREF color, int alpha = 255);

	void pushPen(COLORREF color);
	void pushPen(int style, int width, COLORREF color);
	void popPen();

	int getPenStyle();
	COLORREF getPenColor();
	int getPenWidth();

	void selectClipRgn(api_region *r);
	int getClipBox(RECT *r); // returns 0 if no clipping region
	int getClipRgn(api_region *r); // returns 0 if no clipping region

	// Deprecated?
	void moveTo(int x, int y);
	void lineTo(int x, int y);

	void lineDraw(int fromX, int fromY, int toX, int toY);

	void drawSysObject(const RECT *r, int sysobj, int alpha = 255);

	void blit(int srcx, int srcy, Canvas *dest, int dstx, int dsty, int dstw, int dsth);
	void blitAlpha(ifc_canvas *canvas, int x, int y, int alpha = 255);
	void blitToRect(ifc_canvas *canvas, RECT *src, RECT *dst, int alpha = 255);
	void stretch(ifc_canvas *canvas, int x, int y, int w, int h);
	// src* are in 16.16 fixed point
	void stretchblit(int srcx, int srcy, int srcw, int srch, Canvas *dest, int dstx, int dsty, int dstw, int dsth);
	void stretchToRectAlpha(ifc_canvas *canvas, RECT *src, RECT *dst, int alpha = 255);
	void antiAliasTo(Canvas *dest, int w, int h, int aafactor);

	int getXOffset() const
	{
		return xoffset;
	}
	int getYOffset() const
	{
		return yoffset;
	}
	void offsetRect(RECT *r);
	void debug();

	void colorToColor(COLORREF from, COLORREF to, RECT *r);
	double getSystemFontScale();
	static void premultiply(ARGB32 *m_pBits, int nwords, int newalpha = -1);

protected:
	RECVS_DISPATCH;

	HDC hdc;
	void *bits;
	int width, height, pitch;
	bool fcoord;
	int xoffset, yoffset;

private:
	Stack<penstruct> penstack;

	int penstyle;
	COLORREF pencolor;
	int penwidth;

#ifdef WIN32
	HPEN defpen;
	HPEN curpen;
#endif
#ifdef LINUX
	int raster_x, raster_y;
#endif

};

namespace DrawSysObj
{
enum {
  BUTTON, BUTTON_PUSHED, BUTTON_DISABLED,
  OSBUTTON, OSBUTTON_PUSHED, OSBUTTON_DISABLED,
  OSBUTTON_CLOSE, OSBUTTON_CLOSE_PUSHED, OSBUTTON_CLOSE_DISABLED,
  OSBUTTON_MINIMIZE, OSBUTTON_MINIMIZE_PUSHED, OSBUTTON_MINIMIZE_DISABLED,
  OSBUTTON_MAXIMIZE, OSBUTTON_MAXIMIZE_PUSHED, OSBUTTON_MAXIMIZE_DISABLED,
};
};

class TATAKIAPI PaintCanvas : public Canvas
{
public:
	PaintCanvas();
	virtual ~PaintCanvas();

	int beginPaint(HWND wnd);
	void getRcPaint(RECT *r);

private:	// NONPORTABLE
	HWND hWnd;
	PAINTSTRUCT ps;
};

class TATAKIAPI MemCanvas : public Canvas
{
public:
	MemCanvas();
	virtual ~MemCanvas();

	int createCompatible(Canvas *canvas);
private:
};

class TATAKIAPI DCCanvas : public Canvas
{
public:
	DCCanvas(HDC clone = NULL);
	virtual ~DCCanvas();

	int cloneDC(HDC clone);
};

class TATAKIAPI SysCanvas : public Canvas
{
public:
	SysCanvas();
	virtual ~SysCanvas();
};

class TATAKIAPI DCBltCanvas : public Canvas
{
public:
	DCBltCanvas();
	virtual ~DCBltCanvas();

	int cloneDC(HDC clone, RECT *r);
	int setOrigDC(HDC neworigdc); // set to null to prevent commitdc on delete, non null to change destination dc
	int commitDC(void);						// allows commit to DC without deleting
#if 0
	int cloneCanvas(ifc_canvas *clone, RECT *r);
#endif

protected:
	HDC origdc;
	RECT rect;
	HBITMAP hbmp, prevbmp;
};

class TATAKIAPI DCExBltCanvas : public DCBltCanvas
{
public:
	DCExBltCanvas(HWND hWnd,        HRGN hrgnClip,    DWORD flags);

	~DCExBltCanvas();
private:
	HWND hwnd;
};


class TATAKIAPI BaseCloneCanvas : public Canvas
{
public:
	BaseCloneCanvas(ifc_canvas *cloner = NULL);
	virtual ~BaseCloneCanvas();

	int clone(ifc_canvas *cloner);
};



#ifdef WIN32 
class TATAKIAPI DDSurfaceCanvas : public Canvas
{

public:

	DDSurfaceCanvas(LPDIRECTDRAWSURFACE surface, int w, int h);
	virtual ~DDSurfaceCanvas();

	int isready();
	void enter();
	void exit();

private:
	LPDIRECTDRAWSURFACE surf;
	int _w, _h;
};
#endif

class TATAKIAPI BitsCanvas : public Canvas
{
public:
	BitsCanvas(void *_bits, int _w, int _h)
	{
		bits=_bits;
		width=_w;
		height=_h;
		pitch=_w;
	}
};

enum
{
  ALIGN_LEFT,
  ALIGN_CENTER,
  ALIGN_RIGHT
};

#endif
