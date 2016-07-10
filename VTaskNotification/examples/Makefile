CC  = gcc
CPP = g++
IDL = idl
MOC = $(QTDIR)/bin/moc

CPPFLAGS = -g -I. -I.. -I$(QTDIR)/include
IDLFLAGS = --impl -I/usr/local/idl -I/usr/local/idl/OB
LDFLAGS  = -L$(QTDIR)/lib -L${HOME}/lib -L..

LIBS_CORBA = -lomniDynamic4 -lomniORB4 -lCOSDynamic4 -lCOS4
LIBS_QT    = -lqt
LIBS       = $(LIBS_CORBA) $(LIBS_QT) -lQtOmniORBHelper

all: qttasknotification

qttasknotification: qttasknotification.o
	$(CPP) $(LDFLAGS) -o $@ $^ $(LIBS)

qttasknotification.o: qttasknotification.cpp qttasknotification.moc

qttasknotification.moc: qttasknotification.cpp
	$(MOC) -o $@ $<

clean:
	$(RM) -f qttasknotification core core.* *~ *.o *.moc *_moc.cpp
