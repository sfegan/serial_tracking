# -----------------------------------------------------------------------------
#
# Serial tracking makefile
#
# Original Author: Stephen Fegan
# $Author: sfegan $
# $Date: 2007/01/26 13:02:13 $
# $Revision: 2.1 $
# $Tag$
#
# -----------------------------------------------------------------------------

include ../Makefile.common

LIBOBJECTS = VSOptions.o VSDataConverter.o VATime.o

BINOBJECTS = 

OBJECTS = $(LIBOBJECTS) $(BINOBJECTS)

TARGETS = libutility.a $(BINOBJECTS:.o=)

all: $(TARGETS)

libutility.a: $(LIBOBJECTS)
	$(AR) r $@ $^

.PHONY: clean

clean:
	$(RM) \
	$(TARGETS) $(OBJECTS) *~

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<
