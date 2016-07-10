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

TARGETS = libVMessaging.a libVNETMessaging.a

CXXFLAGS += -I../VOmniORBHelper

all: $(TARGETS)

# ALL IDL FILES
IDLFILE = NET_Message.idl

# ALL HEADERS
HEADERS = Exception.h Debug.h Message.h Messenger.h StreamMessenger.h

NETHEAD = NET_MessageReceiver.h NET_MessageReceiverServant.h \
	  NET_MessageReceiverPolling.h NET_Messenger.h

IDLHEAD = $(IDLFILE:.idl=.h)

MOCHEAD = QtMessenger.h QtDialogMessenger.h QtTextEditMessenger.h

# All SOURCES
SOURCES = $(HEADERS:.h=.cpp)
MOCSRCS = $(MOCHEAD:.h=.cpp) $(MOCHEAD:.h=_moc.cpp)
ALLSRCS = $(SOURCES) $(MOCOBJS)
IDLSRCS = $(IDLHEAD:.h=.cpp) $(IDLHEAD:.h=_dyn.cpp)
NETSRCS = $(NETHEAD:.h=.cpp)

# All OBJECTS
OBJECTS = $(SOURCES:.cpp=.o)
IDLOBJS = $(IDLSRCS:.cpp=.o)
MOCOBJS = $(MOCSRCS:.cpp=.o)
ALLOBJS = $(OBJECTS) $(MOCOBJS)
NETOBJS = $(NETSRCS:.cpp=.o)

CXXFLAGS += -I../VTaskNotification
LDFLAGS += -L../VTaskNotification

libVMessaging.a: $(ALLOBJS)
	$(AR) r $@ $^

libVNETMessaging.a: $(NETOBJS) $(IDLOBJS)
	$(AR) r $@ $^

$(IDLFILE:.idl=.h): %.h: %.idl

$(IDLFILE:.idl=.cpp): %.cpp: %.idl

$(IDLFILE:.idl=_dyn.cpp): %_dyn.cpp: %.idl

$(IDLOBJS): %.o: %.cpp

$(MOCHEAD:.h=.o): %.o: %.cpp

$(MOCHEAD:.h=.o): %.o: %.h

$(HEADERS:.h=.o): %.o: %.h

$(HEADERS:.h=.o): %.o: %.cpp

NET_MessageReceiver.o: NET_Message.h
NET_MessageReceiverServant.o: NET_Message.h NET_MessageReceiver.h
NET_MessageReceiverPoller.o: NET_Message.h
NET_Messenger.o: NET_Message.h

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
                $(IDLSRCS) $(IDLOBJS) \
		core core.* *~ *.o *.moc *_moc.cpp
