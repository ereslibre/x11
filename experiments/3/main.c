#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <iostream>
#include <unistd.h>
/*
Picture create_pen(int red, int green, int blue, int alpha, Display *dpy, Window window)
{
    XRenderColor color;
    color.red = red;
    color.green = green;
    color.blue = blue;
    color.alpha = alpha;
    XRenderPictFormat *fmt = XRenderFindStandardFormat(dpy, PictStandardARGB32);
    Pixmap pm = XCreatePixmap(dpy, window, 1, 1, 32);
    XRenderPictureAttributes pict_attr;
    pict_attr.repeat = 1;
    Picture picture = XRenderCreatePicture(dpy, pm, fmt, CPRepeat, &pict_attr);
    XRenderFillRectangle(dpy, PictOpOver, picture, &color, 0, 0, 1, 1);
    return picture;
}
*/

#include <ft2build.h>
#include FT_FREETYPE_H

void load_glyph(Display *display, GlyphSet gs, FT_Face face, int charcode)
{
	Glyph gid;
	XGlyphInfo ginfo;
	
	int glyph_index=FT_Get_Char_Index(face, charcode);
	FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT);
	
	FT_Bitmap *bitmap=&face->glyph->bitmap;
	ginfo.x=-face->glyph->bitmap_left;
	ginfo.y=face->glyph->bitmap_top;
	ginfo.width=bitmap->width;
	ginfo.height=bitmap->rows;
	ginfo.xOff=face->glyph->advance.x/64;
	ginfo.yOff=face->glyph->advance.y/64;
	
	gid=charcode;
	
	int stride=(ginfo.width+3)&~3;
	char tmpbitmap[stride*ginfo.height];
	int y;
	for(y=0; y<ginfo.height; y++)
		memcpy(tmpbitmap+y*stride, bitmap->buffer+y*ginfo.width, ginfo.width);
	
	XRenderAddGlyphs(display, gs, &gid, &ginfo, 1,
		tmpbitmap, stride*ginfo.height);
	XSync(display, 0);
}

GlyphSet load_glyphset(Display *display, char *filename, int size)
{
	static int ft_lib_initialized = 0;
	static FT_Library library;
	int n;
	XRenderPictFormat *fmt_a8 = XRenderFindStandardFormat(display, PictStandardA8);
	GlyphSet gs = XRenderCreateGlyphSet(display, fmt_a8);
	
	if (!ft_lib_initialized)
		FT_Init_FreeType(&library);
	
	FT_Face face;
	
	FT_New_Face(library,
		filename,
		0, &face);
	
	FT_Set_Char_Size(
		face, 0, size*64,
		90, 90);
	
	for (n = 32; n < 128; n++) {
		load_glyph(display, gs, face, n);
    }
	
	FT_Done_Face(face);
	
	return gs;
}

int main(int argc, char **argv)
{
    Display *dpy = XOpenDisplay(0);

    if (!dpy) {
        return -1;
    }

    const int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
    const int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

    Window window = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 400, 400, 0, whiteColor, whiteColor);
    XSelectInput(dpy, window, StructureNotifyMask | VisibilityChangeMask | KeyPressMask | KeyReleaseMask | ExposureMask);
    XMapWindow(dpy, window);

    GC gc = XCreateGC(dpy, window, 0, 0);
    XSetForeground(dpy, gc, blackColor);

    int eventBase;
    int errorBase;
    const bool renderExt = XRenderQueryExtension(dpy, &eventBase, &errorBase);

    if (renderExt) {
        std::cout << "there is render extension enabled :)" << std::endl;
    }

    while (true) {
        XEvent e;
        XNextEvent(dpy, &e);
        std::cout << "receiving event " << e.type << std::endl;
        if (e.type == VisibilityNotify) {
            XDrawLine(dpy, window, gc, 10, 60, 180, 20);
            XDrawLine(dpy, window, gc, 10, 20, 180, 60);
            XDrawRectangle(dpy, window, gc, 100, 100, 200, 200);
            if (renderExt) {
                XRenderColor color = { 0x00ab, 0x5fff, 0x25ff, 0xe000 };
                Picture cyan = XRenderCreateSolidFill(dpy, &color);
                XRenderPictFormat *fmt = XRenderFindStandardFormat(dpy, PictStandardRGB24);
                XRenderPictureAttributes pict_attr;
                pict_attr.poly_edge = PolyEdgeSmooth;
                pict_attr.poly_mode = PolyModeImprecise;
                Picture picture = XRenderCreatePicture(dpy, window, fmt, CPPolyEdge | CPPolyMode, &pict_attr);
                XTriangle tr;
                tr.p1.x = 200 << 16;
                tr.p1.y = 50 << 16;
                tr.p2.x = 350 << 16;
                tr.p2.y = 350 << 16;
                tr.p3.x = 50 << 16;
                tr.p3.y = 350 << 16;
                XRenderCompositeTriangles(dpy, PictOpOver, cyan, picture, 0, 0, 0, &tr, 1);
                GlyphSet glyphSet = load_glyphset(dpy, "/home/ereslibre/.fonts/Lucida Grande.ttf", 30);
                XRenderCompositeString8(dpy, PictOpOver, cyan, picture, 0, glyphSet, 0, 0, 20, 390, "Probando FreeType", 18);
            }
            XFlush(dpy);
        }
    }

    return 0;
}
