# -----------------------------------------------------------------------------
#
# Serial tracking makefile
#
# Original Author: Stephen Fegan
# $Author: aune $
# $Date: 2011/03/10 22:53:48 $
# $Revision: 2.36 $
# $Tag$
#
# -----------------------------------------------------------------------------

include ../Makefile.common

TARGETS = serial_tracking extractfromdb

all: pixmaps $(TARGETS)

VPATH = subsystem_idl

LIBDEPS = ../SEphem/libSEphem.a                                            \
	  ../utility/libutility.a                                          \
	  ../VMessaging/libVNETMessaging.a                                 \
	  ../VTaskNotification/libQtVTaskNotification.a                    \
	  ../VOmniORBHelper/libVOmniORBHelper.a                            \
	  ../VMessaging/libVMessaging.a

LIBS = -L../SEphem -L../utility -L../VMessaging -L../VTaskNotification     \
       -L../VOmniORBHelper -lSEphem -lutility                              \
       -lQtVTaskNotification -lVOmniORBHelper -lVMessaging

CXXFLAGS += -I../SEphem -I../utility -I../VTaskNotification                \
	    -I../VMessaging -I../VOmniORBHelper

# ALL IDL FILES
IDLFILE = NET_TCInterface.idl NET_SerialTracking.idl                       \
	  NET_SerialTracking_data.idl NET_VGRBMonitor.idl                  \
	  NET_VGRBMonitor_data.idl NET_VAstroDBResolver.idl

# ALL HEADERS
HEADERS = ScopeAPI.h EIA422.h DataStream.h TargetObject.h		   \
	  TelescopeControllerLocal.h TelescopeControllerRemote.h           \
	  TelescopeController.h ScopeProtocolServer.h RNG.h ASA.h	   \
	  TelescopeEmulator.h PIUScopeAPI.h	   \
	  MotionController.h GUIPixmaps.h FailsafeUI.h                     \
	  PositionLogger.h TelescopeMotionLimits.h              \
	  SetpointScheduler.h TelescopePhysicsModel.h TextMenu.h           \
	  NET_TCInterfaceServant.h NET_SerialTrackingServant.h             \
	  MainController.h MainEmulator.h MainFailsafe.h MainTerminate.h   \
	  MainArrayGUI.h GUIUpdateData.h Global.h
#	  TenMeter.h TenMeterControlLoop.h

IDLHEAD = $(IDLFILE:.idl=.h)

UI = GUITargetCollectionDialogUI.ui GUITargetCollectionManagerDialogUI.ui  \
     GUITargetAddDialogUI.ui GUIResolvedTargetSelectorUI.ui

MOCHEAD = GUI.h GUIMisc.h GUIPositionerStatusWidget.h GUICPSolverPane.h    \
	  GUIAboutPane.h GUISummaryDetailsPane.h GUITargetTablePane.h      \
	  GUITCPPane.h GUITabWidget.h GUIArray.h                           \
	  GUIMessengerPane.h GUIObjectSelector.h GUISunWarning.h Main.h    \
	  GUIAzElIndicator.h GUIArrayPane.h GUIGRBMonitor.h                \
	  GUITargetCollectionDialog.h GUITargetDialogs.h                   \
	  GUITargetAddDialog.h GUIResolvedTargetSelector.h                 \
	  GUITargetCollectionManagerDialog.h				   \
	  GUICorrectionDateDialog.h GUICorrectionDialogs.h                 \
	  $(UI:.ui=.h)

ifndef NOQWT
MOCHEAD += GUIOscilloscopePane.h
endif

# All SOURCES
SOURCES = $(HEADERS:.h=.cpp)
IDLSRCS = $(IDLHEAD:.h=.cpp) #$(IDLHEAD:.h=_dyn.cpp)
MOCSRCS = $(MOCHEAD:.h=.cpp) $(MOCHEAD:.h=_moc.cpp)
ALLSRCS = $(SOURCES) $(IDLSRCS) $(MOCSRCS)

# All OBJECTS
OBJECTS = $(SOURCES:.cpp=.o)
IDLOBJS = $(IDLSRCS:.cpp=.o)
MOCOBJS = $(MOCSRCS:.cpp=.o)
ALLOBJS = $(OBJECTS) $(IDLOBJS) $(MOCOBJS)

CXXFLAGS += -I../VTaskNotification
LDFLAGS += -L../VTaskNotification -lVDB

static: pixmaps serial_tracking_static serial_tracking_static.bz2

test: TestTenMeter.o DataStream.o ScopeAPI.o ../VMessaging/Exception.o
	$(CXX) $(LDFLAGS) -o $@ $^ -lZThread

brudrive_method_compiler: TenMeter.cpp TenMeter.h DataStream.o             \
			  ../VMessaging/Exception.o ScopeAPI.o
	$(CXX) -DBRUDRIVE_BUILD_MEMBERS $(CXXFLAGS) $(LDFLAGS) -o $@       \
		-L../utility                                               \
		TenMeter.cpp ../VMessaging/Exception.o DataStream.o        \
		ScopeAPI.o ../VMessaging/Debug.o -lutility -lZThread 

.PHONY: pixmaps pixmaps-clean

pixmaps :
	$(MAKE) -C pixmaps

pixmaps-clean:
	$(MAKE) -C pixmaps clean

clean: 
	$(RM) $(ALLOBJS) $(QOBJECTS:.o=_moc.cpp)                           \
		asa_license.h gpl.h todo.h                                 \
		serial_tracking.o serial_tracking serial_tracking_static   \
		serial_tracking_static.bz2 *~ *.bak test test.bz2          \
		extractfromdb extractfromdb.o                              \
		BRUDrive_commands.txt  BRUDrive_members_cpp.txt            \
		BRUDrive_members_hpp.txt BRUDrive_emulator_cpp.txt         \
		brudrive_method_compiler TestTenMeter.o                    \
		moon.h							   \
		$(IDLHEAD) $(IDLSRCS) $(IDLOBJS)                           \
		$(UI:.ui=.h) $(UI:.ui=.cpp)

distclean: clean pixmaps-clean
	$(RM) test

serial_tracking: $(ALLOBJS) serial_tracking.o $(LIBDEPS)
	$(CXX) -o $@ $^ $(LDFLAGS) 

extractfromdb: $(ALLOBJS) extractfromdb.o $(LIBDEPS)
	$(CXX) -o $@ $^ $(LDFLAGS)

serial_tracking_static: $(OBJECTS) serial_tracking.o                       \
			../SEphem/libSEphem.a
	$(CXX) -L../SEphem -L/usr/X11R6/lib -L$(QTDIR)-static/lib          \
		-o $@ $^ ../VMessaging/libVMessaging.a                     \
		./VTaskNotification/libQtVTaskNotification.a               \
		-lqt -lm -lZThread -lpthread -lGLU -lGL -lXmu -lXrender    \
		-lXinerama -lXft -lfreetype -lXext -lX11 -lm -lSM          \
		-lICE -ldl /usr/X11R6/lib/libXrandr.a                      \
		/usr/X11R6/lib/libXcursor.a /usr/local/lib/libqwt.a        \
		$(DBLIB)

serial_tracking_static.bz2: serial_tracking_static
	$(RM) $@
	strip $^
	bzip2 -k $^

asa_license.h: ASA/LICENSE
	perl -pe 'BEGIN{print "static char ASA_LICENSE[]=\"";};s/\n/\\n/g;s/"/\\"/g;END{print "\";\n";}' $< > $@

gpl.h: GPL_V2
	perl -pe 'BEGIN{print "static char GPL[]=\"";};s/\n/\\n/g;s/"/\\"/g;END{print "\";\n";}' $< > $@

todo.h: TODO
	perl -pe 'BEGIN{print "static char TODO[]=\"";};s/\n/\\n/g;s/"/\\"/g;END{print "\";\n";}' $< > $@

TenMeter.o: TenMeter.cpp TenMeter.h BRUDrive_members_hpp.txt               \
	    BRUDrive_members_cpp.txt

TestTenMeter.o: TenMeter.cpp TenMeter.h BRUDrive_members_hpp.txt           \
	        BRUDrive_members_cpp.txt
	$(CXX) -DTEST_BRU $(CXXFLAGS) -c $< -o $@

TenMeterControlLoop.o: TenMeterControlLoop.cpp TenMeterControlLoop.h       \
	               BRUDrive_members_hpp.txt BRUDrive_members_cpp.txt

BRUDrive_members_hpp.txt \
BRUDrive_members_cpp.txt: BRUDrive_commands.txt                            \
	                  make_brudrive_members.pl
	perl -w make_brudrive_members.pl $<

BRUDrive_commands.txt: brudrive_method_compiler
	./brudrive_method_compiler > $@

GUIAboutPane.o: gpl.h asa_license.h todo.h Version.h

NET_SerialTracking.o: NET_SerialTracking.h NET_SerialTracking_data.h

NET_SerialTrackingServant.o: NET_SerialTracking.h NET_SerialTracking_data.h

NET_TCInterfaceServant.o: NET_TCInterface.h

NET_VGRBMonitor.o: NET_VGRBMonitor.h NET_VGRBMonitor_data.h

GUIGRBMonitor.o: NET_VGRBMonitor.h NET_VGRBMonitor_data.h

MainArrayGUI.o: NET_VGRBMonitor.h NET_VGRBMonitor_data.h

GUIArrayPane.o: NET_VGRBMonitor.h NET_VGRBMonitor_data.h

GUIObjectSelector.o: NET_VGRBMonitor.h NET_VGRBMonitor_data.h

GUITargetCollectionDialog.o: GUITargetCollectionDialogUI.h

GUICorrectionDateDialog.o: GUITargetCollectionDialogUI.h

GUITargetCollectionManagerDialog.o: GUITargetCollectionManagerDialogUI.h

GUITargetAddDialog.o: GUITargetAddDialogUI.h GUIResolvedTargetSelectorUI.h

GUIResolvedTargetSelector.o: GUIResolvedTargetSelectorUI.h

GUIArray.o: GUITargetAddDialogUI.h GUITargetCollectionManagerDialogUI.h

FailsafeUI.o: NET_VGRBMonitor.h NET_VGRBMonitor_data.h

MainController.o: NET_TCInterface.h

TelescopeControllerRemote.o: NET_TCInterface.h

GUIArray.o: text.h
GUIArrayPane.o: text.h  
GUI.o: text.h 
GUICPSolverPane.o: text.h 
GUIGRBMonitor.o: text.h 
GUIObjectSelector.o: text.h 
GUIPositionerStatusWidget.o: text.h 
GUISummaryDetailsPane.o: text.h
GUITCPPane.o: text.h
MainArrayGUI.o: text.h
MainController.o: text.h
Main.o: text.h

$(IDLFILE:.idl=.h): %.h: %.idl

$(IDLFILE:.idl=.cpp): %.cpp: %.idl

#$(IDLFILE:.idl=_dyn.cpp): %_dyn.cpp: %.idl

$(IDLOBJS): %.o: %.cpp

$(UI:.ui=.h): %.h: %.ui
	$(UIC) -o $@ $<

$(UI:.ui=.cpp): %.cpp: %.ui
	$(UIC) -o $@ -impl $(<:.ui=.h) $<

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

