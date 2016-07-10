//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUICorrectionDialogs.cpp
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: sfegan $
 * $Date: 2007/07/19 22:30:13 $
 * $Revision: 2.2 $
 * $Tag$
 *
 **/

#include<cassert>

#include<qfiledialog.h>
#include<qinputdialog.h>
#include<qmessagebox.h>
#include<zthread/Guard.h>

#include<VDB/VDBPositioner.h>

#include<Exception.h>
#include<Message.h>
#include<Messenger.h>
#include<Debug.h>
#include<Astro.h>

#include"Global.h"
#include"GUICorrectionDialogs.h"
#include"GUICorrectionDateDialog.h"
#include"text.h"

using namespace SEphem;
using namespace VERITAS;
using namespace VTracking;
using namespace VMessaging;

// ----------------------------------------------------------------------------
// GENERAL
// ----------------------------------------------------------------------------

GUICorrectionDialogs::Status GUICorrectionDialogs::
loadDefault(SEphem::CorrectionParameters& cp, unsigned scope_id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  return loadFromDB(VATime::now(),cp,scope_id);
  //return loadFromFile(getDefaultLoadFileName(scope_id),cp);
}

GUICorrectionDialogs::Status GUICorrectionDialogs::
save(const SEphem::CorrectionParameters& cp, unsigned scope_id,
     QWidget* parent, const char* name)
{
  int what_to_do = 
    QMessageBox::question(parent, name,
			  "Where do you wish to save the corrections?",
			  "&Database", "&File", "Cancel",
			  0, 2);

  switch(what_to_do)
    {
    case 0: // Database clicked or Alt+D pressed or Enter pressed.
      return saveToDB(cp, scope_id, parent, name);
      break;
    case 1: // File clicked or Alt+F pressed
      return saveToFile(cp, scope_id, parent, name);
      break;
    case 2:
      return S_CANCEL;
      break;
    }

  // does not get here
  assert(0);
  return S_FAIL;
}

// ----------------------------------------------------------------------------
// DATABASE
// ----------------------------------------------------------------------------

GUICorrectionDialogs::Status GUICorrectionDialogs::
loadFromDB(SEphem::CorrectionParameters& cp, unsigned scope_id,
	   QWidget* parent, const char* name)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  VATime ref_time = 
    GUICorrectionDateDialog::getCorrectionDate(scope_id, parent, name);
  
  if(!ref_time.isGood())return S_FAIL;
  else if(ref_time == GUICorrectionDateDialog::loadFromFileTime())
    return loadFromFile(cp, scope_id, parent, name);
  else if(ref_time == GUICorrectionDateDialog::cancelLoadTime())
    return S_CANCEL;
  else return loadFromDB(ref_time, cp, scope_id);
}

GUICorrectionDialogs::Status GUICorrectionDialogs::
loadFromDB(const VATime& ref_time,
	   SEphem::CorrectionParameters& cp, unsigned scope_id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  
  try
    {
      ZThread::Guard<ZThread::RecursiveMutex>
	guard(Global::instance()->dbMutex());

      VDBPOS::CorrectionInfo ci =
	VDBPOS::getCorrections(scope_id, ref_time.getDBTimeStamp());

      cp.enable_offsets     = ci.enable_offsets;
      cp.enable_corrections = ci.enable_corrections;
      cp.enable_vff         = ci.enable_velocity_feed_forward;
      cp.az_ratio           = ci.az_gear_ratio;
      cp.el_ratio           = ci.el_gear_ratio;
      cp.az_offset          = ci.az_offset_rad;
      cp.el_offset          = ci.el_offset_rad;
      cp.az_ns              = ci.azimuth_axisNS_rad;
      cp.az_ew              = ci.azimuth_axisEW_rad;
      cp.el_udew            = ci.perpendicularity_rad;
      cp.fp_az              = ci.collimation_rad;
      cp.flex_el_A          = ci.flexure_cosEL_rad;
      cp.flex_el_B          = ci.flexure_sin2EL_rad;
      cp.el_pos_vff_s       = ci.vff_pos_el_slope_sec;
      cp.el_pos_vff_t       = ci.vff_pos_el_threshold_rad_per_sec;
      cp.el_neg_vff_s       = ci.vff_neg_el_slope_sec;
      cp.el_neg_vff_t       = ci.vff_neg_el_threshold_rad_per_sec;
      cp.az_pos_vff_s       = ci.vff_pos_az_slope_sec;
      cp.az_pos_vff_t       = ci.vff_pos_az_threshold_rad_per_sec;
      cp.az_neg_vff_s       = ci.vff_neg_az_slope_sec;
      cp.az_neg_vff_t       = ci.vff_neg_az_threshold_rad_per_sec;
    }
  catch(const VDBException& x)
    {
      Message message(Message::DR_GLOBAL,Message::PS_EXCEPTIONAL,
		      "VDB exception");

      message.messageStream()
	<< "A database exception was thrown and caught while\n"
	<< "loading corrections for scope/date: "
	<< scope_id+1 << '/' << ref_time.toString();
      message.detailsStream() << x;
      Messenger::relay()->sendMessage(message);
      return S_FAIL;
    }

  return S_GOOD;
}

GUICorrectionDialogs::Status GUICorrectionDialogs::
saveToDB(const SEphem::CorrectionParameters& cp, unsigned scope_id,
	 QWidget* parent, const char* name)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  bool ok = false;
  QString comment = QInputDialog::
    getText("Description of corrections", LAB_CORR_DIALOG_SAVE_COMMENT,
	    QLineEdit::Normal, QString::null, &ok, parent, name);
  
  if(ok)return saveToDB(cp,comment,scope_id);
  else return S_CANCEL;
}

GUICorrectionDialogs::Status GUICorrectionDialogs::
saveToDB(const SEphem::CorrectionParameters& cp, const std::string& comment,
	 unsigned scope_id)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  try
    {
      VDBPOS::CorrectionInfo ci;

      ci.enable_offsets                     = cp.enable_offsets;
      ci.enable_corrections 		    = cp.enable_corrections;
      ci.enable_velocity_feed_forward 	    = cp.enable_vff;
      ci.az_gear_ratio 			    = cp.az_ratio;
      ci.el_gear_ratio 			    = cp.el_ratio;
      ci.az_offset_rad 			    = cp.az_offset;
      ci.el_offset_rad 			    = cp.el_offset;
      ci.azimuth_axisNS_rad 		    = cp.az_ns;
      ci.azimuth_axisEW_rad 		    = cp.az_ew;
      ci.perpendicularity_rad 		    = cp.el_udew;
      ci.collimation_rad 		    = cp.fp_az;
      ci.flexure_cosEL_rad 		    = cp.flex_el_A;
      ci.flexure_sin2EL_rad 		    = cp.flex_el_B;
      ci.vff_pos_el_slope_sec 		    = cp.el_pos_vff_s;
      ci.vff_pos_el_threshold_rad_per_sec   = cp.el_pos_vff_t;
      ci.vff_neg_el_slope_sec 		    = cp.el_neg_vff_s;
      ci.vff_neg_el_threshold_rad_per_sec   = cp.el_neg_vff_t;
      ci.vff_pos_az_slope_sec 		    = cp.az_pos_vff_s;
      ci.vff_pos_az_threshold_rad_per_sec   = cp.az_pos_vff_t;
      ci.vff_neg_az_slope_sec 		    = cp.az_neg_vff_s;
      ci.vff_neg_az_threshold_rad_per_sec   = cp.az_neg_vff_t;
      ci.comment                            = comment;

      ZThread::Guard<ZThread::RecursiveMutex>
	guard(Global::instance()->dbMutex());

      VDBPOS::putCorrections(scope_id, ci);
    }
  catch(const VDBException& x)
    {
      Message message(Message::DR_GLOBAL,Message::PS_EXCEPTIONAL,
		      "VDB exception");

      message.messageStream()
	<< "A database exception was thrown and caught while\n"
	<< "saving corrections for scope: " << scope_id+1;
      message.detailsStream() << x;
      Messenger::relay()->sendMessage(message);
      return S_FAIL;
    }

  return S_GOOD;
}

// ----------------------------------------------------------------------------
// FILENAME
// ----------------------------------------------------------------------------

std::string GUICorrectionDialogs::getDefaultLoadFileName(unsigned scope_id)
{
  return SEphem::CorrectionParameters::loadFilename(scope_id);
}

std::string GUICorrectionDialogs::getDefaultSaveFileName(unsigned scope_id)
{
  return SEphem::CorrectionParameters::saveFilename(scope_id);
}

GUICorrectionDialogs::Status GUICorrectionDialogs::
loadFromFile(SEphem::CorrectionParameters& cp, 
	     unsigned scope_id,
	     QWidget* parent, const char* name)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  QString file = 
    QFileDialog::getOpenFileName(getDefaultLoadFileName(scope_id),
				 "Data Files (*.dat);;All Files (*)",
				 parent, name, "Select Corrections File");

  if(file.isEmpty())return S_CANCEL;
  else return loadFromFile(file,cp);
}

GUICorrectionDialogs::Status GUICorrectionDialogs::
loadFromFile(const std::string& file,
	     SEphem::CorrectionParameters& cp)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  return cp.load(file.c_str())?S_GOOD:S_FAIL;
}

GUICorrectionDialogs::Status GUICorrectionDialogs::
saveToFile(const SEphem::CorrectionParameters& cp, 
	   unsigned scope_id,
	   QWidget* parent, const char* name)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  QString file = 
    QFileDialog::getSaveFileName(getDefaultSaveFileName(scope_id),
				 "Data Files (*.dat);;All Files (*)",
				 parent, name, "Select Corrections File");

  if(file.isEmpty())return S_CANCEL;
  else return saveToFile(file,cp);
}


GUICorrectionDialogs::Status GUICorrectionDialogs::
saveToFile(const std::string& file,
	   const SEphem::CorrectionParameters& cp)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  return cp.save(file.c_str())?S_GOOD:S_FAIL;
}
