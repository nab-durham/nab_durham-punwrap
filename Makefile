CC = gcc
DEBUG = -g
CFLAGS = -Wall -O $(DEBUG)
UNAME = $(shell uname)
NUMPY_INCLUDE=` python -c "import numpy;print(numpy.get_include())" ` 
PYTHON_FLAGS=` python-config --cflags --ldflags `
ifeq "$(UNAME)" "Darwin"
	CFLAGS += -DDARWIN -bundle -flat_namespace -undefined suppress 
   TARGET='Darwin'
#   NUMPY_INCLUDE="/opt/local/Library/Frameworks/Python.framework/Versions/2.6/lib/python2.6//site-packages/numpy/core/include/"
else
   TARGET='Linux'
   CFLAGS += -fPIC -shared
endif
AR=ar
RANLIB=ranlib

CLEANUPS = $(shell find . -maxdepth 1 -name "*.o")
CLEANUPS += $(shell find . -maxdepth 1 -name "*~")
CLEANALLS = $(CLEANUPS) $(shell find . -maxdepth 1 -name "libunwrap2D.a")
CLEANALLS += $(shell find . -maxdepth 1 -name "*.so")
CLEANALLS += $(shell find . -maxdepth 1 -name "*.pyc")
OBJ=Munther_2D_unwrap.o
SRC2=unwrap_phase.c

all: libunwrap2D.a _punwrap2D.so

_punwrap2D.so: $(OBJ) $(SRC2)
	gcc $(CFLAGS) $(PYTHON_FLAGS)\
       -I$(NUMPY_INCLUDE) -o _punwrap2D.so $(SRC2) $(OBJ)
	python -c "import __init__"

test: _punwrap2D.so
	python test.py

	
$(OBJ): 
	$(CC) $(CFLAGS) -c $*.c

libunwrap2D.a: $(OBJ)
	$(AR) -rvu $@ $(OBJ)
	$(RANLIB) $@

#clean:
#ifneq ($(strip $(CLEANUPS)),)
#	\rm $(CLEANUPS)
#endif

clean:
ifneq ($(strip $(CLEANALLS)),)
	\rm $(CLEANALLS)
endif
