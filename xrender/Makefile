CFLAGS=-Wall

all: xrhello rendertext circles learning1

xrhello rendertext circles learning1:
	$(CC) -o $@ $^ $(LDFLAGS)

xrhello: xrhello.o

rendertext: rendertext.o

circles: circles.o

learning1: learning1.o

rendertext: LDFLAGS+=`pkg-config --libs xrender freetype2`
xrhello circles learning1: LDFLAGS+=`pkg-config --libs xrender` -lm

rendertext.o: CFLAGS+=`pkg-config --cflags xrender freetype2`
xrhello.o circles.o learning1.o: CFLAGS+=`pkg-config --cflags xrender`
