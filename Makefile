APPLICATION := triangulate
LIBRARY := lib$(APPLICATION).a

ifeq ($(OS),Windows_NT)
    APPLICATION := $(APPLICATION).exe
endif

CCFLAGS := -O2 -Wall

all: $(APPLICATION) $(LIBRARY)

$(APPLICATION): main.o triangulate.o
	g++ -o $@ $^

$(LIBRARY): triangulate.o
	ar rcs $@ $^

%.o: %.c triangulate.h Makefile
	gcc $(CCFLAGS) -c $< -o $@

%.o: %.cpp triangulate.h Makefile
	g++ $(CCFLAGS) -c $< -o $@

.PHONY: clean install uninstall
install: 
	cp $(LIBRARY) /usr/local/lib/
	cp $(APPLICATION) /usr/local/bin/
	cp triangulate.h /usr/local/include/

uninstall:
	rm -f /usr/local/lib/$(LIBRARY)
	rm -f /usr/local/bin/$(APPLICATION)
	rm -f /usr/local/include/triangulate.h

clean:
	rm -f *.o
	rm -f $(LIBRARY)
	rm -f $(APPLICATION)
