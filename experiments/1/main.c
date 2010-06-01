#include <X11/Xlib.h>
#include <iostream>

int main(int argc, char **argv)
{
    Display *dpy = XOpenDisplay(0);

    if (!dpy) {
        return -1;
    }

    std::cout << "we have " << XScreenCount(dpy) << " screens" << std::endl;
    std::cout << "server vendor: " << XServerVendor(dpy) << " - release: " << XVendorRelease(dpy) << std::endl;

    Screen *scr = XDefaultScreenOfDisplay(dpy);

    std::cout << "width: " << XWidthOfScreen(scr) << " - height: " << XHeightOfScreen(scr) << std::endl;

    return 0;
}
