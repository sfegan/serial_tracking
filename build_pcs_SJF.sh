#!/bin/bash
# -----------------------------------------------------------------------------
# Script to compile package on pcs.t1.vts et al.
#
# Original Author: Stephen Fegan
# $Author: sfegan $
# $Date: 2008/02/27 21:00:31 $
# $Revision: 2.1 $
# $Tag$
# -----------------------------------------------------------------------------

NOQWT=1 \
QWTLIBNAME=qwt-qt3 QWTINCDIR=/usr/include/qwt-qt3 \
VERITASDIR=/usr/local/veritas \
OMNIEVENTINCDIR=/usr/include/omniEvents \
QTINCDIR=/usr/include/qt3 \
make -j3
