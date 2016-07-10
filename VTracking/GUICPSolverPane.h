//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUICPSolverPane.h
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
 * $Date: 2006/07/17 14:25:03 $
 * $Revision: 2.4 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUICPSOLVERPANE_H
#define VTRACKING_GUICPSOLVERPANE_H

#include<vector>

#include<qframe.h>
#include<qtable.h>
#include<qlineedit.h>
#include<qcheckbox.h>
#include<qpushbutton.h>
#include<qcombobox.h>
#include<qmessagebox.h>
#include<qlabel.h>

#include"ASA.h"
#include"TelescopeController.h"

#include "GUITabWidget.h"

using namespace SEphem;
using namespace VTracking;

class BusyDialog: public QMessageBox
{
  Q_OBJECT
public:
  BusyDialog(QWidget * parent = 0, const char * name = 0);
  virtual ~BusyDialog();

public slots:
  void finished();
  void tick();

private:
  int m_frame;
  long m_next_sec;
  long m_next_usec;

  QLabel* m_icon_lab;

  static const long sc_update_interval = 100000; // micro sec
};

class GUICPSolverPane: public QFrame, public GUITabPane
{
  Q_OBJECT
public:
  GUICPSolverPane(TelescopeController* controller, unsigned scope_id, 
		  QWidget* parent = 0, const char* name = 0);
  virtual ~GUICPSolverPane();

  void getCorrectionParameters(CorrectionParameters& cp)
    const;
  void setCorrectionParameters(const 
			       CorrectionParameters& cp);
  

signals:
  void newParametersAvailable();
  
public slots:
  void saveMeasurements();
  void loadMeasurements();
  void exportMeasurementsTable();
  void solve();
  void enableSolver(bool enable);
  void addData(double raw_az, double raw_el, double cor_az, double cor_el);
  void calculateResidual();

  void saveCorrections();
  void loadCorrections();

  void update(const GUIUpdateData& data);

private slots:
  void deleteMeasurement();

private:
  static double costFunction(double *, double *, double *, double *, double *, 
			     ALLOC_INT *, int *, int *, int *, USER_DEFINES *);
  void justAddData(double raw_az, double raw_el, double cor_az, double cor_el);
    
  TelescopeController*     m_controller;
  unsigned                 m_scope_id;

  class Measurement
  {
  public:
    double az_driveangle;
    double el_driveangle;
    double az_real;
    double el_real;
  };

  enum FitMode { FM_RMS, FM_MAX };

  class FitThings
  {
  public:
    std::vector<Measurement>* measurement;
    FitMode mode;
  };

  std::vector<Measurement> m_measurement;

  QTable*                  m_data_table;

  QCheckBox*               m_azoffset_enable_cb;
  QLineEdit*               m_azoffset_range_le;
  QLineEdit*               m_azoffset_fit_le;

  QCheckBox*               m_eloffset_enable_cb;
  QLineEdit*               m_eloffset_range_le;
  QLineEdit*               m_eloffset_fit_le;
  
  QCheckBox*               m_azns_enable_cb;
  QLineEdit*               m_azns_range_le;
  QLineEdit*               m_azns_fit_le;

  QCheckBox*               m_azew_enable_cb;
  QLineEdit*               m_azew_range_le;
  QLineEdit*               m_azew_fit_le;

  QCheckBox*               m_elaz_enable_cb;
  QLineEdit*               m_elaz_range_le;
  QLineEdit*               m_elaz_fit_le;
  
  QCheckBox*               m_fpcol_enable_cb;
  QLineEdit*               m_fpcol_range_le;
  QLineEdit*               m_fpcol_fit_le;

  QCheckBox*               m_azencrat_enable_cb;
  QLineEdit*               m_azencrat_range_le;
  QLineEdit*               m_azencrat_fit_le;

  QCheckBox*               m_elencrat_enable_cb;
  QLineEdit*               m_elencrat_range_le;
  QLineEdit*               m_elencrat_fit_le;

  QCheckBox*               m_flexelA_enable_cb;
  QLineEdit*               m_flexelA_range_le;
  QLineEdit*               m_flexelA_fit_le;

  QCheckBox*               m_flexelB_enable_cb;
  QLineEdit*               m_flexelB_range_le;
  QLineEdit*               m_flexelB_fit_le;

  QLineEdit*               m_residual_rms_le;
  QLineEdit*               m_residual_rms_az_le;
  QLineEdit*               m_residual_rms_el_le;

  QLineEdit*               m_residual_max_le;
  QLineEdit*               m_residual_max_az_le;
  QLineEdit*               m_residual_max_el_le;

  QPushButton*             m_solve_pb;
  QComboBox*               m_solve_for_cb;

  static BusyDialog*       s_busy_dialog;
};

#endif // VTRACKING_GUICPSOLVERPANE_H
