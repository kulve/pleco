# Minimal makefile for minimal netrelay

CC            = gcc
LINK          = gcc
SOURCES       = netrelay.c
OBJECTS       = netrelay.o
TARGET        = netrelay

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o "$@" "$<"

all: $(TARGET)

$(TARGET):  $(OBJECTS)
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJCOMP) $(LIBS)

netrelay.o: netrelay.c
	$(CC) -c $(CFLAGS) $(INCPATH) -o netrelay.o netrelay.c
