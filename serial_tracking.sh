#!/bin/sh

# -----------------------------------------------------------------------------
# Serial tracking script for pcs control computers
#
# Original Author: Stephen Fegan
# $Author: aune $
# $Date: 2011/03/10 20:40:13 $
# $Revision: 2.30 $
# $Tag$
# -----------------------------------------------------------------------------

# Change to correct directory -- can be relative
mydirname=`dirname $0`
cd $mydirname

# Get absolute directory
mydirname=`/bin/pwd`

# Set the MySQL variables
export VDBHOST=db.vts
export VDBUSER=readwrite
export VDBPW=AmadoScope

my_hostname=$(hostname -f)

# CORBA nameserver
corba_ns="corbaname::db.vts"

# Common options for direct use
direct_opt="-no_corba"

# Common options for CORBA use
corba_opt="-corba_nameserver=${corba_ns}"

# Options for array CORBA GUI
corba_array_opt="$corba_opt -corba_no_auto_stopping -suppress_servo_fail=0 -array"

# Common options for single CORBA GUI
corba_one_opt="$corba_opt -corba_no_auto_stopping -controller=remote"

# Common options for single CORBA server
corba_kill_opt="$corba_opt -terminate"

# Common options for single CORBA server
corba_server_opt="$corba_opt -no_qt" #-daemon"

# Script name
script_name=`basename $0`

# Switch on the name of the script
case $script_name in
serial_tracking|direct_tracking)
	# Get the default options for the program
	case $my_hostname in
	telectl.t1.vts)
		opt="${direct_opt} -scope=0"
		;;
	telectl.t2.vts)
		opt="${direct_opt} -scope=1"
		;;
	telectl.t3.vts)
		opt="${direct_opt} -scope=2"
		;;
	telectl.t4.vts)
		opt="${direct_opt} -scope=3"
		;;
	paw19.sao.arizona.edu)
		opt="${direct_opt} -scope=10"
		;;
	*)
		opt="${direct_opt}"
		;;
	esac
	;;
array_tracking)
	opt="${corba_array_opt}"
	;;
monitor_array)
	opt="${corba_array_opt} -corba_readonly"
	;;
tracking_server)
	# Get the default options for the program
	case $my_hostname in
	telectl.t1.vts)
		opt="${corba_server_opt} -scope=0"
		;;
	telectl.t2.vts)
		opt="${corba_server_opt} -scope=1"
		;;
	telectl.t3.vts)
		opt="${corba_server_opt} -scope=2"
		;;
	telectl.t4.vts)
		opt="${corba_server_opt} -scope=3"
		;;
	*)
		echo $0: CORBA server not configured to run from host $my_hostname
		exit
		;;
	esac
	;;
kill_tracking_server)
	# Get the default options for the program
	case $my_hostname in
	telectl.t1.vts)
		opt="${corba_kill_opt} -scope=0"
		;;
	telectl.t2.vts)
		opt="${corba_kill_opt} -scope=1"
		;;
	telectl.t3.vts)
		opt="${corba_kill_opt} -scope=2"
		;;
	telectl.t4.vts)
		opt="${corba_kill_opt} -scope=3"
		;;
	*)
		echo $0: CORBA server not configured to run from host $my_hostname
		exit
		;;
	esac
	;;
one_tracking_t1)
	opt="${corba_one_opt} -scope=0"
	;;
one_tracking_t2)
	opt="${corba_one_opt} -scope=1"
	;;
one_tracking_t3)
	opt="${corba_one_opt} -scope=2"
	;;
one_tracking_t4)
	opt="${corba_one_opt} -scope=3"
	;;
kill_tracking_server_t1)
	opt="${corba_kill_opt} -scope=0"
	;;
kill_tracking_server_t2)
	opt="${corba_kill_opt} -scope=1"
	;;
kill_tracking_server_t3)
	opt="${corba_kill_opt} -scope=2"
	;;
kill_tracking_server_t4)
	opt="${corba_kill_opt} -scope=3"
	;;
esac

# Execute the program
exec $mydirname/serialTracking/VTracking/serial_tracking ${opt} $* 