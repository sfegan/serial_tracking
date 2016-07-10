//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUITCPPane.h
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
 * $Date: 2007/02/28 17:55:24 $
 * $Revision: 2.4 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUITCPPANE_H
#define VTRACKING_GUITCPPANE_H

#include<qhbox.h>
#include<qlineedit.h>
#include<qgroupbox.h>

#include<Angle.h>
#include<CorrectionParameters.h>

#include"GUITabWidget.h"

class TripleButtonWidget: public QHBox
{ 
  Q_OBJECT
public:
  TripleButtonWidget(double min, double max, double small_step,
		     int point, const QString& tooltip,
		     QWidget* parent = 0, const char* name = 0);
  virtual ~TripleButtonWidget();
  
  QLineEdit* le() const { return m_le; }
  double value() const;
public slots:
  void zeroValue();
  void setValue(double value);
signals:
  void valueChanged(double);
private slots:
  void mmmPushed();
  void mmPushed();
  void mPushed();
  void pPushed();
  void ppPushed();
  void pppPushed();
  void zeroPushed();
  void returnPushed();
private:
  void delta(double d);
  QLineEdit* m_le;
  double m_min;
  double m_max;
  double m_small_step;
  int m_dp;
};

class GUITCPPane: public QFrame, public GUITabPane
{
  Q_OBJECT
public:
  GUITCPPane(const SEphem::CorrectionParameters& tcp,
	     unsigned scope_id,
	     QWidget* parent = 0, const char* name = 0);
  virtual ~GUITCPPane();

  virtual void update(const GUIUpdateData& ud);

  SEphem::CorrectionParameters getParameters() const;
public slots:
  void setParameters(const SEphem::CorrectionParameters& tcp);
  
signals:
  void parametersChanged();
  void recordPosition(double raw_az, double raw_el,   
		      double cor_az, double cor_el);
  
private slots:
  void somethingHasChanged();
  void tbwHasChanged();
  void save();
  void load();
  void zeroAlignments();
  void zeroAll();
  void recordButtonPressed();

private:
  unsigned m_scope_id;
  double m_last_raw_az;
  double m_last_raw_el;
  SEphem::Angle m_last_cor_az;
  SEphem::Angle m_last_cor_el;
  SEphem::Angle m_last_tar_az;
  SEphem::Angle m_last_tar_el;
  SEphem::Angle m_last_last_tar_az;
  SEphem::Angle m_last_last_tar_el;
  bool m_last_com_failure;
  bool m_last_has_target;
  bool m_last_last_has_target;

  QGroupBox* m_offsetsbox;
  QGroupBox* m_correctionsbox;
  QGroupBox* m_measurementbox;
  QGroupBox* m_vffbox;

  QLineEdit* m_tar_az;
  QLineEdit* m_tar_el;
  QLineEdit* m_raw_az;
  QLineEdit* m_raw_el;
  QLineEdit* m_cor_az;
  QLineEdit* m_cor_el;
  QLineEdit* m_del_az;
  QLineEdit* m_del_el;

  QLineEdit* m_az_offset;
  QLineEdit* m_el_offset;
  QLineEdit* m_az_ratio;
  QLineEdit* m_el_ratio;
  QLineEdit* m_az_ns;
  QLineEdit* m_az_ew;
  QLineEdit* m_el_udew;
  QLineEdit* m_fp_az;
  QLineEdit* m_flex_el_A;
  QLineEdit* m_flex_el_B;

  TripleButtonWidget* m_el_tbw;
  TripleButtonWidget* m_fp_tbw;

  QLineEdit* m_el_pos_vff_s;
  QLineEdit* m_el_pos_vff_i;
  QLineEdit* m_el_neg_vff_s;
  QLineEdit* m_el_neg_vff_i;
  QLineEdit* m_az_pos_vff_s;
  QLineEdit* m_az_pos_vff_i;
  QLineEdit* m_az_neg_vff_s;
  QLineEdit* m_az_neg_vff_i;
};

#endif // VTRACKING_GUITCPPANE_H
