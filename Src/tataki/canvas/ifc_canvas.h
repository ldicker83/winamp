#ifndef NULLSOFT_TATAKI_IFC_CANVAS_H
#define NULLSOFT_TATAKI_IFC_CANVAS_H

#include <bfc/dispatch.h>
#include <bfc/platform/platform.h>
#include <bfc/wasabi_std.h>

// abstract base class: safe to use in API
class NOVTABLE ifc_canvas : public Dispatchable
{
protected:
	ifc_canvas()
	{} // protect constructor
	~ifc_canvas()
	{}

public:
	DISPATCH_CODES
	{
	  GETHDC	= 100,
	  GETROOTWND	= 200,
	  GETBITS	= 300,
	  GETOFFSETS	= 400,
	  ISFIXEDCOORDS	= 500,
	  GETDIM	= 600,
	  GETTEXTFONT	= 700,
	  GETTEXTSIZE	= 710,
	  GETTEXTBOLD	= 720,
	  GETTEXTOPAQUE	= 730,
	  GETTEXTALIGN	= 740,
	  GETTEXTCOLOR	= 750,
	  GETTEXTBKCOLOR	= 760,
	  GETTEXTAA = 770,
	  GETTEXTUNDERLINE	= 780,
	  GETTEXTITALIC	= 790,
	  GETCLIPBOX	= 800,
	};
public:
	HDC getHDC();
	void *getBits();
	void getOffsets(int *x, int *y);
	bool isFixedCoords(); //FG> allows onPaint to handle double buffers as well as normal DCs
	bool getDim(int *w, int *h = NULL, int *p = NULL);  // w & h in pixels, pitch in bytes. 0 on success.
	int getClipBox(RECT *r); // returns 0 if no clipping region
};


inline HDC ifc_canvas::getHDC()
{
	return _call(ifc_canvas::GETHDC, (HDC)0);
}
inline void *ifc_canvas::getBits()
{
	return _call(ifc_canvas::GETBITS, (void *)0);
}
inline void ifc_canvas::getOffsets(int *x, int *y)
{
	_voidcall(ifc_canvas::GETOFFSETS, x, y);
}
inline bool ifc_canvas::isFixedCoords()
{ //FG> allows onPaint to handle double buffers as well as normal DCs
	return _call(ifc_canvas::ISFIXEDCOORDS, false);
}
inline bool ifc_canvas::getDim(int *w, int *h, int *p)
{ // w & h in pixels, pitch in bytes. 0 on success.
	return _call(ifc_canvas::GETDIM, false, w, h, p);
}
inline int ifc_canvas::getClipBox(RECT *r)
{ // returns 0 if no clipping region
	return _call(ifc_canvas::GETCLIPBOX, 0, r);
}

typedef ifc_canvas api_canvas;
#endif
