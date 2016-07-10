#!/bin/bash
# -----------------------------------------------------------------------------
# Example script to start CORBA name and event services
#
# Original Author: Stephen Fegan
# $Author: sfegan $
# $Date: 2006/04/04 17:06:57 $
# $Revision: 2.0 $
# $Tag$
# -----------------------------------------------------------------------------

rm -f /tmp/omninames*
omniNames -start -logdir /tmp &
sleep 2
rm /tmp/omnievents*
/usr/local/sbin/omniEvents -l /tmp -ORBInitRef NameService=corbaname::localhost
sleep 2
eventc -ORBInitRef NameService=corbaloc::localhost/NameService -n SerialTrackingEventChannel -v
nameclt -ior corbaname::localhost bind_new_context VERITAS
nameclt -ior corbaname::localhost bind_new_context VERITAS/serial_tracking.Program
nameclt -ior corbaname::localhost bind_new_context VERITAS/serial_tracking0.Program
nameclt -ior corbaname::localhost bind_new_context VERITAS/serial_tracking1.Program
nameclt -ior corbaname::localhost bind_new_context VERITAS/serial_tracking2.Program
nameclt -ior corbaname::localhost bind_new_context VERITAS/serial_tracking3.Program
nameclt -ior corbaname::localhost bind VERITAS/serial_tracking.Program/EventChannel.Object `nameclt -ior corbaname::localhost resolve SerialTrackingEventChannel`
nameclt -ior corbaname::localhost bind VERITAS/serial_tracking0.Program/EventChannel.Object `nameclt -ior corbaname::localhost resolve SerialTrackingEventChannel`
nameclt -ior corbaname::localhost bind VERITAS/serial_tracking1.Program/EventChannel.Object `nameclt -ior corbaname::localhost resolve SerialTrackingEventChannel`
nameclt -ior corbaname::localhost bind VERITAS/serial_tracking2.Program/EventChannel.Object `nameclt -ior corbaname::localhost resolve SerialTrackingEventChannel`
nameclt -ior corbaname::localhost bind VERITAS/serial_tracking3.Program/EventChannel.Object `nameclt -ior corbaname::localhost resolve SerialTrackingEventChannel`

