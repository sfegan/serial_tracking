//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIOscilloscopePane.h
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
 * $Date: 2010/10/28 14:48:05 $
 * $Revision: 2.3 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUIOSCILLOSCOPEPANE_H
#define VTRACKING_GUIOSCILLOSCOPEPANE_H

#ifndef GUI_NO_QWT

#include<map>

#include<qlineedit.h>
#include<qlabel.h>
#include<qpushbutton.h>
#include<qframe.h>

#include<qwt.h>
#include<qwt_plot.h>


#include"GUITabWidget.h"
#include"GUIMisc.h"

class GUIOscilloscopePane: 
  public QFrame, public GUITabPane
{
  Q_OBJECT
public:
  enum ScaleMode { SM_AUTO, SM_FIXED, SM_FIXED_EQUAL };

  GUIOscilloscopePane(unsigned history_size, unsigned history_zoom,
		      unsigned update_period,
		      QWidget* parent=0, const char* name=0);
  virtual ~GUIOscilloscopePane();
  
  void update(const GUIUpdateData& ud);

public slots:
  void clear();
  void setPausePlotting(bool pause);
  void setAutoscalePlot(ScaleMode scale_mode);
  void setZoomPlot(bool zoom);
  void setCurveOn(long curve, bool on);
  void setDataSet(int dataset);
  void togglePausePlotting();
  void toggleAutoscalePlot();
  void toggleZoomPlot();
  void toggleCurve(long curve);

private:

  void draw();

  unsigned                             m_update_period;
  unsigned                             m_history_size;
  unsigned                             m_history_zoom;

  QwtPlot*                             m_plot;
  QLineEdit*                           m_trace1_le;
  QLineEdit*                           m_trace2_le;
  QLineEdit*                           m_elvel_le;

  QComboBox*                           m_data_cb;
  QPushButton*                         m_halt_button;
  QPushButton*                         m_scale_button;
  QPushButton*                         m_reset_button;
  QPushButton*                         m_zoom_button;

  int                                  m_data_set;
  bool                                 m_halt_plot;
  ScaleMode                            m_scale_mode;
  bool                                 m_zoom;

  long                                 m_curve1;
  long                                 m_curve2;
  long                                 m_curve3;
  std::map<long,bool>                  m_curve_on;

  class HistoryDatum
  {
  public:
    HistoryDatum(): mjd(), azvel(), elvel(), azerr(), elerr(), 
		    adc1(), adc2() {}
    HistoryDatum(double _mjd, 
		 double _azvel, double _elvel, 
		 double _azerr, double _elerr, 
		 double _adc1, double _adc2): 
      mjd(_mjd), azvel(_azvel), elvel(_elvel), azerr(_azerr), elerr(_elerr), 
      adc1(_adc1), adc2(_adc2) { /* nothing to see here */ }
    double mjd;
    double azvel;
    double elvel;
    double azerr;
    double elerr;
    double adc1;
    double adc2;
  };

  std::vector<HistoryDatum>            m_history;
  std::vector<HistoryDatum>            m_paused_history;
};

#endif // GUI_NO_QWT

#endif // VTRACKING_GUIOSCILLOSCOPEPANE_H
