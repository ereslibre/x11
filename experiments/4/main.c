#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <iostream>
#include <functional>
#include <unistd.h>

int main(int argc, char **argv)
{
    Display *dpy = XOpenDisplay(0);

    if (!dpy) {
        return -1;
    }

    const int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
    const int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

    Window window = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 400, 400, 0, whiteColor, whiteColor);
    XSelectInput(dpy, window, StructureNotifyMask | VisibilityChangeMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ExposureMask);
    XMapWindow(dpy, window);

    //Window window2 = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 400, 400, 0, whiteColor, whiteColor);
    //XSelectInput(dpy, window2, StructureNotifyMask | VisibilityChangeMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ExposureMask);
    //XMapWindow(dpy, window2);

    XGCValues gcv;
    gcv.line_style = LineOnOffDash;
    gcv.line_width = 10;

    GC gc = XCreateGC(dpy, window, GCLineWidth | GCLineStyle, &gcv);
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
        std::cout << "receiving event " << e.type << " on window " << e.xany.window << std::endl;
        if (e.type == VisibilityNotify) {
            XClearWindow(dpy, e.xany.window);
            //XDrawLine(dpy, e.xany.window, gc, 10, 60, 180, 20);
            //XDrawLine(dpy, e.xany.window, gc, 10, 20, 180, 60);
            //XDrawRectangle(dpy, e.xany.window, gc, 100, 100, 200, 200);
            if (renderExt) {
                XRenderColor color = { 0xffff, 0x0000, 0x0000, 0x5fff };
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
                //XRenderCompositeTriangles(dpy, PictOpOver, cyan, picture, 0, 0, 0, &tr, 1);

                XRenderPictFormat *fmt2 = XRenderFindStandardFormat(dpy, PictStandardA8);
                XPointDouble line[4];
                line[0].x = 60;
                line[0].y = 30;
                line[1].x = 370;
                line[1].y = 60;
                line[2].x = 120;
                line[2].y = 380;
                line[3].x = 20;
                line[3].y = 320;
                XRenderCompositeDoublePoly(dpy, PictOpOver, cyan, picture, fmt2, 0, 0, 0, 0, line, 4, 0);

                XRenderFillRectangle(dpy, PictOpOver, picture, &color, 10, 10, 300, 300);
            }
            XFlush(dpy);
        }
    }

    return 0;
}
