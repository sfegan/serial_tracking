//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIOscilloscopePane.cpp
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
 * $Revision: 2.4 $
 * $Tag$
 *
 **/

#include <iomanip>

#include"GUIOscilloscopePane.h"

#ifndef GUI_NO_QWT

#include<qlineedit.h>
#include<qlabel.h>
#include<qpushbutton.h>
#include<qframe.h>
#include<qlayout.h>

#include<qwt.h>
#include<qwt_plot.h>


#include"GUITabWidget.h"
#include"GUIMisc.h"

#define PENWIDTH 0

using namespace VTracking;

GUIOscilloscopePane::
GUIOscilloscopePane(unsigned history_size, unsigned history_zoom,
		    unsigned update_period,
		    QWidget* parent, const char* name):
  QFrame(parent,name), GUITabPane(this),
  m_update_period(update_period), m_history_size(history_size),
  m_history_zoom(history_zoom), m_plot(0), m_trace1_le(0), m_trace2_le(0), 
  m_elvel_le(0), m_data_cb(0), m_halt_button(0), m_scale_button(0), 
  m_reset_button(0), m_zoom_button(0),
  m_data_set(0), m_halt_plot(false), m_scale_mode(SM_AUTO), m_zoom(false), 
  m_curve1(0), m_curve2(0), m_curve3(0), m_curve_on(),
  m_history(), m_paused_history()
{
  QString basename(name);

  // LAYOUT AND FRAMES
  
  QGridLayout* outerlayout = 
    new QGridLayout(this,2,1,5,5,basename+" outer layout");
  
  QGroupBox* plotbox = new MyQGroupBox(1,Qt::Horizontal,"Oscilloscope",
				       this,basename+" plot box");
  QFrame* plotframe = new QFrame(plotbox,basename+" plot box frame");
  QGridLayout* plotlayout = 
    new QGridLayout(plotframe,2,6,0,5,basename+" plot box frame layout");
  
  QGroupBox* cmdbox =
    new MyQGroupBox(1,Qt::Horizontal,"Commands",this,basename+" cmd box");  
  QFrame* cmdframe = new QFrame(cmdbox,basename+" cmd box frame");
  QGridLayout* cmdlayout = 
    new QGridLayout(cmdframe,1,5,0,5,basename+" cmd box frame layout");


  // ----------
  // PLOT FRAME
  // ----------

  // CREATE PLOT
  
  m_plot = new QwtPlot("ADC voltage and velocity",
		       plotframe,basename+" qwtplot");
  m_plot->setCanvasBackground(white);
  m_plot->setAxisScale(QwtPlot::xBottom,
		       -double(m_history_size)*m_update_period/1000,0);

  m_data_set=1;  // Fake setDataSet() into thinking the 
  setDataSet(0); // dataset has been changed

  // CREATE LINE EDITS FOR READOUT OF MEAN/VAR

  struct status_entry
  {
    QGridLayout* layout;
    int row;
    int col;
    int width;
    QLineEdit** le;
    QString label;
    QString texttemplate;
    double fontratio;
  };
  
  struct status_entry status_entries[] = {
    { plotlayout, 1, 1, 1, &m_trace1_le, "Trace1",       "+18.888", 1.0 },
    { plotlayout, 1, 3, 1, &m_trace2_le, "Trace2",       "+18.888", 1.0 },
    { plotlayout, 1, 5, 1, &m_elvel_le,  "El Velocity",  "+0.88888", 1.0 } };

  for(unsigned i=0; i<sizeof(status_entries)/sizeof(*status_entries);i++)
    {
      struct status_entry* entry = status_entries+i;

      QLabel* lab(0);
      if(!entry->label.isNull())
	{
	  lab = new QLabel(entry->label,entry->layout->mainWidget(), 
				 basename+""+entry->label+QString(" label"));
	  entry->layout->addWidget(lab,entry->row,entry->col-1);
	}
      
      (*entry->le) = 
	new InfoQLineEdit(entry->texttemplate, entry->fontratio, true, false,
		  entry->layout->mainWidget(), basename+" "+entry->label+" le");

      entry->layout->addMultiCellWidget(*entry->le,entry->row,entry->row,
					entry->col,entry->col+entry->width-1);

      if((lab)&&(lab->font() != (*entry->le)->font()))
	lab->setFont((*entry->le)->font());
    } 

  plotlayout->addMultiCellWidget(m_plot,0,0,0,7);

  // -------------
  // COMMAND FRAME
  // -------------

  m_data_cb = new QComboBox(cmdframe," cmd data select cb");
  m_data_cb->setEditable(false);
  m_data_cb->insertItem("ADC values");
  m_data_cb->insertItem("Tracking errors");
  m_data_cb->insertItem("Velocities");
  m_data_cb->insertItem("Azimuth error and velocity");
  m_data_cb->insertItem("Elevation error and velocity");

  m_reset_button = 
    new QPushButton("Reset Plot",cmdframe,basename+" cmd reset plot button");
  m_halt_button = new QPushButton("Pause plotting",cmdframe,
				  basename+" cmd pause plot button");
  m_scale_button = new QPushButton("Fixed y-axis scale",cmdframe,
				   basename+" cmd scale plot button");
  m_zoom_button = new QPushButton("Zoom recent data",cmdframe,
				   basename+" cmd zoom plot button");
  
  connect(m_data_cb,SIGNAL(activated(int)),this,SLOT(setDataSet(int)));
  connect(m_reset_button,SIGNAL(clicked()),this,SLOT(clear()));
  connect(m_halt_button,SIGNAL(clicked()), this,SLOT(togglePausePlotting()));
  connect(m_scale_button,SIGNAL(clicked()), this,SLOT(toggleAutoscalePlot()));
  connect(m_zoom_button,SIGNAL(clicked()), this,SLOT(toggleZoomPlot()));
  connect(m_plot,SIGNAL(legendClicked(long)), this,SLOT(toggleCurve(long)));
  
  cmdlayout->addWidget(m_data_cb,0,0);
  cmdlayout->addWidget(m_reset_button,0,1);
  cmdlayout->addWidget(m_halt_button,0,2);  
  cmdlayout->addWidget(m_scale_button,0,3);  
  cmdlayout->addWidget(m_zoom_button,0,4);  
  
  // ADD OUTER LAYOUTS
  outerlayout->addWidget(plotbox,0,0);
  outerlayout->addWidget(cmdbox,1,0);
}

GUIOscilloscopePane::~GUIOscilloscopePane()
{
  // nothing to see here
}
  
void GUIOscilloscopePane::update(const GUIUpdateData& ud)
{
  if((ud.full_update)&&(!ud.replay)&&
     (ud.tse.state != TelescopeController::TS_COM_FAILURE))
    {
      HistoryDatum 
	datum(ud.mjd,
	      ud.tse.az_driveangle_estimated_speed_dps,
	      ud.tse.el_driveangle_estimated_speed_dps,
	      ud.tse.status.az.driveangle_deg-ud.last_cmd_az_driveangle,
	      ud.tse.status.el.driveangle_deg-ud.last_cmd_el_driveangle,
	      ud.tse.status.Analog1, ud.tse.status.Analog2);
      m_history.push_back(datum);

      if(m_history.size() > m_history_size*2)
	m_history.erase(m_history.begin(),
			m_history.begin()+m_history.size()-m_history_size);

      draw();
    }
  else if(ud.replay)draw();

  if(isVisible())
    {
      m_halt_button->setEnabled(controlsEnabled());
      m_scale_button->setEnabled(controlsEnabled());
      m_reset_button->setEnabled(controlsEnabled());
    }
}

void GUIOscilloscopePane::draw()
{
  if(!isVisible())return;

  double* x = new double[m_history_size];
  double* velocity = new double[m_history_size];
  double* trace1 = new double[m_history_size];
  double* trace2 = new double[m_history_size];

  QString trace1_str;
  QString trace2_str;
  QString vel_str;
  
  unsigned desired = m_history_size;
  if(m_zoom)desired = m_history_zoom;

  std::vector<HistoryDatum>* data = &m_history;
  if(m_halt_plot)data=&m_paused_history;

  unsigned entries = (*data).size();
  if(entries > desired)entries=desired;

  if(entries!=0)
    {
      double sum_trace1=0;
      double ssq_trace1=0;
      double sum_trace2=0;
      double ssq_trace2=0;
      double sum_vel=0;
      double ssq_vel=0;
      
      double t0 = (*data)[(*data).size()-1].mjd;

      int cut=0;
      for(unsigned i=0;i<entries;i++)
	{
	  double t = ((*data)[(*data).size()-entries+i].mjd-t0)*24*3600;
	  double elvel = (*data)[(*data).size()-entries+i].elvel;
	  double tr1;
	  double tr2;
	  
	  switch(m_data_set)
	    {
	    case 0:
	      tr1 = (*data)[(*data).size()-entries+i].adc1;
	      tr2 = (*data)[(*data).size()-entries+i].adc2;
	      break;

	    case 1:
	      tr1 = (*data)[(*data).size()-entries+i].elerr;
	      tr2 = (*data)[(*data).size()-entries+i].azerr;
	      break;

	    case 2:
	      tr1 = (*data)[(*data).size()-entries+i].elvel;
	      tr2 = (*data)[(*data).size()-entries+i].azvel;
	      break;

	    case 3:
	      tr1 = (*data)[(*data).size()-entries+i].azerr;
	      tr2 = (*data)[(*data).size()-entries+i].azvel;
	      break;

	    case 4:
	      tr1 = (*data)[(*data).size()-entries+i].elerr;
	      tr2 = (*data)[(*data).size()-entries+i].elvel;
	      break;
	    }

	  if(t >= -double(m_history_size)*double(m_update_period)/1000.0)
	    {
	      x[i-cut] = t;
	      velocity[i-cut] = elvel;
	      trace1[i-cut] = tr1;
	      trace2[i-cut] = tr2;

	      sum_trace1 += tr1;
	      ssq_trace1 += tr1*tr1;
	      sum_trace2 += tr2;
	      ssq_trace2 += tr2*tr2;
	      sum_vel += elvel;
	      ssq_vel += elvel*elvel;
	    }
	  else
	    {
	      cut++;
	    }
	}
      entries -= cut;

      double mean_trace1 = sum_trace1/double(entries);
      double mean_trace2 = sum_trace2/double(entries);
      double mean_vel = sum_vel/double(entries);

      std::ostringstream mean_trace1_stream;
      std::ostringstream rms_trace1_stream;
      std::ostringstream mean_trace2_stream;
      std::ostringstream rms_trace2_stream;
      std::ostringstream mean_vel_stream;
      std::ostringstream rms_vel_stream;

      QString units1;
      QString units2;
      
      switch(m_data_set)
	{
	case 0:
	  units1 = "V";
	  units2 = "V";
	  break;
	case 1:
	  units1 = MAKEDEG("");
	  units2 = MAKEDEG("");
	  break;
	case 2:
	  units1 = MAKEDPS("");
	  units2 = MAKEDPS("");
	  break;
	case 3:
	case 4:
	  units1 = MAKEDEG("");
	  units2 = MAKEDPS("");
	  break;
	}

      mean_trace1_stream << std::showpos << std::fixed << std::setprecision(5) 
			 << mean_trace1;
      rms_trace1_stream << std::fixed << std::setprecision(5) 
			<< sqrt(ssq_trace1/double(entries) - 
				mean_trace1*mean_trace1) << units1;
      mean_trace2_stream << std::showpos << std::fixed << std::setprecision(5) 
			 << mean_trace2;
      rms_trace2_stream << std::fixed << std::setprecision(5) 
			<< sqrt(ssq_trace2/double(entries) - 
				mean_trace2*mean_trace2) << units2;
      mean_vel_stream << std::showpos << std::fixed << std::setprecision(5) 
		      << mean_vel;
      rms_vel_stream << std::fixed << std::setprecision(5) 
		     << sqrt(ssq_vel/double(entries) - mean_vel*mean_vel);
      
      trace1_str = 
	XPLUSMINUSY(mean_trace1_stream.str(), rms_trace1_stream.str());
      
      trace2_str = 
	XPLUSMINUSY(mean_trace2_stream.str(), rms_trace2_stream.str());

      vel_str = 
	XPLUSMINUSY(mean_vel_stream.str(), MAKEDPS(rms_vel_stream.str()));
	}

  m_trace1_le->setText(trace1_str);
  m_trace2_le->setText(trace2_str);
  m_elvel_le->setText(vel_str);

  m_plot->setCurveData(m_curve1,x,trace1,entries);
  m_plot->setCurveData(m_curve2,x,trace2,entries);
  m_plot->setCurveData(m_curve3,x,velocity,entries);
  m_plot->replot();

  delete[] x;
  delete[] velocity;
  delete[] trace1;
  delete[] trace2;
}

void GUIOscilloscopePane::clear()
{
  m_history.clear();
  draw();
}

void GUIOscilloscopePane::setPausePlotting(bool pause)
{
  m_halt_plot=pause;
  if(pause)m_paused_history=m_history;
  else m_paused_history.clear();
  if(!pause)draw();
}

void GUIOscilloscopePane::setAutoscalePlot(ScaleMode scale_mode)
{
  m_scale_mode=scale_mode;
  
  switch(m_scale_mode)
    {
    case SM_AUTO:
      m_scale_button->setText("Fixed y-axis scale");
      m_plot->setAxisAutoScale(QwtPlot::yLeft);
      m_plot->setAxisAutoScale(QwtPlot::yRight);
      break;
    case SM_FIXED:
      m_plot->setAxisScale(QwtPlot::yLeft,
			   m_plot->axisScale(QwtPlot::yLeft)->lBound(),
			   m_plot->axisScale(QwtPlot::yLeft)->hBound());
      m_plot->setAxisScale(QwtPlot::yRight,
			   m_plot->axisScale(QwtPlot::yRight)->lBound(),
			   m_plot->axisScale(QwtPlot::yRight)->hBound());
      m_scale_button->setText("Equal y-axis scale");
      break;
    case SM_FIXED_EQUAL:
      {
	double lbound = m_plot->axisScale(QwtPlot::yLeft)->lBound();
	if(m_plot->axisScale(QwtPlot::yRight)->lBound() < lbound)
	  lbound = m_plot->axisScale(QwtPlot::yRight)->lBound();

	double hbound = m_plot->axisScale(QwtPlot::yLeft)->hBound();
	if(m_plot->axisScale(QwtPlot::yRight)->hBound() > hbound)
	  hbound = m_plot->axisScale(QwtPlot::yRight)->hBound();

	m_plot->setAxisScale(QwtPlot::yLeft, lbound, hbound);
	m_plot->setAxisScale(QwtPlot::yRight, lbound, hbound);
	m_scale_button->setText("Auto y-axis scale");
      }
      break;
    }
}

void GUIOscilloscopePane::setZoomPlot(bool zoom)
{
  if(m_zoom != zoom)
    {
      m_zoom=zoom;

      if(zoom)
	{
	  m_zoom_button->setText("Show all data");
	  m_plot->setAxisScale(QwtPlot::xBottom,
			       -double(m_history_zoom)*m_update_period/1000,0);
	}
      else
	{
	  m_zoom_button->setText("Zoom recent data");
	  m_plot->setAxisScale(QwtPlot::xBottom,
			       -double(m_history_size)*m_update_period/1000,0);
	}	  
      draw();
    }
}

void GUIOscilloscopePane::setCurveOn(long curve, bool on)
{
  if(m_curve_on[curve] != on)
    {
      m_curve_on[curve] = on;
      if(on)m_plot->setCurveStyle(curve,QwtCurve::Lines);
      else m_plot->setCurveStyle(curve,QwtCurve::NoCurve);
      m_plot->replot();
    }
}

void GUIOscilloscopePane::setDataSet(int dataset)
{
  if(m_data_set != dataset)
    {
      if(m_curve1) { m_plot->removeCurve(m_curve1); m_curve1=0; }
      if(m_curve2) { m_plot->removeCurve(m_curve2); m_curve2=0; }
      if(m_curve3) { m_plot->removeCurve(m_curve3); m_curve3=0; }

      switch(dataset)
	{
	case 0:
	  // ADC values and El Velocity
	  m_plot->enableAxis(QwtPlot::yRight,true);
	  m_plot->setAxisTitle(QwtPlot::xBottom, "Time [s]");
	  m_plot->setAxisTitle(QwtPlot::yLeft, "Voltage [V]");
	  m_plot->setAxisTitle(QwtPlot::yRight, 
			       MAKEDEG("El Velocity [")+"/s]");
	  m_curve1 = m_plot->insertCurve("ADC 1", 
					 QwtPlot::xBottom,QwtPlot::yLeft);
	  m_curve2 = m_plot->insertCurve("ADC 2",
					 QwtPlot::xBottom,QwtPlot::yLeft);
	  m_curve3 = m_plot->insertCurve("El Velocity", 
					 QwtPlot::xBottom,QwtPlot::yRight);

	  m_curve_on[m_curve1]=true;
	  m_curve_on[m_curve2]=true;
	  m_curve_on[m_curve3]=true;

	  m_plot->setCurvePen(m_curve1,QPen(blue,PENWIDTH));
	  m_plot->setCurvePen(m_curve2,QPen(black,PENWIDTH));
	  m_plot->setCurvePen(m_curve3,QPen(red,PENWIDTH));

	  m_plot->enableLegend(true);
	  m_plot->setTitle("ADC voltage and elevation velocity");
	  break;

	case 1:
	  m_plot->enableAxis(QwtPlot::yRight,true);
	  m_plot->setAxisTitle(QwtPlot::xBottom, "Time [s]");
	  m_plot->setAxisTitle(QwtPlot::yLeft, MAKEDEG("El Error [")+"]");
	  m_plot->setAxisTitle(QwtPlot::yRight, MAKEDEG("Az Error [")+"]");
	  m_curve1 = m_plot->insertCurve("El Error", 
					 QwtPlot::xBottom,QwtPlot::yLeft);
	  m_curve2 = m_plot->insertCurve("Az Error",
					 QwtPlot::xBottom,QwtPlot::yRight);

	  m_curve_on[m_curve1]=true;
	  m_curve_on[m_curve2]=true;

	  m_plot->setCurvePen(m_curve1,QPen(blue,PENWIDTH));
	  m_plot->setCurvePen(m_curve2,QPen(red,PENWIDTH));

	  m_plot->enableLegend(true);
	  m_plot->setTitle("El and Az Error");
	  break;

	case 2:
	  m_plot->enableAxis(QwtPlot::yRight,true);
	  m_plot->setAxisTitle(QwtPlot::xBottom, "Time [s]");
	  m_plot->setAxisTitle(QwtPlot::yLeft, MAKEDPS("El Velocity [")+"]");
	  m_plot->setAxisTitle(QwtPlot::yRight, MAKEDPS("Az Velocity [")+"]");
	  m_curve1 = m_plot->insertCurve("El Velocity", 
					 QwtPlot::xBottom,QwtPlot::yLeft);
	  m_curve2 = m_plot->insertCurve("Az Velocity",
					 QwtPlot::xBottom,QwtPlot::yRight);

	  m_curve_on[m_curve1]=true;
	  m_curve_on[m_curve2]=true;

	  m_plot->setCurvePen(m_curve1,QPen(blue,PENWIDTH));
	  m_plot->setCurvePen(m_curve2,QPen(red,PENWIDTH));

	  m_plot->enableLegend(true);
	  m_plot->setTitle("El and Az Velocity");
	  break;

	case 3:
	  m_plot->enableAxis(QwtPlot::yRight,true);
	  m_plot->setAxisTitle(QwtPlot::xBottom, "Time [s]");
	  m_plot->setAxisTitle(QwtPlot::yLeft, MAKEDEG("Az Error [")+"]");
	  m_plot->setAxisTitle(QwtPlot::yRight, MAKEDPS("Az Velocity [")+"]");
	  m_curve1 = m_plot->insertCurve("Az Error", 
					 QwtPlot::xBottom,QwtPlot::yLeft);
	  m_curve2 = m_plot->insertCurve("Az Velocity",
					 QwtPlot::xBottom,QwtPlot::yRight);

	  m_curve_on[m_curve1]=true;
	  m_curve_on[m_curve2]=true;

	  m_plot->setCurvePen(m_curve1,QPen(blue,PENWIDTH));
	  m_plot->setCurvePen(m_curve2,QPen(red,PENWIDTH));

	  m_plot->enableLegend(true);
	  m_plot->setTitle("Az Error and Velocity");
	  break;

	case 4:
	  m_plot->enableAxis(QwtPlot::yRight,true);
	  m_plot->setAxisTitle(QwtPlot::xBottom, "Time [s]");
	  m_plot->setAxisTitle(QwtPlot::yLeft, MAKEDEG("El Error [")+"]");
	  m_plot->setAxisTitle(QwtPlot::yRight, MAKEDPS("El Velocity [")+"]");
	  m_curve1 = m_plot->insertCurve("El Error", 
					 QwtPlot::xBottom,QwtPlot::yLeft);
	  m_curve2 = m_plot->insertCurve("El Velocity",
					 QwtPlot::xBottom,QwtPlot::yRight);

	  m_curve_on[m_curve1]=true;
	  m_curve_on[m_curve2]=true;

	  m_plot->setCurvePen(m_curve1,QPen(blue,PENWIDTH));
	  m_plot->setCurvePen(m_curve2,QPen(red,PENWIDTH));

	  m_plot->enableLegend(true);
	  m_plot->setTitle("El Error and Velocity");
	  break;
	}	  
      m_plot->replot();
      m_data_set = dataset;
    }      
}

void GUIOscilloscopePane::togglePausePlotting()
{
  setPausePlotting(!m_halt_plot);
}

void GUIOscilloscopePane::toggleAutoscalePlot()
{
  switch(m_scale_mode)
    {
    case SM_AUTO:
      setAutoscalePlot(SM_FIXED);
      break;
    case SM_FIXED:
      setAutoscalePlot(SM_FIXED_EQUAL);
      break;
    case SM_FIXED_EQUAL:
      setAutoscalePlot(SM_AUTO);
      break;
    }
}

void GUIOscilloscopePane::toggleZoomPlot()
{
  setZoomPlot(!m_zoom);
}

void GUIOscilloscopePane::toggleCurve(long curve)
{
  setCurveOn(curve,!m_curve_on[curve]);
}

#endif // GUI_NO_QWT
