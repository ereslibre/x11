/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** Copyright (C) 2006-2007 Mirco "MacSlow" Mueller <macslow@gmail.com>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License as
** published by the Free Software Foundation; either version 2 of the
** License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** General Public License for more details.
**
** You should have received a copy of the GNU General Public
** License along with this program; if not, write to the
** Free Software Foundation, Inc., 59 Temple Place - Suite 330,
** Boston, MA 02111-1307, USA.
**
** CAUTION: Running this code on anything but Xgl or anything below Xorg 7.1
** will crash your X11-session!!! This is mainly due to some non-implemented
** things (filters, conical-gradients) in the earlier X11-servers.
**
*******************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/keysymdef.h>
#include <X11/extensions/Xrender.h>

#define WIN_WIDTH 384
#define WIN_HEIGHT 384

typedef enum _GradientToDraw
{
	RADIAL = 0,
	LINEAR,
	CONICAL	
} GradientToDraw;

/* set_picture_transform is heavily based (99%) on code
** from compiz/gnome/window-decorator/gnome-window-decorator.c */
void
set_picture_transform (Display*	pDisplay,
		       Picture	pict,
		       int	iDeltaX,
		       int	iDeltaY)
{
	XTransform transform =	{{
					{ 1 << 16, 0,       -iDeltaX << 16 },
					{ 0,       1 << 16, -iDeltaY << 16 },
					{ 0,       0,         1 << 16 },
				}};

	XRenderSetPictureTransform (pDisplay, pict, &transform);
}

/* create_a8_picture is heavily based (99%) on code
** from cairo/src/cairo-xlib-surface.c */
Picture
create_a8_picture (Display*	 pDisplay,
		   Drawable	 drawable,
		   XRenderColor* pColor,
		   int		 iWidth,
		   int		 iHeight,
		   Bool		 bRepeat)
{
	XRenderPictureAttributes pa;
	unsigned long		 ulMask = 0;
	Picture			 picture;
	Pixmap			 pixmap;

	pixmap = XCreatePixmap (pDisplay,
				drawable,
				iWidth <= 0 ? 1 : iWidth,
				iHeight <= 0 ? 1 : iHeight,
				8);

	if (bRepeat)
	{
		pa.repeat = True;
		ulMask = CPRepeat;
	}

	picture = XRenderCreatePicture (pDisplay,
					pixmap,
					XRenderFindStandardFormat (pDisplay,
								   PictStandardA8),
								   ulMask,
								   &pa);

	XRenderFillRectangle (pDisplay,
			      PictOpSrc,
			      picture,
			      pColor,
			      0,
			      0,
			      iWidth,
			      iHeight);

	XFreePixmap (pDisplay, pixmap);

	return picture;
}

/* create_gaussian_kernel is heavily based (99%) on code
** from compiz/gnome/window-decorator/gnome-window-decorator.c */
XFixed *
create_gaussian_kernel_1d (double fRadius,
			   double fSigma,
			   double fAlpha,
			   double fOpacity,
			   int*	  piSize)
{
	XFixed*	pConvolutionKernel = NULL;
	double*	pfAmp		   = NULL;
	double	fScale		   = 0.0f;
	double	fXScale		   = 0.0f;
	double	fX		   = 0.0f;
	double	fSum		   = 0.0f;
	int	iSize		   = 0;
	int	iHalfSize	   = 0;
	int	iX		   = 0;
	int	i		   = 0;
	int	n		   = 0;

	fScale = 1.0f / (2.0f * M_PI * fSigma * fSigma);
	iHalfSize = (int) (fAlpha + 0.5f);

	if (iHalfSize == 0)
		iHalfSize = 1;

	iSize = iHalfSize * 2 + 1;
	fXScale = 2.0f * fRadius / (double) iSize;

	if (iSize < 3)
		return NULL;

	n = iSize;

	/* allocate memory to hold working-buffer for the convolution-kernel */
	pfAmp = malloc (sizeof (double) * n);
	if (!pfAmp)
		return NULL;

	/* index needs to be two entries larger for storing M- and N-values */
	n += 2;

	/* allocate the buffer for the return-array to pass to the caller */
	pConvolutionKernel = malloc (sizeof (XFixed) * n);
	if (!pConvolutionKernel)
		return NULL;

	i = 0;
	fSum = 0.0f;

	/* fill the working-buffer with the convolution-kernel values */
	for (iX = 0; iX < iSize; iX++)
	{
		fX = fXScale * (iX - iHalfSize);
		pfAmp[i] = fScale *
			   exp ((-1.0f * (fX * fX)) / (2.0f * fSigma * fSigma));
		fSum += pfAmp[i];
		i++;
	}

	/* normalize */
	if (fSum != 0.0)
		fSum = 1.0 / fSum;

	/* reset M and N values describing the MxN kernel */
	pConvolutionKernel[0] = pConvolutionKernel[1] = 0;

	/* fill the "return" array with the convolution-kernel entries */
	for (i = 2; i < n; i++)
		pConvolutionKernel[i] = XDoubleToFixed (pfAmp[i - 2] *
							fSum *
							fOpacity *
							1.2f);

	/* free temporary buffer */
	free (pfAmp);

	/* "pass" array-size to the called */
	*piSize = iSize;

	return pConvolutionKernel;
}

/* this is similar to create_gaussian_kernel_1d() except for the fact that I use
** a naive (or let's call it: brute-force) 2d implementation of a gaussian-blur
** to be able to compare the performance... and the performance non-surprisingly
** sucks... 2 pass 1d gauss. blur is faster than 1 pass 2d gauss. blur */
XFixed*
create_gaussian_kernel_2d (int	iRadius,
			   int*	piSize)
{
	double	fSigma = (double) iRadius / 2.0f;
	double	fScale1;
	double	fScale2;
	double*	pTmpKernel = NULL;
	XFixed*	pKernel = NULL;
	double	fU;
	double	fV;
	int	iX;
	int	iY;
	double	fSum = 0.0f;

	assert (piSize != NULL);

	fScale2 = 2.0f * fSigma * fSigma;
	fScale1 = 1.0f / (M_PI * fScale2);

	*piSize = 4 * iRadius * iRadius;
	pTmpKernel = (double*) calloc (*piSize, sizeof (double));
	if (!pTmpKernel)
		return NULL;

	for (iX = 0; iX < 2 * iRadius; iX++)
	{
		for (iY = 0; iY < 2 * iRadius; iY++)
		{
			fU = (double) iX;
			fV = (double) iY;
			pTmpKernel[iX+iY*2*iRadius] = fScale1 *
						      exp (-(fU*fU+fV*fV) / fScale2);
		}
	}

	for (iX = 0; iX < *piSize; iX++)
		fSum += pTmpKernel[iX];

	for (iX = 0; iX < *piSize; iX++)
		pTmpKernel[iX] /= fSum;

	pKernel = (XFixed*) calloc (*piSize + 2, sizeof (XFixed));
	if (!pKernel)
	{
		free (pTmpKernel);
		return NULL;
	}

	for (iX = 0; iX < *piSize; iX++)
		pKernel[iX + 2] = XDoubleToFixed (pTmpKernel[iX]);

	free (pTmpKernel);

	*piSize += 2;
	pKernel[0] = XDoubleToFixed (2 * iRadius);
	pKernel[1] = XDoubleToFixed (2 * iRadius);

	return pKernel;
}

void
repaint (Display*	pDisplay,
	 Window		window,
	 Visual*	pVisual,
	 int		iWidth,
	 int		iHeight,
	 GradientToDraw	gradient)
{
	XRenderPictFormat*	 pPictFormatDefault = NULL;
	XRenderPictFormat*	 pPictFormatA8 = NULL;
	XRenderPictureAttributes pictAttribs;
	XRenderColor		 colorTrans = {0x0000, 0x0000, 0x0000, 0x0000};
	XRenderColor		 colorWhite = {0xffff, 0xffff, 0xffff, 0xffff};
	Picture			 destPict;
	Picture			 maskPict;
	Picture			 solidPict;
	Picture			 gradientPict;
	XTrapezoid		 trapezoid;
	XLinearGradient		 linearGradient;
	XFixed			 aColorStops[4];
	XRenderColor		 aColorList[4];
	XConicalGradient	 conicalGradient;
	XRadialGradient		 radialGradient;
	XFixed*			 pConvolutionKernel;
	int			 iKernelSize;

	/* get some typical formats */
	pPictFormatDefault = XRenderFindVisualFormat (pDisplay, pVisual);
	pPictFormatA8 = XRenderFindStandardFormat (pDisplay, PictStandardA8);

	/* this is my target- or destination-drawable derived from the window */
	destPict = XRenderCreatePicture (pDisplay,
					 window,
					 pPictFormatDefault,
					 0,
					 &pictAttribs);

	/* two intermediate pictures */
	solidPict = create_a8_picture (pDisplay,
				       window,
				       &colorWhite,
				       iWidth,
				       iHeight,
				       True);
	maskPict = create_a8_picture (pDisplay,
				      window,
				      &colorTrans,
				      iWidth,
				      iHeight,
				      False);

	/* define a as rectangle/square as trapezoid */
	trapezoid.top = XDoubleToFixed (30.0f);
	trapezoid.bottom = XDoubleToFixed ((double) iHeight - 30.0f);
	trapezoid.left.p1.x = XDoubleToFixed (30.0f);
	trapezoid.left.p1.y = XDoubleToFixed (30.0f);
	trapezoid.left.p2.x = XDoubleToFixed (30.0f);
	trapezoid.left.p2.y = XDoubleToFixed ((double) iHeight - 30.0f);
	trapezoid.right.p1.x = XDoubleToFixed ((double) iWidth - 30.0f);
	trapezoid.right.p1.y = XDoubleToFixed (30.0f);
	trapezoid.right.p2.x = XDoubleToFixed ((double) iWidth - 30.0f);
	trapezoid.right.p2.y = XDoubleToFixed ((double) iHeight - 30.0f);

	/* draw a trapezoid in the mask-picture */
	XRenderCompositeTrapezoids (pDisplay,
				    PictOpOver,
				    solidPict,
				    maskPict,
				    pPictFormatA8,
				    0, 0,
				    &trapezoid,
				    1);

	/* coordinates for the start- and end-point of the linear gradient are
	** in  window-space, they are not normalized like in cairo... here using
	** a 10th of the width so it gradient will repeat 10 times sideways if
	** the repeat-attribute is used (see further below) */
	linearGradient.p1.x = XDoubleToFixed (0.0f);
	linearGradient.p1.y = XDoubleToFixed (0.0f);
	linearGradient.p2.x = XDoubleToFixed (0.0f);
	linearGradient.p2.y = XDoubleToFixed ((double) iHeight / 10);

	/* here an example for a conical gradient, the angle is in degrees */
	conicalGradient.center.x = XDoubleToFixed ((double) iWidth / 2);
	conicalGradient.center.y = XDoubleToFixed ((double) iHeight / 2);
	conicalGradient.angle = XDoubleToFixed (123.0f);

	/* and an example for a radial gradient, coordinates are in window-space
	** again */
	radialGradient.inner.x = XDoubleToFixed (50.0f);
	radialGradient.inner.y = XDoubleToFixed (50.0f);
	radialGradient.inner.radius = XDoubleToFixed (45.0f);
	radialGradient.outer.x = XDoubleToFixed (100.0f);
	radialGradient.outer.y = XDoubleToFixed (75.0f);
	radialGradient.outer.radius = XDoubleToFixed (200.0f);

	/* offsets for color-stops have to be stated in normalized form,
	** which means within the range of [0.0, 1.0f] */
	aColorStops[0] = XDoubleToFixed (0.0f);
	aColorStops[1] = XDoubleToFixed (0.33f);
	aColorStops[2] = XDoubleToFixed (0.66f);
	aColorStops[3] = XDoubleToFixed (1.0f);

	/* there's nothing much to say about a XRenderColor,
	** each R/G/B/A-component is an unsigned int (16 bit) */
	aColorList[0].red = 0xffff;
	aColorList[0].green = 0x0000;
	aColorList[0].blue = 0x0000;
	aColorList[0].alpha = 0xffff;
	aColorList[1].red = 0x0000;
	aColorList[1].green = 0xffff;
	aColorList[1].blue = 0x0000;
	aColorList[1].alpha = 0xffff;
	aColorList[2].red = 0x0000;
	aColorList[2].green = 0x0000;
	aColorList[2].blue = 0xffff;
	aColorList[2].alpha = 0xffff;
	aColorList[3].red = 0xffff;
	aColorList[3].green = 0x0000;
	aColorList[3].blue = 0x0000;
	aColorList[3].alpha = 0xffff;

	/* create the actual gradient-filling picture */
	switch (gradient)
	{
		case RADIAL :
			gradientPict = XRenderCreateRadialGradient (pDisplay,
								    &radialGradient,
								    aColorStops,
								    aColorList,
								    4);
		break;

		case LINEAR :
			gradientPict = XRenderCreateLinearGradient (pDisplay,
								    &linearGradient,
								    aColorStops,
								    aColorList,
								    4);
		break;

		case CONICAL :
			gradientPict = XRenderCreateConicalGradient (pDisplay,
								     &conicalGradient,
								     aColorStops,
								     aColorList,
								     4);
		break;

		default :
			gradientPict = XRenderCreateConicalGradient (pDisplay,
								     &conicalGradient,
								     aColorStops,
								     aColorList,
								     4);
		break;
	}

	/* make the gradient in the gradient picture repeat */
	pictAttribs.repeat = True;
	XRenderChangePicture (pDisplay,
			      gradientPict,
			      CPRepeat,
			      &pictAttribs);

	pConvolutionKernel = create_gaussian_kernel_2d (20, &iKernelSize);

	/* "attach" the gaussian-blur to that mask-picture */
	XRenderSetPictureFilter (pDisplay,
				 maskPict,
				 FilterConvolution,
				 pConvolutionKernel,
				 iKernelSize);

	/* render a trapezoid */
	XRenderComposite (pDisplay,
			  PictOpSrc,
			  gradientPict,
			  maskPict,
			  destPict,
			  0, 0,
			  0, 0,
			  0, 0,
			  iWidth, iHeight);

	free (pConvolutionKernel);
	XRenderFreePicture (pDisplay, destPict);
	XRenderFreePicture (pDisplay, solidPict);
	XRenderFreePicture (pDisplay, maskPict);
	XRenderFreePicture (pDisplay, gradientPict);
}

int
main (int    argc,
      char** argv)
{
	Display*	     pDisplay = NULL;
	XEvent		     event;
	Bool		     bKeepGoing = True;
	int		     iEventBase;
	int		     iErrorBase;
	int		     iMajorVersion;
	int		     iMinorVersion;
	XSetWindowAttributes values;
	Window		     window;
	Visual*		     pVisual = NULL;
	int		     iWindowAttribsMask;
	GradientToDraw       iGradient = CONICAL;
	Atom		     property;
	Atom		     type;
	unsigned char	     acWindowTitle[] = "blurred gradients with Xrender";

	/* connect to display */
	pDisplay = XOpenDisplay (NULL);
	if (!pDisplay)
	{
		printf ("Could not open display\n");
		return 1;
	}

	/* check for Xrender-extension */
	if (!XRenderQueryExtension (pDisplay, &iEventBase, &iErrorBase))
	{
		XCloseDisplay (pDisplay);
		printf ("Xrender-extension not found\n");
		return 2;
	}

	/* see which version is available */
	XRenderQueryVersion (pDisplay, &iMajorVersion, &iMinorVersion);
	if (iMajorVersion == 0 && iMinorVersion < 10)
	{
		XCloseDisplay (pDisplay);
		printf ("At least Xrender 0.10 needed\n");
		return 3;
	}
	printf ("Found Xrender %d.%d\n", iMajorVersion, iMinorVersion);

	/* make operations asynchronous */
	XSynchronize (pDisplay, True);

	/* get the default visual for this display */
	pVisual = DefaultVisual (pDisplay, 0);

	/* create and open window */
	values.background_pixel = WhitePixel (pDisplay, 0);
	values.event_mask = KeyPressMask | ExposureMask;
	iWindowAttribsMask = CWEventMask | CWBackPixel;
	window = XCreateWindow (pDisplay,
				RootWindow (pDisplay, 0),
				50, 50, WIN_WIDTH, WIN_HEIGHT, 0,
				DefaultDepth (pDisplay, 0),
				InputOutput,
				pVisual,
				iWindowAttribsMask,
				&values);
	XMapWindow (pDisplay, window);

	/* this is indeed how you change the window-title under pure X11 */
	property = XInternAtom (pDisplay, "_NET_WM_NAME", True);
	type = XInternAtom (pDisplay, "UTF8_STRING", True);
	XChangeProperty (pDisplay,
			 window,
			 property,
			 type,
			 8,
			 PropModeReplace,
			 acWindowTitle,
			 30);

	if (argc == 2)
		iGradient = atoi (argv[1]) - 1;

	XSelectInput (pDisplay, window, KeyPressMask | ExposureMask);

	while (bKeepGoing)
	{
		XNextEvent (pDisplay, &event);
		switch (event.type)
		{
			case KeyPress :
				if (XLookupKeysym (&event.xkey, 0) == XK_Escape)
					bKeepGoing = False;
			break;

			case Expose :
				if (event.xexpose.count < 1)
					repaint (pDisplay,
						 window,
						 pVisual,
						 event.xexpose.width,
						 event.xexpose.height,
						 iGradient);
			break;
		}
	}

	XUnmapWindow (pDisplay, window);
	XDestroyWindow (pDisplay, window);
	XCloseDisplay (pDisplay);

	return 0;
}

