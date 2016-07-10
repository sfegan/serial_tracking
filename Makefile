# -----------------------------------------------------------------------------
#
# Serial tracking top level makefile
#
# Original Author: Stephen Fegan
# $Author: aune $
# $Date: 2010/09/14 22:40:10 $
# $Revision: 2.1 $
# $Tag$
#
# -----------------------------------------------------------------------------

# Array Control make command (2005-10-01): LD_LIBRARY_PATH=/home/observer/steering/lib VERITASDIR=/home/observer/steering QTNOMT=1 MYSQLDIR=/usr/local/bin QTDIR=/home/observer/steering/src/libs/qt-x11-free-3.3.4 make

COMPONENTS = utility \
	VTaskNotification \
	SEphem \
	VMessaging \
	VOmniORBHelper 

LIBDEPS = utility/libutility.a \
	VTaskNotification/libVTaskNotification.a \
	SEphem/libSEphem.a \
	VMessaging/libVMessaging.a \
	VOmniORBHelper/libVOmniORBHelper.a 

TARGET = VTracking

ALL = $(COMPONENTS) $(TARGET)

all: $(TARGET)

$(TARGET): $(LIBDEPS)

utility/libutility.a: utility
VTaskNotification/libVTaskNotification.a: VTaskNotification
VOmniORBHelper/libVOmniORBHelper.a: VOmniORBHelper
SEphem/libSEphem.a: SEphem
VMessaging/libVMessaging.a: VMessaging

.PHONY: $(ALL)

$(ALL):
	$(MAKE) -C $@

.PHONY: clean

clean: $(addsuffix -clean,$(ALL))
	$(RM) *~

.PHONY: $(addsuffix -clean,$(ALL))

$(addsuffix -clean,$(ALL)):
	$(MAKE) -C $(@:-clean=) clean

.PHONY: install

install: 
	ln -s ${PWD}/serial_tracking.sh ../array_tracking
	ln -s ${PWD}/serial_tracking.sh ../direct_tracking
	ln -s ${PWD}/serial_tracking.sh ../kill_tracking_server
	ln -s ${PWD}/serial_tracking.sh ../kill_tracking_server_t1
	ln -s ${PWD}/serial_tracking.sh ../kill_tracking_server_t2
	ln -s ${PWD}/serial_tracking.sh ../kill_tracking_server_t3
	ln -s ${PWD}/serial_tracking.sh ../kill_tracking_server_t4
	ln -s ${PWD}/serial_tracking.sh ../monitor_array
	ln -s ${PWD}/serial_tracking.sh ../one_tracking_t1
	ln -s ${PWD}/serial_tracking.sh ../one_tracking_t2
	ln -s ${PWD}/serial_tracking.sh ../one_tracking_t3
	ln -s ${PWD}/serial_tracking.sh ../one_tracking_t4
	ln -s ${PWD}/serial_tracking.sh ../serial_tracking
	ln -s ${PWD}/serial_tracking.sh ../tracking_server
