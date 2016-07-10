# -----------------------------------------------------------------------------
#
# Serial tracking makefile
#
# Original Author: Stephen Fegan
# $Author: sfegan $
# $Date: 2006/04/04 17:06:58 $
# $Revision: 2.0 $
# $Tag$
#
# -----------------------------------------------------------------------------

include ../Makefile.common

TARGETS = libVOmniORBHelper.a

all: $(TARGETS)

# ALL IDL FILES
IDLFILE = #CORBAMessage.idl

# ALL HEADERS
HEADERS = VOmniORBHelper.h
IDLHEAD = $(IDLFILE:.idl=.h)
MOCHEAD = 

# All SOURCES
SOURCES = $(HEADERS:.h=.cpp)
IDLSRCS = $(IDLHEAD:.h=.cpp) $(IDLHEAD:.h=_dyn.cpp)
MOCSRCS = $(MOCHEAD:.h=.cpp) $(MOCHEAD:.h=_moc.cpp)
ALLSRCS = $(SOURCES) $(IDLSRCS) $(MOCOBJS)

# All OBJECTS
OBJECTS = $(SOURCES:.cpp=.o)
IDLOBJS = $(IDLSRCS:.cpp=.o)
MOCOBJS = $(MOCSRCS:.cpp=.o)
ALLOBJS = $(OBJECTS) $(IDLOBJS) $(MOCOBJS)

CXXFLAGS += 
LDFLAGS += 

libVOmniORBHelper.a: $(ALLOBJS)
	$(AR) r $@ $^

$(IDLFILE:.idl=.h): %.h: %.idl

$(IDLFILE:.idl=_skel.h): %_skel.h: %.idl

$(IDLFILE:.idl=.cpp): %.cpp: %.idl

$(IDLFILE:.idl=_skel.cpp): %_skel.cpp: %.idl

$(IDLOBJS): %.o: %.cpp

$(MOCHEAD:.h=.o): %.o: %.cpp

$(MOCHEAD:.h=.o): %.o: %.h

$(HEADERS:.h=.o): %.o: %.h

$(HEADERS:.h=.o): %.o: %.cpp

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

%_moc.cpp: %.h
	$(MOC) -o $@ $<

%.h: %.idl
	$(IDL) $(IDLFLAGS) $<

%.cpp: %.idl
	$(IDL) $(IDLFLAGS) $<

clean:
	$(RM) -f $(TARGETS) \
		$(IDLHEAD) $(IDLSRCS) \
		$(IDLFILE:.idl=_impl.h) $(IDLFILE:.idl=_impl.cpp) \
		core core.* *~ *.o *.moc *_moc.cpp

