
FF_SRC = /usr/include/ffmpeg

INSTALL_DIR = /usr/local/bin

INCLUDE = -I$(FF_SRC)/libswscale -I$(FF_SRC)/libavutil -I$(FF_SRC) -I$(FF_SRC)/libavformat -I$(FF_SRC)/libavcodec -I/usr/include/freetype2

LDLIBS = -L$(FF_SRC)/libswscale -L$(FF_SRC)/libavcodec -L$(FF_SRC)/libavformat -L$(FF_SRC)/libavutil -lgd -lswscale -lavformat -lavcodec -lavutil -lpng -ljpeg -lz -lbz2 -lfreetype

CFLAGS = -g -Wall -Wno-deprecated -O2 -pg
# trying extra:  -static-libgcc -Wl,-static -la

C++ = g++

PROG = ovsurgen
OBJS = main.o mediaConfig.o mediaParse.o mediaFrame.o mediaAnalysis.o
HDRS = mediaAnalysis.h  mediaConfig.h  mediaFrame.h  mediaParse.h

all: $(PROG)

$(PROG): $(OBJS)
	$(C++) $(OBJS) $(LDLIBS) -o $(PROG)

%.o: %.cc $(HDRS)
	$(C++) -c $(CFLAGS) $(INCLUDE) $<
%.o: %.cpp $(HDRS)
	$(C++) -c $(CFLAGS) $(INCLUDE) $<

clean:
	rm *.o $(PROG)

install: $(PROG)
	install -m 0755 $(PROG) $(INSTALL_DIR)

.PHONY: install
