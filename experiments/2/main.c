#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <iostream>
#include <unistd.h>

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

    while (true) {
        XEvent e;
        XNextEvent(dpy, &e);
        std::cout << "receiving event " << e.type << std::endl;
        if (e.type == ConfigureNotify || e.type == VisibilityNotify || e.type == Expose) {
            XDrawLine(dpy, window, gc, 10, 60, 180, 20);
            XDrawLine(dpy, window, gc, 10, 20, 180, 60);
            XDrawRectangle(dpy, window, gc, 100, 100, 200, 200);
            if (renderExt) {
                std::cout << "there is render extension enabled :)" << std::endl;
                Picture cyan = create_pen(0, 0, 0xffff, 0xffff, dpy, window);
                XRenderPictFormat *fmt = XRenderFindStandardFormat(dpy, PictStandardRGB24);
                XRenderPictureAttributes pict_attr;
                pict_attr.poly_edge = PolyEdgeSmooth;
                pict_attr.poly_mode = PolyModeImprecise;
                Picture picture = XRenderCreatePicture(dpy, window, fmt, CPPolyEdge | CPPolyMode, &pict_attr);
                XTriangle tr;
                tr.p1.x = 50 << 16;
                tr.p1.y = 50 << 16;
                tr.p2.x = 200 << 16;
                tr.p2.y = 100 << 16;
                tr.p3.x = 100 << 16;
                tr.p3.y = 200 << 16;
                XRenderCompositeTriangles(dpy, PictOpOver, cyan, picture, 0, 0, 0, &tr, 1);
            }
            XFlush(dpy);
        }
    }

    return 0;
}
