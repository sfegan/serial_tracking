//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUICPSolverPane.cpp
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: aune $
 * $Date: 2010/09/09 18:56:11 $
 * $Revision: 2.10 $
 * $Tag$
 *
 **/

#include<iostream>
#include<iomanip>
#include<fstream>
#include<sstream>
#include<limits>

#include<sys/time.h>

#include<qlayout.h>
#include<qhbox.h>
#include<qlineedit.h>
#include<qtable.h>
#include<qpushbutton.h>
#include<qgroupbox.h>
#include<qstring.h>
#include<qstringlist.h>
#include<qlabel.h>
#include<qvalidator.h>
#include<qfiledialog.h>
#include<qmessagebox.h>
#include<qimage.h>
#include<qpixmap.h>
#include<qtooltip.h>
#include<qwidget.h>
#include<qapplication.h>

#include<Angle.h>
#include<SphericalCoords.h>
#include<Debug.h>

#include"text.h"
#include"GUIMisc.h"
#include"GUICPSolverPane.h"
#include"GUICorrectionDialogs.h"
#include"GUIPixmaps.h"

#include"ASA.h"

using namespace SEphem;
using namespace VMessaging;

// ============================================================================
// BusyDialog
// ============================================================================

BusyDialog::BusyDialog(QWidget* parent,const char* name):
  QMessageBox("Busy","Minimization in progress, please wait...",
	      QMessageBox::Information,QMessageBox::Cancel,
	      QMessageBox::NoButton,QMessageBox::NoButton,parent,name), 
  m_frame(0), m_next_sec(0), m_next_usec(0), m_icon_lab()
{
  show();
  tick();
}

BusyDialog::~BusyDialog()
{
  hide();
}

void BusyDialog::tick()
{
  struct timeval tv;
  gettimeofday(&tv,0);
  if((tv.tv_sec>m_next_sec)||
     ((tv.tv_sec==m_next_sec)&&(tv.tv_usec>=m_next_usec)))
    {
      m_next_sec=tv.tv_sec;
      m_next_usec=tv.tv_usec+sc_update_interval;
      if(m_next_usec>1000000)
	{
	  m_next_sec+=m_next_usec/1000000;
	  m_next_usec=m_next_usec%1000000;
	}
      setIconPixmap(*GUIPixmaps::instance()->sw_pixmaps(m_frame++));
      
      qApp->processEvents(1);
      QApplication::flush();
    }
}

void BusyDialog::finished()
{
  hide();
  delete this;
}

// ============================================================================
// GUICPSolverPane
// ============================================================================

GUICPSolverPane::GUICPSolverPane(TelescopeController* controller,
				 unsigned scope_id,
				 QWidget* parent, const char* name):
  QFrame(parent,name), GUITabPane(this),
  m_controller(controller), m_scope_id(scope_id),
  m_measurement(), m_data_table(),
  m_azoffset_enable_cb(), m_azoffset_range_le(), m_azoffset_fit_le(),
  m_eloffset_enable_cb(), m_eloffset_range_le(), m_eloffset_fit_le(),
  m_azns_enable_cb(), m_azns_range_le(), m_azns_fit_le(),
  m_azew_enable_cb(), m_azew_range_le(), m_azew_fit_le(),
  m_elaz_enable_cb(), m_elaz_range_le(), m_elaz_fit_le(),
  m_fpcol_enable_cb(), m_fpcol_range_le(), m_fpcol_fit_le(),
  m_azencrat_enable_cb(), m_azencrat_range_le(), m_azencrat_fit_le(),
  m_elencrat_enable_cb(), m_elencrat_range_le(), m_elencrat_fit_le(),
  m_flexelA_enable_cb(), m_flexelA_range_le(), m_flexelA_fit_le(),
  m_flexelB_enable_cb(), m_flexelB_range_le(), m_flexelB_fit_le(),
  m_residual_rms_le(), m_residual_rms_az_le(), m_residual_rms_el_le(),
  m_residual_max_le(), m_residual_max_az_le(), m_residual_max_el_le(),
  m_solve_pb(), m_solve_for_cb()
{
  QGridLayout* layout = new QGridLayout(this, 3, 2, 5, 5,
					QString(name)+QString(" layout"));
    
  QGroupBox* table_gb = new MyQGroupBox(1,Qt::Horizontal,"Measurements",this,
					QString(name)+QString(" object gb"));

  m_data_table = new MyQTable(table_gb,QString(name)+QString(" object table"));
  m_data_table->setNumCols(9);
  m_data_table->setSelectionMode(QTable::SingleRow);
  m_data_table->setReadOnly(true);
  m_data_table->setShowGrid(false);

  int colwidths[] = { 65, 65, 65, 65, 65, 65, 65, 65, -1 };
  for(unsigned i=0; i<sizeof(colwidths)/sizeof(*colwidths); i++)
    {
      if(colwidths[i]<0)m_data_table->setColumnStretchable(i,true);
      else m_data_table->setColumnWidth(i,colwidths[i]);
    }
  
  QStringList tablelabels;
  tablelabels.append("Drive Az");
  tablelabels.append("Drive El");
  tablelabels.append("Real Az");
  tablelabels.append("Real El");
  tablelabels.append("Corr Az");
  tablelabels.append("Corr El");
  tablelabels.append("Delta Az");
  tablelabels.append("Delta El");
  tablelabels.append("Residual");
  m_data_table->setColumnLabels(tablelabels);

  QHBox* tablepb_hb = 
    new QHBox(table_gb,QString(name)+QString(" object pb hb"));
    
  QPushButton* delete_pb = 
    new QPushButton("Delete",tablepb_hb,
		    QString(name)+QString(" object delete"));
  QToolTip::add(delete_pb,TT_SOLVER_DELETE);

  QPushButton* save_pb = 
    new QPushButton("Save",tablepb_hb,
		    QString(name)+QString(" object save"));
  QToolTip::add(save_pb,TT_SOLVER_SAVE);

  QPushButton* load_pb = 
    new QPushButton("Load",tablepb_hb,
		    QString(name)+QString(" object load"));
  QToolTip::add(load_pb,TT_SOLVER_LOAD);

  QPushButton* export_pb = 
    new QPushButton("Export Table",tablepb_hb,
		    QString(name)+QString(" object export"));
  QToolTip::add(export_pb,TT_SOLVER_EXPORT);

  connect(delete_pb,SIGNAL(clicked()),this,SLOT(deleteMeasurement()));
  connect(save_pb,SIGNAL(clicked()),this,SLOT(saveMeasurements()));
  connect(load_pb,SIGNAL(clicked()),this,SLOT(loadMeasurements()));
  connect(export_pb,SIGNAL(clicked()),this,SLOT(exportMeasurementsTable()));

  // CP range and value
  
  struct cp_element
  {
    QString label;
    QString tooltip;
    QString tooltip2;
    QCheckBox** enable;
    QLineEdit** range;
    QLineEdit** value;
    double range_init;
    double value_init;
  };

  struct cp_element cp_elements[] = 
    {
      { "AzOff", TT_SOLVER_AZ_OFF, MAKEDEG("0.0"),
	&m_azoffset_enable_cb, &m_azoffset_range_le,
	&m_azoffset_fit_le, 18.0, 0 },
      { "ElOff", TT_SOLVER_EL_OFF, MAKEDEG("0.0"),
	&m_eloffset_enable_cb, &m_eloffset_range_le,
	&m_eloffset_fit_le, 2.0, 0 },
      { "AzNS", TT_SOLVER_AZ_NS, MAKEDEG("0.0"),
	&m_azns_enable_cb, &m_azns_range_le,
	&m_azns_fit_le, 0.5, 0 },
      { "AzEW", TT_SOLVER_AZ_EW, MAKEDEG("0.0"),
	&m_azew_enable_cb, &m_azew_range_le,
	&m_azew_fit_le, 0.5, 0 },
      { "ElAz", TT_SOLVER_EL_AZ, MAKEDEG("0.0"),
	&m_elaz_enable_cb, &m_elaz_range_le,
	&m_elaz_fit_le, 0.2, 0 },
      { "FPCol", TT_SOLVER_FP_COLL, MAKEDEG("0.0"),
	&m_fpcol_enable_cb, &m_fpcol_range_le,
	&m_fpcol_fit_le, 2.0, 0 },
      { "ElFlexA", TT_SOLVER_EL_FLEX_A,	MAKEDEG("0.0"),
	&m_flexelA_enable_cb, &m_flexelA_range_le,
	&m_flexelA_fit_le, 0.2, 0 },
      { "ElFlexB", TT_SOLVER_EL_FLEX_B,	MAKEDEG("0.0"),
	&m_flexelB_enable_cb, &m_flexelB_range_le,
	&m_flexelB_fit_le, 0.2, 0 },
      { "AzEnc", TT_SOLVER_AZ_ENC, "1.0",
	&m_azencrat_enable_cb, &m_azencrat_range_le,
	&m_azencrat_fit_le, 0.001, 1.0 },
      { "ElEnc", TT_SOLVER_EL_ENC, "1.0",
	&m_elencrat_enable_cb, &m_elencrat_range_le,
	&m_elencrat_fit_le, 0.001, 1.0 }
    };

  QGroupBox* search_gb = new MyQGroupBox(6,Qt::Horizontal,
					 "Search Parameters",this,
					 QString(name)+QString(" search gb"));

  new QLabel("Enable", search_gb, 
	     QString(name)+QString(" search enable label"));
  new QLabel("Range", search_gb, 
	     QString(name)+QString(" search range label"));
  new QLabel("Value", search_gb, 
	     QString(name)+QString(" search value label"));
  new QLabel("Enable", search_gb, 
	     QString(name)+QString(" search enable label"));
  new QLabel("Range", search_gb, 
	     QString(name)+QString(" search range label"));
  new QLabel("Value", search_gb, 
	     QString(name)+QString(" search value label"));
  
  for(unsigned i=0; i<sizeof(cp_elements)/sizeof(*cp_elements); i++)
    {
      struct cp_element* el = &cp_elements[i];
      *el->enable =
	new QCheckBox(el->label,search_gb,
		      QString(name)+QString(" search enable ")+el->label);
      if(el->tooltip!="")QToolTip::add(*el->enable,el->tooltip);

      *el->range =
	new QLineEdit(search_gb,
		      QString(name)+QString(" search range le  ")+el->label);

      if(el->tooltip2!="")
	QToolTip::add(*el->range,QString("Search from ")+
		      el->tooltip2+QString("-Range to ")+
		      el->tooltip2+QString("+Range"));

      *el->value =
	new QLineEdit(search_gb,
		      QString(name)+QString(" search value le  ")+el->label);

      (*el->range)->setText(QString::number(el->range_init));
      (*el->value)->setText(QString::number(el->value_init));

      (*el->range)->setValidator(new QDoubleValidator(*el->range,
		     QString(name)+QString(" search range val  ")+el->label));
      (*el->value)->setValidator(new QDoubleValidator(*el->range,
		     QString(name)+QString(" search value val  ")+el->label));

      (*el->range)->setMinimumWidth(55);
      (*el->value)->setMinimumWidth(65);
			
      (*el->enable)->setChecked(false);
      (*el->range)->setEnabled(false);
#if 0
      (*el->value)->setEnabled(false);
#endif

      connect(*el->enable,SIGNAL(toggled(bool)),
	      *el->range,SLOT(setEnabled(bool)));
#if 0
      connect(*el->enable,SIGNAL(toggled(bool)),
	      *el->value,SLOT(setEnabled(bool)));
#endif

      connect(*el->value,SIGNAL(returnPressed()),
	      this,SLOT(calculateResidual()));
      connect(*el->enable,SIGNAL(toggled(bool)),
	      this,SLOT(calculateResidual()));
    }

  // CORRECTIONS
  QGroupBox* cor_gb = new MyQGroupBox(2,Qt::Horizontal,"Corrections",this,
  				      QString(name)+QString(" residual gb"));

  QPushButton* cor_load_pb = 
    new QPushButton("Load",cor_gb,
		    QString(name)+QString(" corrections load pb"));
  connect(cor_load_pb,SIGNAL(clicked()),this,SLOT(loadCorrections()));
  QToolTip::add(cor_load_pb, TT_SOLVER_CP_LOAD);

  QPushButton* cor_save_pb = 
    new QPushButton("Save",cor_gb,
		    QString(name)+QString(" corrections save pb"));
  connect(cor_save_pb,SIGNAL(clicked()),this,SLOT(saveCorrections()));
  QToolTip::add(cor_save_pb,TT_SOLVER_CP_SAVE);

  // RESIDUAL
  QGroupBox* res_gb_outer = new MyQGroupBox(1,Qt::Vertical,"Residual",this,
					QString(name)+QString(" residual gb"));

  QFrame* res_gb = new QFrame(res_gb_outer, 
			      QString(name)+QString(" residual inner frame"));

  QGridLayout* res_layout = 
    new QGridLayout(res_gb,5,3,0,5,
		    QString(name)+QString(" residual gb layout"));

  QLabel* res_rms_lab =
    new QLabel("RMS",res_gb,QString(name)+QString(" residual rms label"));
  QLabel* res_max_lab =
    new QLabel("Max",res_gb,QString(name)+QString(" residual max label"));

  QLabel* res_az_lab = 
    new QLabel("Az",res_gb,QString(name)+QString(" residual az label"));
  m_residual_rms_az_le = 
    new InfoQLineEdit(res_gb,QString(name)+QString(" residual az le"));
  QToolTip::add(m_residual_rms_az_le, TT_SOLVER_AZ_RMS);
  m_residual_max_az_le = 
    new InfoQLineEdit(res_gb,QString(name)+QString(" residual az le"));
  QToolTip::add(m_residual_max_az_le, TT_SOLVER_AZ_MAX);

  QLabel* res_el_lab = 
    new QLabel("El",res_gb,QString(name)+QString(" residual el label"));
  m_residual_rms_el_le = 
    new InfoQLineEdit(res_gb,QString(name)+QString(" residual el le"));
  QToolTip::add(m_residual_rms_el_le, TT_SOLVER_AZ_MAX);
  m_residual_max_el_le = 
    new InfoQLineEdit(res_gb,QString(name)+QString(" residual el le"));
  QToolTip::add(m_residual_max_el_le, TT_SOLVER_EL_MAX);

  QLabel* res_lab = 
    new QLabel("Total",res_gb,QString(name)+QString(" residual label"));
  m_residual_rms_le = 
    new InfoQLineEdit(res_gb,QString(name)+QString(" residual le"));
  QToolTip::add(m_residual_rms_le, TT_SOLVER_RMS);
  m_residual_max_le = 
    new InfoQLineEdit(res_gb,QString(name)+QString(" residual le"));
  QToolTip::add(m_residual_max_le, TT_SOLVER_MAX);

  m_residual_rms_az_le->setMinimumWidth(65);
  m_residual_max_az_le->setMinimumWidth(65);
  m_residual_rms_el_le->setMinimumWidth(65);
  m_residual_max_el_le->setMinimumWidth(65);
  m_residual_rms_le->setMinimumWidth(65);
  m_residual_max_le->setMinimumWidth(65);

  m_solve_pb = new QPushButton("Minimize",res_gb,
			       QString(name)+QString(" solve pb"));
  QToolTip::add(m_solve_pb, TT_SOLVER_MINIMIZE);
  connect(m_solve_pb,SIGNAL(clicked()),this,SLOT(solve()));

  m_solve_for_cb = new QComboBox(res_gb,
				 QString(name)+QString(" solve for cb"));
  QToolTip::add(m_solve_for_cb, TT_SOLVER_MIN_CHOOSE);
  m_solve_for_cb->insertItem("RMS");
  m_solve_for_cb->insertItem("Max");

  res_layout->addWidget(res_rms_lab,0,1);
  res_layout->addWidget(res_max_lab,0,2);
  res_layout->addWidget(res_az_lab,1,0);
  res_layout->addWidget(m_residual_rms_az_le,1,1);
  res_layout->addWidget(m_residual_max_az_le,1,2);
  res_layout->addWidget(res_el_lab,2,0);
  res_layout->addWidget(m_residual_rms_el_le,2,1);
  res_layout->addWidget(m_residual_max_el_le,2,2);
  res_layout->addWidget(res_lab,3,0);
  res_layout->addWidget(m_residual_rms_le,3,1);
  res_layout->addWidget(m_residual_max_le,3,2);
  res_layout->addMultiCellWidget(m_solve_pb,4,4,0,1);
  res_layout->addWidget(m_solve_for_cb,4,2);

  // LAYOUT
  layout->setColStretch(0,10);
  layout->setColStretch(1,1);

  res_gb_outer->setFixedWidth(200);

  layout->addMultiCellWidget(table_gb,0,0,0,1);
  layout->addMultiCellWidget(search_gb,1,2,0,0);
  //  layout->addMultiCellWidget(vals_gb,1,2,1,1);
  layout->addWidget(cor_gb,1,1);
  layout->addWidget(res_gb_outer,2,1);
}

GUICPSolverPane::~GUICPSolverPane()
{
  // nothing to see here
}

void GUICPSolverPane::saveMeasurements()
{
  QString fn = 
    QFileDialog::getSaveFileName("measurements.dat", 
				 "Data Files (*.dat);;All Files (*)",
				 this, "save dialog", 
				 "Select Measurements File");
  if (!fn.isEmpty())
    {
      std::ofstream stream(fn.ascii());
      if(!stream)
	{
	  QMessageBox::warning(0,"Save Measurements",
			       QString("Could not save to file ")+fn,
			       QMessageBox::Ok,QMessageBox::NoButton);
	  return;
	}
      
      for(unsigned i=0;i<m_measurement.size();i++)
	stream << std::fixed << std::showpos
	       << std::setprecision(4) 
	       << Angle::toDeg(m_measurement[i].az_driveangle) << ' ' 
	       << std::setprecision(4) 
	       << Angle::toDeg(m_measurement[i].el_driveangle) << ' ' 
	       << std::setprecision(4)
	       << Angle::toDeg(m_measurement[i].az_real) << ' ' 
	       << std::setprecision(4)
	       << Angle::toDeg(m_measurement[i].el_real) << std::endl;
    }
}

void GUICPSolverPane::loadMeasurements()
{
  QString fn = 
    QFileDialog::getOpenFileName("measurements.dat", 
				 "Data Files (*.dat);;All Files (*)",
				 this, "load dialog", 
				 "Select Measurements File");
  if (!fn.isEmpty())
    {
      std::ifstream stream(fn.ascii());
      if(!stream)
	{
	  QMessageBox::warning(0,"Load Measurements",
			       QString("Could not open file ")+fn,
			       QMessageBox::Ok,QMessageBox::NoButton);
	  return;
	}

      m_measurement.clear();
      m_data_table->setNumRows(0);

      while(stream)
	{
	  Measurement m;
	  if((stream >> m.az_driveangle) &&
	     (stream >> m.el_driveangle) &&
	     (stream >> m.az_real) &&
	     (stream >> m.el_real))
	    justAddData(Angle::frDeg(m.az_driveangle), 
			Angle::frDeg(m.el_driveangle), 
			Angle::frDeg(m.az_real), Angle::frDeg(m.el_real));
	}
      
      calculateResidual();
    }
}

void GUICPSolverPane::exportMeasurementsTable()
{
  QString fn = 
    QFileDialog::getSaveFileName("measurements_export.tab", 
			  "Tables (*.tab);;Text Files (*.txt);;All Files (*)", 
				 this, "export dialog", "Select File");
  if (!fn.isEmpty())
    {
      std::ofstream stream(fn.ascii());
      if(!stream)
	{
	  QMessageBox::warning(0,"Save Measurements",
			       QString("Could not save to file ")+fn,
			       QMessageBox::Ok,QMessageBox::NoButton);
	  return;
	}
      for(int i=0;i<m_data_table->numRows();i++)
	{
	  for(int j=0;j<m_data_table->numCols();j++)
	    {
	      if(j!=0)stream << ' ';
	      stream << std::setprecision(10) << m_data_table->text(i,j);
	    }
	  stream << std::endl;
	}
    }
}

static void setParm(double &min, double& max, double& iv, const double& zv,
		    const QCheckBox* enable_cb,
		    const QLineEdit* range_le, const QLineEdit* iv_le,
		    bool convertRadians)
{
  double range=0;
  bool ok;
  range=range_le->text().toDouble(&ok);
  if((!ok)||(!enable_cb->isOn()))range=0;
  min = zv-range;
  max = zv+range;
  iv=iv_le->text().toDouble(&ok);
  if(!ok)iv=zv;
  if(iv<zv)iv=zv;
  else if(iv>zv)iv=zv;

  if(convertRadians)
    iv=SEphem::Angle::frDeg(iv),
      min=SEphem::Angle::frDeg(min),
      max=SEphem::Angle::frDeg(max);
}

static void unsetParm(const double& val,
		      const QCheckBox* enable_cb, QLineEdit* iv_le,
		      bool convertRadians)
{
  //  if(enable_cb->isOn()) -- NO... otherwise the fit looks silly
  if(convertRadians)
    iv_le->setText(QString::number(SEphem::Angle::toDeg(val)));
  else iv_le->setText(QString::number(val));
}

void GUICPSolverPane::solve()
{
  long nparm = 10;
  double* parm_min = new double[nparm];
  double* parm_max = new double[nparm];
  double* parm_val = new double[nparm];

  CorrectionParameters tcp;

  setParm(parm_min[0],parm_max[0],parm_val[0],tcp.az_offset,
	  m_azoffset_enable_cb, m_azoffset_range_le, m_azoffset_fit_le, true);

  setParm(parm_min[1],parm_max[1],parm_val[1],tcp.el_offset,
	  m_eloffset_enable_cb, m_eloffset_range_le, m_eloffset_fit_le, true);

  setParm(parm_min[2],parm_max[2],parm_val[2],tcp.az_ns,
	  m_azns_enable_cb, m_azns_range_le, m_azns_fit_le, true);

  setParm(parm_min[3],parm_max[3],parm_val[3],tcp.az_ew,
	  m_azew_enable_cb, m_azew_range_le, m_azew_fit_le, true);

  setParm(parm_min[4],parm_max[4],parm_val[4],tcp.el_udew,
	  m_elaz_enable_cb, m_elaz_range_le, m_elaz_fit_le, true);

  setParm(parm_min[5],parm_max[5],parm_val[5],tcp.fp_az,
	  m_fpcol_enable_cb, m_fpcol_range_le, m_fpcol_fit_le, true);

  setParm(parm_min[6],parm_max[6],parm_val[6],tcp.az_ratio,
	  m_azencrat_enable_cb, m_azencrat_range_le, m_azencrat_fit_le, false);

  setParm(parm_min[7],parm_max[7],parm_val[7],tcp.el_ratio,
	  m_elencrat_enable_cb, m_elencrat_range_le, m_elencrat_fit_le, false);

  setParm(parm_min[8],parm_max[8],parm_val[8],tcp.flex_el_A,
	  m_flexelA_enable_cb,m_flexelA_range_le,m_flexelA_fit_le,true);

  setParm(parm_min[9],parm_max[9],parm_val[9],tcp.flex_el_B,
	  m_flexelB_enable_cb,m_flexelB_range_le,m_flexelB_fit_le,true);
  
  FitThings ft;
  ft.measurement = &m_measurement;

  switch(m_solve_for_cb->currentItem())
    {
    case 0:
      ft.mode = FM_RMS;
      break;
    case 1:
      ft.mode = FM_MAX;
      break;
    }

  s_busy_dialog = new BusyDialog;

  ASA::ASAMain(reinterpret_cast<void*>(&ft), &costFunction,
	       nparm, parm_val, parm_min, parm_max);

  if(s_busy_dialog->isShown())
    {
      // the dialog is not shown only if the cancel button has been pressed

      unsetParm(parm_val[0], m_azoffset_enable_cb, m_azoffset_fit_le, true);
      unsetParm(parm_val[1], m_eloffset_enable_cb, m_eloffset_fit_le, true);
      unsetParm(parm_val[2], m_azns_enable_cb, m_azns_fit_le, true);
      unsetParm(parm_val[3], m_azew_enable_cb, m_azew_fit_le, true);
      unsetParm(parm_val[4], m_elaz_enable_cb, m_elaz_fit_le, true);
      unsetParm(parm_val[5], m_fpcol_enable_cb, m_fpcol_fit_le, true);
      unsetParm(parm_val[6], m_azencrat_enable_cb, m_azencrat_fit_le, false);
      unsetParm(parm_val[7], m_elencrat_enable_cb, m_elencrat_fit_le, false);
      unsetParm(parm_val[8], m_flexelA_enable_cb, m_flexelA_fit_le, true);
      unsetParm(parm_val[9], m_flexelB_enable_cb, m_flexelB_fit_le, true);
      
      delete[] parm_val;
      delete[] parm_max;
      delete[] parm_min;
      
      calculateResidual();
    }
  
  s_busy_dialog->finished();
}

void GUICPSolverPane::enableSolver(bool enable)
{
  m_solve_pb->setEnabled(enable);
}

void GUICPSolverPane::addData(double raw_az, double raw_el, 
		       double cor_az, double cor_el)
{
  if(!controlsEnabled())return;
  justAddData(raw_az,raw_el,cor_az,cor_el);
  calculateResidual();
}

void GUICPSolverPane::justAddData(double raw_az, double raw_el, 
			   double cor_az, double cor_el)
{
  Measurement m;
  m.az_driveangle = raw_az;
  m.el_driveangle = raw_el;
  m.az_real = cor_az;
  m.el_real = cor_el;
  m_measurement.push_back(m);
  m_data_table->setNumRows(m_measurement.size());

  std::ostringstream rawazstream;
  rawazstream << std::fixed << std::showpos << std::setprecision(4) 
	      << Angle::toDeg(raw_az);
  m_data_table->setText(m_measurement.size()-1,0,rawazstream.str().c_str());

  std::ostringstream rawelstream;
  rawelstream << std::fixed << std::showpos << std::setprecision(4) 
	      << Angle::toDeg(raw_el);
  m_data_table->setText(m_measurement.size()-1,1,rawelstream.str().c_str());

  std::ostringstream realazstream;
  realazstream << std::fixed << std::showpos << std::setprecision(4) 
	       << Angle::toDeg(cor_az);
  m_data_table->setText(m_measurement.size()-1,2,realazstream.str().c_str());

  std::ostringstream realelstream;
  realelstream << std::fixed << std::showpos << std::setprecision(4) 
	       << Angle::toDeg(cor_el);
  m_data_table->setText(m_measurement.size()-1,3,realelstream.str().c_str());
}

void GUICPSolverPane::
getCorrectionParameters(CorrectionParameters& cp) const
{
  cp=m_controller->getCorrections();
  
  cp.enable_offsets=true;
  cp.enable_corrections=true;

  double value;
  bool ok;
  
  value=m_azoffset_fit_le->text().toDouble(&ok);
  if((m_azoffset_enable_cb->isOn())&&(ok))cp.az_offset=Angle::frDeg(value);

  value=m_eloffset_fit_le->text().toDouble(&ok);
  if((m_eloffset_enable_cb->isOn())&&(ok))cp.el_offset=Angle::frDeg(value);

  value=m_azns_fit_le->text().toDouble(&ok);
  if((m_azns_enable_cb->isOn())&&(ok))cp.az_ns=Angle::frDeg(value);

  value=m_azew_fit_le->text().toDouble(&ok);
  if((m_azew_enable_cb->isOn())&&(ok))cp.az_ew=Angle::frDeg(value);

  value=m_elaz_fit_le->text().toDouble(&ok);
  if((m_elaz_enable_cb->isOn())&&(ok))cp.el_udew=Angle::frDeg(value);

  value=m_fpcol_fit_le->text().toDouble(&ok);
  if((m_fpcol_enable_cb->isOn())&&(ok))cp.fp_az=Angle::frDeg(value);

  value=m_azencrat_fit_le->text().toDouble(&ok);
  if((m_azencrat_enable_cb->isOn())&&(ok))cp.az_ratio=value;

  value=m_elencrat_fit_le->text().toDouble(&ok);
  if((m_elencrat_enable_cb->isOn())&&(ok))cp.el_ratio=value;

  value=m_flexelA_fit_le->text().toDouble(&ok);
  if((m_flexelA_enable_cb->isOn())&&(ok))cp.flex_el_A=Angle::frDeg(value);

  value=m_flexelB_fit_le->text().toDouble(&ok);
  if((m_flexelB_enable_cb->isOn())&&(ok))cp.flex_el_B=Angle::frDeg(value);
}

void GUICPSolverPane::
setCorrectionParameters(const CorrectionParameters& cp)
{
  m_azoffset_fit_le->
    setText(QString::number(Angle::toDeg(cp.az_offset)));
  m_eloffset_fit_le->
    setText(QString::number(Angle::toDeg(cp.el_offset)));
  m_azns_fit_le->
    setText(QString::number(Angle::toDeg(cp.az_ns)));
  m_azew_fit_le->
    setText(QString::number(Angle::toDeg(cp.az_ew)));
  m_elaz_fit_le->
    setText(QString::number(Angle::toDeg(cp.el_udew)));
  m_fpcol_fit_le->
    setText(QString::number(Angle::toDeg(cp.fp_az)));
  m_azencrat_fit_le->
    setText(QString::number(cp.az_ratio));
  m_elencrat_fit_le->
    setText(QString::number(cp.el_ratio));
  m_flexelA_fit_le->
    setText(QString::number(Angle::toDeg(cp.flex_el_A)));
  m_flexelB_fit_le->
    setText(QString::number(Angle::toDeg(cp.flex_el_B)));
  
  if(!m_azoffset_enable_cb->isOn())m_azoffset_enable_cb->toggle();
  if(!m_eloffset_enable_cb->isOn())m_eloffset_enable_cb->toggle();
  if(!m_azns_enable_cb->isOn())m_azns_enable_cb->toggle();
  if(!m_azew_enable_cb->isOn())m_azew_enable_cb->toggle();
  if(!m_elaz_enable_cb->isOn())m_elaz_enable_cb->toggle();
  if(!m_fpcol_enable_cb->isOn())m_fpcol_enable_cb->toggle();
  if(!m_azencrat_enable_cb->isOn())m_azencrat_enable_cb->toggle();
  if(!m_elencrat_enable_cb->isOn())m_elencrat_enable_cb->toggle();
  if(!m_flexelA_enable_cb->isOn())m_flexelA_enable_cb->toggle();
  if(!m_flexelB_enable_cb->isOn())m_flexelB_enable_cb->toggle();

  calculateResidual();
}

void GUICPSolverPane::calculateResidual()
{
  double residual=0;
  double residual_az=0;
  double residual_el=0;

  double max_residual=0;
  double max_residual_az=0;
  double max_residual_el=0;

  if(m_measurement.size())
    {
      CorrectionParameters cp;
      getCorrectionParameters(cp);
      for(unsigned i=0; i<m_measurement.size();i++)
	{
	  double az = m_measurement[i].az_driveangle;
	  double el = m_measurement[i].el_driveangle;
	  
	  cp.undoAzElCorrections(az,el,true);
	  
	  std::ostringstream corazstream;
	  corazstream << std::fixed << std::showpos << std::setprecision(4) 
		      << Angle::toDeg(az);
	  m_data_table->setText(i,4,corazstream.str().c_str());
	  
	  std::ostringstream corelstream;
	  corelstream << std::fixed << std::showpos << std::setprecision(4) 
		      << Angle::toDeg(el);
	  m_data_table->setText(i,5,corelstream.str().c_str());
	  
	  double azr = m_measurement[i].az_real;
	  double elr = m_measurement[i].el_real;
	  
	  std::ostringstream delazstream;
	  delazstream << std::fixed << std::showpos << std::setprecision(4) 
		      << Angle::toDeg(az-azr)*cos(el);
	  m_data_table->setText(i,6,delazstream.str().c_str());
	  
	  std::ostringstream delelstream;
	  delelstream << std::fixed << std::showpos << std::setprecision(4) 
		      << Angle::toDeg(el-elr);
	  m_data_table->setText(i,7,delelstream.str().c_str());
	  
	  SphericalCoords scc=SphericalCoords::makeLatLongRad(el,az);
	  SphericalCoords scr=SphericalCoords::makeLatLongRad(elr,azr);
	  double sep = scc.separation(scr);
	  
	  std::ostringstream residualstream;
	  residualstream << std::fixed << std::showpos << std::setprecision(4) 
			 << Angle::toDeg(sep);
	  m_data_table->setText(i,8,residualstream.str().c_str());
	  
	  double this_residual = sep*sep;
	  double this_residual_az = (az-azr)*cos(el)*(az-azr)*cos(el);
	  double this_residual_el = (el-elr)*(el-elr);
	  
	  residual += this_residual;
	  residual_az += this_residual_az;
	  residual_el += this_residual_el;
	  
	  if(this_residual>max_residual)max_residual=this_residual;
	  if(this_residual_az>max_residual_az)max_residual_az=this_residual_az;
	  if(this_residual_el>max_residual_el)max_residual_el=this_residual_el;
	}
      
      residual = sqrt(residual/m_measurement.size());
      residual_az = sqrt(residual_az/m_measurement.size());
      residual_el = sqrt(residual_el/m_measurement.size());

      max_residual=sqrt(max_residual);
      max_residual_az=sqrt(max_residual_az);
      max_residual_el=sqrt(max_residual_el);
    }

  std::ostringstream stream;

  stream.str("");
  stream << std::fixed << std::showpos << std::setprecision(4) 
	 << Angle::toDeg(residual_az);
  m_residual_rms_az_le->setText(stream.str().c_str());

  stream.str("");
  stream << std::fixed << std::showpos << std::setprecision(4) 
	 << Angle::toDeg(residual_el);
  m_residual_rms_el_le->setText(stream.str().c_str());

  stream.str("");
  stream << std::fixed << std::showpos << std::setprecision(4) 
	 << Angle::toDeg(residual);
  m_residual_rms_le->setText(stream.str().c_str());

  stream.str("");
  stream << std::fixed << std::showpos << std::setprecision(4) 
	 << Angle::toDeg(max_residual_az);
  m_residual_max_az_le->setText(stream.str().c_str());

  stream.str("");
  stream << std::fixed << std::showpos << std::setprecision(4) 
	 << Angle::toDeg(max_residual_el);
  m_residual_max_el_le->setText(stream.str().c_str());

  stream.str("");
  stream << std::fixed << std::showpos << std::setprecision(4) 
	 << Angle::toDeg(max_residual);
  m_residual_max_le->setText(stream.str().c_str());
}

void GUICPSolverPane::deleteMeasurement()
{
  unsigned row = m_data_table->currentRow();
  unsigned col = m_data_table->currentColumn();
  if((row>=0)&&(row<m_measurement.size()))
    {
      m_data_table->removeRow(row);
      //      for(unsigned j=row;j<m_measurement.size()-1;j++)
      //	m_measurement[j]=m_measurement[j+1];
      m_measurement.erase(m_measurement.begin()+row);
      if(row>=m_measurement.size())row=m_measurement.size()-1;
      if(row>=0)m_data_table->setCurrentCell(row,col);
      calculateResidual();
    }
}

void GUICPSolverPane::saveCorrections()
{
#if 0
  std::string default_fn = 
    CorrectionParameters::saveFilename(m_scope_id);

  QString fn = 
    QFileDialog::getSaveFileName(default_fn,
				 "Data Files (*.dat);;All Files (*)",
				 this, "save dialog", 
				 "Select Corrections File");
  if (!fn.isEmpty())
    {
      CorrectionParameters tcp; 
      getCorrectionParameters(tcp);
      if(!tcp.save(fn))
	QMessageBox::warning(0,"Save Corrections",
			     "Could not save tracking corrections",
			     QMessageBox::Ok,QMessageBox::NoButton);
    }
#else
  CorrectionParameters tcp;
  getCorrectionParameters(tcp);
  if (GUICorrectionDialogs::save(tcp, m_scope_id, this, "save dialog") 
      == GUICorrectionDialogs::S_FAIL)
    {
      QMessageBox::warning(0,"Save Corrections",
			   "Could not save tracking corrections",
			   QMessageBox::Ok,QMessageBox::NoButton);
    }
#endif
}

void GUICPSolverPane::loadCorrections()
{
#if 0
  std::string default_fn = 
    CorrectionParameters::loadFilename(m_scope_id);

  QString fn = 
    QFileDialog::getOpenFileName(default_fn,
				 "Data Files (*.dat);;All Files (*)",
				 this, "load dialog", 
				 "Select Corrections File");
  if (!fn.isEmpty())
    {
      CorrectionParameters tcp;
      if(tcp.load(fn))
	setCorrectionParameters(tcp);
      else
	QMessageBox::warning(0,"Load Corrections",
			     "Could not load tracking corrections",
			     QMessageBox::Ok,QMessageBox::NoButton);
    }
#else
  CorrectionParameters tcp;
  GUICorrectionDialogs::Status s = 
    GUICorrectionDialogs::loadFromDB(tcp, m_scope_id, this, "load dialog");
  switch(s)
    {
    case GUICorrectionDialogs::S_GOOD:
      setCorrectionParameters(tcp);
      break;
    case GUICorrectionDialogs::S_FAIL:
      QMessageBox::warning(0,"Load Corrections",
			   "Could not load tracking corrections",
			   QMessageBox::Ok,QMessageBox::NoButton);
      break;
    case GUICorrectionDialogs::S_CANCEL:
      break;
    }
#endif
}

double GUICPSolverPane::
costFunction(double* x, double* pmin, double* pmax, double* tangent, 
	     double* curvature, ALLOC_INT* nparams, int* ptype, int* flag,
	     int *code, USER_DEFINES* opts)
{
  static int call_number=0;
  if(call_number++ == 10)
    {
      if(!s_busy_dialog->isShown())
	{
	  // cancel button pressed
	  opts->Immediate_Exit=TRUE;
	}
      else
	{
	  s_busy_dialog->tick();
	}

      call_number=0;
    }

  for(ALLOC_INT i=0;i<*nparams;i++)
    {
      // Debug::stream() 
      //   << pmin[i] << '\t' << x[i] << '\t' << pmax[i] << std::endl;
      if((x[i]<pmin[i])||(x[i]>pmax[i]))
	{
	  *flag=FALSE;
	  return std::numeric_limits<double>::infinity();
	}
    }

  *flag=TRUE;
  
  CorrectionParameters tcp;

  tcp.enable_offsets = true;
  tcp.enable_corrections = true;

  tcp.az_offset  = x[0];
  tcp.el_offset  = x[1];
  tcp.az_ns      = x[2];
  tcp.az_ew      = x[3];
  tcp.el_udew    = x[4];
  tcp.fp_az      = x[5];
  tcp.az_ratio   = x[6];
  tcp.el_ratio   = x[7];
  tcp.flex_el_A  = x[8];
  tcp.flex_el_B  = x[9];

  FitThings* ft = reinterpret_cast<FitThings*>(opts->Asa_Data_Ptr);

  double residual = 0;
  for(unsigned i=0; i<ft->measurement->size(); i++)
    {
      double az = (*ft->measurement)[i].az_driveangle;
      double el = (*ft->measurement)[i].el_driveangle;

      tcp.undoAzElCorrections(az,el,true);
      
      double azr = (*ft->measurement)[i].az_real;
      double elr = (*ft->measurement)[i].el_real;
      
      SphericalCoords scc=SphericalCoords::makeLatLongRad(el,az);
      SphericalCoords scr=SphericalCoords::makeLatLongRad(elr,azr);
      double sep = scc.separation(scr);

      double this_residual = sep*sep;

      switch(ft->mode)
	{
	case FM_RMS:
	  residual += this_residual;
	  break;
	  
	case FM_MAX:
	  if(this_residual>residual)residual = this_residual;
	  break;
	}
    }
  return residual;
}

void GUICPSolverPane::update(const GUIUpdateData& ud)
{
  if(!isVisible())return;
  if((ud.full_update)||(ud.replay))
    {
      bool canchange = ((controlsEnabled())&&
			(ud.tse.req==TelescopeController::REQ_STOP));
      enableSolver(canchange);
    }
}
BusyDialog* GUICPSolverPane::s_busy_dialog;
