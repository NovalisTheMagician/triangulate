APPLICATION := triangulate
LIBRARY := lib$(APPLICATION).a

CPPFLAGS := -Iearcut/include

INSTALL_PREFIX := /usr/local

INSTALL_LIB_PATH := $(INSTALL_PREFIX)/lib
INSTALL_INC_PATH := $(INSTALL_PREFIX)/include

ifeq ($(OS),Windows_NT)
    APPLICATION := $(APPLICATION).exe
    CPPFLAGS += -I$(INC_PATH)
    INSTALL_LIB_PATH := $(LIB_GCC_PATH)
    INSTALL_INC_PATH := $(INC_PATH)
endif

CCFLAGS := -O2 -Wall

all: $(LIBRARY)

#$(APPLICATION): main.o triangulate.o
#	g++ -o $@ $^

$(LIBRARY): triangulate.o
	ar rcs $@ $^

%.o: %.c triangulate.h Makefile
	gcc $(CCFLAGS) $(CPPFLAGS) -c $< -o $@

%.o: %.cpp triangulate.h Makefile
	g++ $(CCFLAGS) $(CPPFLAGS) -c $< -o $@

.PHONY: clean install uninstall
install: 
	cp $(LIBRARY) $(INSTALL_LIB_PATH)/
	cp triangulate.h $(INSTALL_INC_PATH)/

uninstall:
	rm -f $(INSTALL_LIB_PATH)/$(LIBRARY)
	rm -f $(INSTALL_INC_PATH)/triangulate.h

clean:
	rm -f *.o
	rm -f $(LIBRARY)
	rm -f $(APPLICATION)
