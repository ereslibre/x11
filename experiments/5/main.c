#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#include <iostream>
#include <functional>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv)
{
    Display *dpy = XOpenDisplay(0);

    if (!dpy) {
        return -1;
    }

    const int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

    Window window = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 400, 400, 0, whiteColor, whiteColor);
    XTextProperty prop;
    prop.value = (unsigned char*) "Testing";
    prop.encoding = XA_STRING;
    prop.format = 8;
    prop.nitems = strlen((char*) prop.value);
    XSetWMName(dpy, window, &prop);
    XSelectInput(dpy, window, StructureNotifyMask | VisibilityChangeMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ExposureMask);
    XMapWindow(dpy, window);

    int eventBase;
    int errorBase;
    const bool renderExt = XRenderQueryExtension(dpy, &eventBase, &errorBase);

    if (renderExt) {
        std::cout << "there is render extension enabled :)" << std::endl;
    }

    while (true) {
        XEvent e;
        XNextEvent(dpy, &e);
        if (e.type == VisibilityNotify ||
            e.type == ConfigureNotify) {
            if (e.type == ConfigureNotify) {
                std::cout << "window being resized" << std::endl;
            }
            XClearWindow(dpy, e.xany.window);
            if (renderExt) {
                XRenderColor color = { 0xffff, 0x0000, 0x0000, 0x5fff };
                Picture cyan = XRenderCreateSolidFill(dpy, &color);

                XRenderPictFormat *fmt = XRenderFindStandardFormat(dpy, PictStandardRGB24);
                XRenderPictureAttributes pict_attr;
                pict_attr.poly_edge = PolyEdgeSmooth;
                pict_attr.poly_mode = PolyModeImprecise;
                Picture picture = XRenderCreatePicture(dpy, window, fmt, CPPolyEdge | CPPolyMode, &pict_attr);
                XRenderFillRectangle(dpy, PictOpOver, picture, &color, 10, 10, 300, 300);

                XRenderPictFormat *fmt2 = XRenderFindStandardFormat(dpy, PictStandardA8);
                XPointDouble line[10];
                line[0].x = 20;
                line[0].y = 60;
                line[1].x = 140;
                line[1].y = 30;
                line[2].x = 300;
                line[2].y = 30;
                line[3].x = 260;
                line[3].y = 160;
                line[4].x = 380;
                line[4].y = 160;
                line[5].x = 340;
                line[5].y = 200;
                line[6].x = 340;
                line[6].y = 240;
                line[7].x = 260;
                line[7].y = 240;
                line[8].x = 180;
                line[8].y = 200;
                line[9].x = 140;
                line[9].y = 360;
                XRenderCompositeDoublePoly(dpy, PictOpOver, cyan, picture, fmt2, 0, 0, 0, 0, line, 10, 0);
            }
            XFlush(dpy);
        }
    }

    return 0;
}
