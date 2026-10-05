CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99
LDFLAGS = -lm

TARGET  = traitement_image
SRCS    = main.c io_image.c traitement_base.c convolution.c contours.c fourier.c \
          images_binaires.c segmentation.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean clean-resultats distclean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c image.h
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(TARGET)
	./$(TARGET)


clean:
	rm -f $(OBJS) $(TARGET)

clean-resultats:
	rm -rf resultats/*

distclean: clean clean-resultats
