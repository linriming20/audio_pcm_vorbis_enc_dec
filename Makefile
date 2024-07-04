TARGET := pcm_2_ogg_vorbis ogg_vorbis_2_pcm blog_readOggPage

all: $(TARGET)

# our static lib is compile by x86_64 gcc
CC := gcc
CFLAG := -I./include
LDFLAGS :=-lvorbis -lvorbisenc -lvorbisfile -logg -lm -L./lib

pcm_2_ogg_vorbis: main_pcm_2_ogg_vorbis.c
	$(CC) $^ $(CFLAG) $(LDFLAGS) -o $@

ogg_vorbis_2_pcm: main_ogg_vorbis_2_pcm.c
	$(CC) $^ $(CFLAG) $(LDFLAGS) -o $@

blog_readOggPage: blog_readOggPage.c
	$(CC) $^ -o $@

clean :
	rm -rf out* $(TARGET)
.PHONY := clean

