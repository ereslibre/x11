#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

#define panic(x...) do {fprintf(stderr, x); abort();} while(0)

Display *display;
XRenderPictFormat *fmt;
int depth;

Window window;
Pixmap background;
Picture drawingarea, white_brush, red_brush, blue_brush;

Picture create_brush(int red, int green, int blue, int alpha)
{
	XRenderColor color={.red=red, .green=green, .blue=blue, .alpha=alpha};
	
	Pixmap pm=XCreatePixmap(display, window, 1, 1, depth);
	XRenderPictureAttributes pict_attr;
	pict_attr.repeat=1;
	Picture picture=XRenderCreatePicture(display, pm, fmt, CPRepeat, &pict_attr);
	XRenderFillRectangle(display, PictOpOver, picture, &color, 0, 0, 1, 1);
	XFreePixmap(display, pm);
	
	return picture;
}


void init(void)
{
	display=XOpenDisplay(0);
	
	int render_event_base, render_error_base;
	int render_present=XRenderQueryExtension(display, &render_event_base, &render_error_base);
	if (!render_present) panic("RENDER extension missing\n");
	
	int screen=DefaultScreen(display);
	Window root=DefaultRootWindow(display);
	Visual *visual=DefaultVisual(display, screen);
	depth=DefaultDepth(display, screen);
	
	fmt=XRenderFindVisualFormat(display, visual);
	
	window=XCreateWindow(display, root, 0, 0, 640, 480, 0,
		depth, InputOutput, visual, 0, NULL);
	background=XCreatePixmap(display, window, 640, 480, depth);
	XSelectInput(display, window, StructureNotifyMask);
	XSetWindowBackgroundPixmap(display, window, background);
	XMapWindow(display, window);
	
	XRenderPictureAttributes pict_attr;
	pict_attr.poly_edge=PolyEdgeSmooth;
	pict_attr.poly_mode=PolyModeImprecise;
	drawingarea=XRenderCreatePicture(display, background, fmt, CPPolyEdge|CPPolyMode,
		&pict_attr);
	
	white_brush=create_brush(0xffff, 0xffff, 0xffff, 0xffff);
	red_brush=create_brush(0xffff, 0x0000, 0x0000, 0xffff);
	blue_brush=create_brush(0x0000, 0x0000, 0xffff, 0xffff);
	
	while(1) {
		XEvent ev;
		XNextEvent(display, &ev);
		if (ev.type==MapNotify) break;
	}
}

int make_circle(int cx, int cy, int radius, int max_ntraps, XTrapezoid traps[])
{
	int n=0, k=0, y1, y2;
	double w;
	while(k<max_ntraps) {
		y1=(int)(-radius*cos(M_PI*k/max_ntraps));
		traps[n].top=(cy+y1)<<16;
		traps[n].left.p1.y=(cy+y1)<<16;
		traps[n].right.p1.y=(cy+y1)<<16;
		w=sqrt(radius*radius-y1*y1)*65536;
		traps[n].left.p1.x=(int)((cx<<16)-w);
		traps[n].right.p1.x=(int)((cx<<16)+w);
		
		do {
			k++;
			y2=(int)(-radius*cos(M_PI*k/max_ntraps));
		} while(y1==y2);
		
		traps[n].bottom=(cy+y2)<<16;
		traps[n].left.p2.y=(cy+y2)<<16;
		traps[n].right.p2.y=(cy+y2)<<16;
		w=sqrt(radius*radius-y2*y2)*65536;
		traps[n].left.p2.x=(int)((cx<<16)-w);
		traps[n].right.p2.x=(int)((cx<<16)+w);
		n++;
	}
	return n;
}

typedef struct {
	int px, py, vx, vy, radius;
	Picture brush;
} Ball;

int main()
{
	init();
	XRenderColor black={0,0,0,0xffff};
	
	Ball balls[3];
	
	int n;
	srand((int) time(0));
	for(n=0; n<3; n++) {
		balls[n].px=rand()%640;
		balls[n].py=rand()%480;
		balls[n].vx=rand()%7-3;
		balls[n].vy=rand()%7-3;
		balls[n].radius=n*3+15;
	}
	balls[0].brush=white_brush;
	balls[1].brush=red_brush;
	balls[2].brush=blue_brush;
	
	while(1) {
		XFlush(display);
		
		XRenderFillRectangle(display, PictOpOver, drawingarea, &black,
			0, 0, 640, 480);
		for(n=0; n<3; n++) {
			XTrapezoid traps[30];
			int ntraps=make_circle(balls[n].px, balls[n].py, balls[n].radius,
				30, traps);
			XRenderCompositeTrapezoids(display, PictOpOver, balls[n].brush,
				drawingarea, 0, 0, 0, traps, ntraps);
			//balls[n].px+=balls[n].vx;
			//balls[n].py+=balls[n].vy;
			if (balls[n].px<balls[n].radius) {balls[n].px=2*balls[n].radius-balls[n].px;balls[n].vx=-balls[n].vx;}
			if (balls[n].px>=640-balls[n].radius) {balls[n].px=1280-2*balls[n].radius-balls[n].px;balls[n].vx=-balls[n].vx;}
			if (balls[n].py<balls[n].radius) {balls[n].py=2*balls[n].radius-balls[n].py;balls[n].vy=-balls[n].vy;}
			if (balls[n].py>=480-balls[n].radius) {balls[n].py=960-2*balls[n].radius-balls[n].py;balls[n].vy=-balls[n].vy;}
		}
		XClearArea(display, window, 0, 0, 640, 480, 0);
		usleep(20*1000);
	}
	
	return 0;
}
