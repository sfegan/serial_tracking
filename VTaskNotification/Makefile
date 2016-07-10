# -----------------------------------------------------------------------------
#
# Serial tracking makefile
#
# Original Author: Stephen Fegan
# $Author: sfegan $
# $Date: 2006/04/10 18:01:11 $
# $Revision: 2.1 $
# $Tag$
#
# -----------------------------------------------------------------------------

include ../Makefile.common

TARGETS = libVTaskNotification.a libQtVTaskNotification.a

all: $(TARGETS)

# All HEADERS that are written by me
HEADERS = Task.h Notification.h TaskNotification.h PhaseLockedLoop.h Daemon.h
MOCHEAD = QtGUIThreadNotifier.h \
	  QtNotification.h QtTaskNotification.h
ALLHEAD = $(HEADERS) $(MOCHEAD)

# All sources
SOURCES = $(HEADERS:.h=.cpp)
MOCSRCS = $(MOCHEAD:.h=.cpp) $(MOCHEAD:.h=_moc.cpp)
ALLSRCS = $(SOURCES) $(MOCSRCS)

# All objects
OBJECTS = $(SOURCES:.cpp=.o)
MOCOBJS = $(MOCSRCS:.cpp=.o)
ALLOBJS = $(OBJECTS) $(MOCOBJS)

CXXFLAGS += -I../VMessaging
LDFLAGS += -L../VMessaging

libVTaskNotification.a: $(HEADERS:.h=.o)
	$(AR) r $@ $^

libQtVTaskNotification.a: $(ALLOBJS)
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
