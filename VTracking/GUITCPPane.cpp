//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUITCPPane.cpp
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
 * $Date: 2007/07/19 04:35:12 $
 * $Revision: 2.7 $
 * $Tag$
 *
 **/

#include <iomanip>

#include<qpixmap.h>
#include<qimage.h>
#include<qpushbutton.h>
#include<qframe.h>
#include<qstring.h>
#include<qlayout.h>
#include<qfiledialog.h>
#include<qmessagebox.h>
#include<qvalidator.h>
#include<qtooltip.h>

#include"GUITCPPane.h"
#include"GUICorrectionDialogs.h"
#include"text.h"

#include"pixmaps/button1r.xpm"
#include"pixmaps/button2r.xpm"
#include"pixmaps/button3r.xpm"
#include"pixmaps/button1l.xpm"
#include"pixmaps/button2l.xpm"
#include"pixmaps/button3l.xpm"

using namespace SEphem;
using namespace VTracking;

// ----------------------------------------------------------------------------
//
// TRIPLE BUTTON WIDGET
//
// ----------------------------------------------------------------------------

TripleButtonWidget::TripleButtonWidget(double min, double max, 
				       double small_step, int points,
				       const QString& tooltip,
				       QWidget* parent, const char* name)
  : QHBox(parent,name), 
    m_le(), m_min(min), m_max(max), m_small_step(small_step), m_dp(points)
{
  static bool initialised;
  static QPixmap px_mmm;
  static QPixmap px_mm;
  static QPixmap px_m;
  static QPixmap px_p;
  static QPixmap px_pp;
  static QPixmap px_ppp;
  
  if(!initialised)
    {
      QImage im_orig_mmm(button3l);
      QImage im_small_mmm=im_orig_mmm.smoothScale(27,14,QImage::ScaleMin);
      px_mmm=im_small_mmm;

      QImage im_orig_mm(button2l);
      QImage im_small_mm=im_orig_mm.smoothScale(20,14,QImage::ScaleMin);
      px_mm=im_small_mm;

      QImage im_orig_m(button1l);
      QImage im_small_m=im_orig_m.smoothScale(12,14,QImage::ScaleMin);
      px_m=im_small_m;

      QImage im_orig_ppp(button3r);
      QImage im_small_ppp=im_orig_ppp.smoothScale(27,14,QImage::ScaleMin);
      px_ppp=im_small_ppp;

      QImage im_orig_pp(button2r);
      QImage im_small_pp=im_orig_pp.smoothScale(20,14,QImage::ScaleMin);
      px_pp=im_small_pp;

      QImage im_orig_p(button1r);
      QImage im_small_p=im_orig_p.smoothScale(12,14,QImage::ScaleMin);
      px_p=im_small_p;

      initialised=true;
    }

  QPushButton* mmm = new QPushButton(this, QString(name)+QString(" mmm"));
  mmm->setPixmap(px_mmm);

  QPushButton* mm = new QPushButton(this, QString(name)+QString(" mm"));
  mm->setPixmap(px_mm);

  QPushButton* m = new QPushButton(this, QString(name)+QString(" m"));
  m->setPixmap(px_m);

  QPushButton* p = new QPushButton(this, QString(name)+QString(" p"));
  p->setPixmap(px_p);
  
  QPushButton* pp = new QPushButton(this, QString(name)+QString(" pp"));
  pp->setPixmap(px_pp);
  
  QPushButton* ppp = new QPushButton(this, QString(name)+QString(" ppp"));
  ppp->setPixmap(px_ppp);
  
  QPushButton* z = new QPushButton("Zero", this, QString(name)+QString(" z"));
  
  m_le = new QLineEdit(this,QString(name)+QString(" le"));
  m_le->setValidator(new QDoubleValidator(min,max,points,m_le,
					  QString(name)+QString("val")));
  if(tooltip!="")QToolTip::add(m_le,tooltip);
  m_le->setText(QString::number((min+max)/2));
  m_le->setAlignment(Qt::AlignHCenter);
  
  ppp->setMinimumHeight(24);
  pp->setMinimumHeight(24);
  p->setMinimumHeight(24);
  m->setMinimumHeight(24);
  mm->setMinimumHeight(24);
  mmm->setMinimumHeight(24);
  z->setMinimumHeight(24);

  ppp->setMinimumWidth(35);
  pp->setMinimumWidth(35);
  p->setMinimumWidth(35);
  m->setMinimumWidth(35);
  mm->setMinimumWidth(35);
  mmm->setMinimumWidth(35);
  z->setMinimumWidth(45);

  ppp->setAutoRepeat(true);
  pp->setAutoRepeat(true);
  p->setAutoRepeat(true);
  m->setAutoRepeat(true);
  mm->setAutoRepeat(true);
  mmm->setAutoRepeat(true);
  
  connect(mmm,SIGNAL(clicked()),this,SLOT(mmmPushed()));
  connect(mm,SIGNAL(clicked()),this,SLOT(mmPushed()));
  connect(m,SIGNAL(clicked()),this,SLOT(mPushed()));
  connect(p,SIGNAL(clicked()),this,SLOT(pPushed()));
  connect(pp,SIGNAL(clicked()),this,SLOT(ppPushed()));
  connect(ppp,SIGNAL(clicked()),this,SLOT(pppPushed()));
  connect(z,SIGNAL(clicked()),this,SLOT(zeroPushed()));
  connect(m_le,SIGNAL(returnPressed()),this,SLOT(returnPushed()));
}

TripleButtonWidget::~TripleButtonWidget()
{
  // nothing to see here
}

void TripleButtonWidget::mmmPushed()
{
  delta(-100.0*m_small_step);
}

void TripleButtonWidget::mmPushed()
{
  delta(-10.0*m_small_step);
}

void TripleButtonWidget::mPushed()
{
  delta(-1.0*m_small_step);
}

void TripleButtonWidget::pPushed()
{
  delta(+1.0*m_small_step);
}

void TripleButtonWidget::ppPushed()
{
  delta(+10.0*m_small_step);
}

void TripleButtonWidget::pppPushed()
{
  delta(+100.0*m_small_step);
}

void TripleButtonWidget::zeroPushed()
{
  zeroValue();
  emit valueChanged(value());
}

void TripleButtonWidget::returnPushed()
{
  delta(0);
}

void TripleButtonWidget::delta(double d)
{
  bool valid;
  double value = m_le->text().toDouble(&valid);
  if(!valid)value = (m_min+m_max)/2.0;
  value+=d;
  if(value>m_max)value=m_max;
  else if(value<m_min)value=m_min;
  m_le->setText(QString::number(value,'f',m_dp));
  emit valueChanged(value);
}

void TripleButtonWidget::setValue(double value)
{
  m_le->setText(QString::number(value,'f',m_dp));
}

double TripleButtonWidget::value() const
{
  bool valid;
  double value = m_le->text().toDouble(&valid);
  if(!valid)value = (m_min+m_max)/2.0;
  if(value>m_max)value=m_max;
  else if(value<m_min)value=m_min;
  return value;
}

void TripleButtonWidget::zeroValue()
{
  setValue((m_min+m_max)/2);
}

// ----------------------------------------------------------------------------
//
// TELESCOPE CORRECTIONS PARAMETERS PANE
//
// ----------------------------------------------------------------------------

static void MAXIMIZE(int& max, int data)
{
  if(data>max)max=data;
}

GUITCPPane::
GUITCPPane(const CorrectionParameters& tcp,
	   unsigned scope_id,
	   QWidget* parent, const char* name) :
  QFrame(parent,name), GUITabPane(this),
  m_scope_id(scope_id),
  m_last_raw_az(),
  m_last_raw_el(),
  m_last_cor_az(),
  m_last_cor_el(),
  m_last_tar_az(),
  m_last_tar_el(),
  m_last_last_tar_az(),
  m_last_last_tar_el(),
  m_last_com_failure(),
  m_last_has_target(),
  m_last_last_has_target(),
  m_offsetsbox(),
  m_correctionsbox(),
  m_measurementbox(),
  m_vffbox(),
  m_tar_az(),
  m_tar_el(),
  m_raw_az(),
  m_raw_el(),
  m_cor_az(),
  m_cor_el(),
  m_del_az(),
  m_del_el(),
  m_az_offset(),
  m_el_offset(),
  m_az_ratio(),
  m_el_ratio(),
  m_az_ns(),
  m_az_ew(),
  m_el_udew(),
  m_fp_az(),
  m_flex_el_A(),
  m_flex_el_B(),
  m_el_tbw(),
  m_fp_tbw(),
  m_el_pos_vff_s(),
  m_el_pos_vff_i(),
  m_el_neg_vff_s(),
  m_el_neg_vff_i(),
  m_az_pos_vff_s(),
  m_az_pos_vff_i(),
  m_az_neg_vff_s(),
  m_az_neg_vff_i()
{
 QGridLayout* layout = new QGridLayout(this,1,7,5,5,"tcp layout");

  MyQGroupBox* locationbox = 
    new MyQGroupBox(8,Qt::Horizontal, "Telescope Location",this, "tcp loc");

  // ----------------
  // Position display
  // ----------------

  struct status_entry
  {
    QFrame* box;
    QLineEdit** le;
    QString label;
    QString tooltip;
  };

  struct status_entry statusentries[] =
  {
    { locationbox, &m_raw_az, "Raw Az", TT_SB_AZ },
    { locationbox, &m_cor_az, "  Corrected Az", TT_SUMDET_TEL_AZ },
    { locationbox, &m_tar_az, "  Target Az", TT_TCP_TAR_AZ },
    { locationbox, &m_del_az, "  Delta Az", TT_SUMDET_ERR_AZ },
    { locationbox, &m_raw_el, "Raw El", TT_SB_EL },
    { locationbox, &m_cor_el, "  Corrected El", TT_SUMDET_TEL_EL },
    { locationbox, &m_tar_el, "  Target El", TT_TCP_TAR_EL },
    { locationbox, &m_del_el, "  Delta El", TT_SUMDET_ERR_EL },
  };

  for(unsigned i=0; i<sizeof(statusentries)/sizeof(*statusentries); i++)
    {
      new QLabel(statusentries[i].label, statusentries[i].box,
		 QString("tcp ")+statusentries[i].label+QString(" lab"));

      *statusentries[i].le = 
	new InfoQLineEdit(statusentries[i].box,
		      QString("tcp ")+statusentries[i].label+QString(" le"));
      (*statusentries[i].le)->setEnabled(false);
      if(statusentries[i].tooltip != "")
	QToolTip::add(*statusentries[i].le,statusentries[i].tooltip);
    }

  // -------
  // Offsets
  // -------

  m_offsetsbox = new MyQGroupBox(1,Qt::Horizontal, "Offsets",
				 this, "tcp enables");

  m_offsetsbox->setCheckable(true);
  connect(m_offsetsbox,SIGNAL(toggled(bool)),this,SLOT(somethingHasChanged()));

  QFrame* offsetsinnerbox = new QFrame(m_offsetsbox);
  QGridLayout* offsetslayout = new QGridLayout(offsetsinnerbox,1,4,0,2);
  offsetslayout->setAutoAdd(true);

  // -----------
  // Corrections
  // -----------

  m_correctionsbox = new MyQGroupBox(1,Qt::Horizontal, "Corrections", 
				     this, "tcp parameters");

  m_correctionsbox->setCheckable(true);
  connect(m_correctionsbox,SIGNAL(toggled(bool)),
	  this,SLOT(somethingHasChanged()));

  QFrame* correctionsinnerbox = new QFrame(m_correctionsbox);
  QGridLayout* correctionslayout = 
    new QGridLayout(correctionsinnerbox,4,4,0,2);
  correctionslayout->setAutoAdd(true);
  
  // ---------------------
  // Velocity feed-forward
  // ---------------------

  m_vffbox = new MyQGroupBox(1,Qt::Horizontal, "Velocity Feed-Forward", 
			     this, "tcp vff parameters");

  m_vffbox->setCheckable(true);
  connect(m_vffbox,SIGNAL(toggled(bool)),this,SLOT(somethingHasChanged()));

  QFrame* vffinnerbox = new QFrame(m_vffbox);
  QGridLayout* vfflayout = new QGridLayout(vffinnerbox,4,4,0,2);
  vfflayout->setAutoAdd(true);  

  // ------------
  // Measurements
  // ------------

  m_measurementbox = 
    new MyQGroupBox(1,Qt::Horizontal, "Star Measurements", 
		    this, "tcp measurement parameters");

  QFrame* measurementinnerbox = new QFrame(m_measurementbox);
  QGridLayout* measurementlayout = 
    new QGridLayout(measurementinnerbox,2,1,0,0);
  measurementlayout->setAutoAdd(true);    

  // ----------------
  // LineEdit widgets
  // ----------------

  struct tcparamlineedits
  { 
    QFrame* grid;
    QGridLayout *layout;
    QLineEdit** le;
    QString label;
    QString tooltip;
    double min;
    double max;
    int dp;
  };
  
  struct tcparamlineedits lineedits[] =
    {
      { offsetsinnerbox, offsetslayout, &m_az_offset,
	MAKEDEG("Azimuth offset [")+QChar(']'), TT_TCP_AZ_OFF,
	-5.0, 5.0, 4 },
      { offsetsinnerbox, offsetslayout, &m_el_offset,
	MAKEDEG("  Elevation offset [")+QChar(']'), TT_TCP_EL_OFF,
	-5.0, 5.0, 4 },

      { correctionsinnerbox, correctionslayout, &m_az_ns,
	MAKEDEG("Az North-South inclination [")+QChar(']'), TT_TCP_AZ_NS,
	-5.0, 5.0, 4 },
      { correctionsinnerbox, correctionslayout, &m_el_udew, 
	MAKEDEG("  El axis mis-alignment [")+QChar(']'), TT_TCP_EL_UDEW,
	-5.0, 5.0, 4 },
      { correctionsinnerbox, correctionslayout, &m_az_ew,
	MAKEDEG("Az East-West inclination [")+QChar(']'), TT_TCP_AZ_EW,
	-5.0, 5.0, 4 },
      { correctionsinnerbox, correctionslayout, &m_flex_el_A,
	MAKEDEG(MAKEMULT("  Flexure "," cos(El) ["))+QChar(']'), 
	TT_TCP_EL_FLEX_A, 
	-5.0, 5.0, 4 },
      { correctionsinnerbox, correctionslayout, &m_fp_az,
	MAKEDEG("Collimation mis-alignment [")+QChar(']'), TT_TCP_FP_COLL,
	-5.0, 5.0, 4 },
      { correctionsinnerbox, correctionslayout, &m_flex_el_B,
	MAKEDEG(MAKEMULT("  Flexure "," sin(2 El) ["))+QChar(']'), 
	TT_TCP_EL_FLEX_B,
	-5.0, 5.0, 4 },
      { correctionsinnerbox, correctionslayout, &m_az_ratio,
	"Az encoder ratio [1]", TT_TCP_AZ_ENC,
	0.98, 1.02, 4 },
      { correctionsinnerbox, correctionslayout, &m_el_ratio,
	"  El encoder ratio [1]", TT_TCP_EL_ENC, 
	0.98, 1.02, 4 },

      { vffinnerbox, vfflayout, &m_az_pos_vff_s, 
	"Az positive VFF slope [s]", TT_TCP_AZ_POS_VFF_S,
	-10.00, 10.00, 4 },
      { vffinnerbox, vfflayout, &m_el_pos_vff_s,
	"  El positive VFF slope [s]", TT_TCP_EL_POS_VFF_S, 
	-10.00, 10.00, 4 },
      { vffinnerbox, vfflayout, &m_az_pos_vff_i,
	MAKEDPS("Az positive VFF threshold [")+QChar(']'), 
	TT_TCP_AZ_POS_VFF_I,
	-0.1000, 0.1000, 5 },
      { vffinnerbox, vfflayout, &m_el_pos_vff_i,
	MAKEDPS("  El positive VFF threshold [")+QChar(']'), 
	TT_TCP_EL_POS_VFF_I,
	-0.1000, 0.1000, 5 },
      { vffinnerbox, vfflayout, &m_az_neg_vff_s,
	"Az negative VFF slope [s]", TT_TCP_AZ_NEG_VFF_S,
	-10.00, 10.00, 4 },
      { vffinnerbox, vfflayout, &m_el_neg_vff_s,
	"  El negative VFF slope [s]", TT_TCP_EL_NEG_VFF_S,
	-10.00, 10.00, 4 },
      { vffinnerbox, vfflayout, &m_az_neg_vff_i,
	MAKEDPS("Az negative VFF threshold [")+QChar(']'), 
	TT_TCP_AZ_NEG_VFF_I,
	-0.1000, 0.1000, 5 },
      { vffinnerbox, vfflayout, &m_el_neg_vff_i,
	MAKEDPS("  El negative VFF threshold [")+QChar(']'),
	TT_TCP_EL_NEG_VFF_I,
	-0.1000, 0.1000, 5 }
    };

  for(unsigned i=0; i<sizeof(lineedits)/sizeof(*lineedits); i++)
    {
      double min = lineedits[i].min;
      double max = lineedits[i].max;
      int dp = lineedits[i].dp;

      QLabel* lab = 
	new QLabel(lineedits[i].label, lineedits[i].grid,
		   QString("tcp ")+lineedits[i].label+QString(" lab"));

      QLineEdit* le = 
	new QLineEdit(lineedits[i].grid, 
		      QString("tcp ")+lineedits[i].label+QString(" le"));

      QValidator* val = 
	new QDoubleValidator(min, max, dp, le, 
			     QString("tcp ")+lineedits[i].label+
			     QString(" val"));

      le->setValidator(val);
      if(lineedits[i].tooltip!="")
	{
	  QToolTip::add(le,lineedits[i].tooltip);
	  QToolTip::add(lab,lineedits[i].tooltip);
	}
      le->setText(QString::number((min+max)/2,'f',dp));
      le->setAlignment(Qt::AlignHCenter);

      connect(le,SIGNAL(returnPressed()),this,SLOT(somethingHasChanged()));

      *lineedits[i].le = le;
    }

  offsetslayout->setColStretch(0,1); offsetslayout->setColStretch(2,1);
  offsetslayout->setColStretch(1,0); offsetslayout->setColStretch(3,0);
  correctionslayout->setColStretch(0,1); correctionslayout->setColStretch(2,1);
  correctionslayout->setColStretch(1,0); correctionslayout->setColStretch(3,0);
  vfflayout->setColStretch(0,1); vfflayout->setColStretch(2,1);
  vfflayout->setColStretch(1,0); vfflayout->setColStretch(3,0);
  measurementlayout->setColStretch(0,1);
  measurementlayout->setColStretch(1,0);

  int colsize=offsetslayout->colSpacing(0);
  MAXIMIZE(colsize,offsetslayout->colSpacing(2));
  MAXIMIZE(colsize,correctionslayout->colSpacing(0));
  MAXIMIZE(colsize,correctionslayout->colSpacing(2));
  MAXIMIZE(colsize,vfflayout->colSpacing(0));
  MAXIMIZE(colsize,vfflayout->colSpacing(2));
  colsize=colsize*105/100;

  offsetslayout->setColSpacing(0,colsize);
  offsetslayout->setColSpacing(2,colsize);
  correctionslayout->setColSpacing(0,colsize);
  correctionslayout->setColSpacing(2,colsize);
  vfflayout->setColSpacing(0,colsize);
  vfflayout->setColSpacing(2,colsize);

  // ---------------------
  // Triple button widgets
  // ---------------------

  struct tcparamwidgets
  { 
    QFrame* grid;
    QGridLayout *layout;
    TripleButtonWidget** tbw;
    QString label;
    QString tooltip;
    double min;
    double max;
    double small_step;
    int dp;
  };

  struct tcparamwidgets widgets[] =
    {
      { measurementinnerbox, measurementlayout, &m_el_tbw,
        MAKEDEG("Elevation offset [")+QChar(']'), TT_TCP_EL_OFF,
        -5.0, 5.0, 0.001, 4 },
      { measurementinnerbox, measurementlayout, &m_fp_tbw,
        MAKEDEG("Collimation mis-alignment [")+QChar(']'), TT_TCP_FP_COLL,
        -5.0, 5.0, 0.001, 4 }
    };

  for(unsigned i=0; i<sizeof(widgets)/sizeof(*widgets); i++)
    {
      new QLabel(widgets[i].label, widgets[i].grid,
                 QString("tcp ")+widgets[i].label+QString(" lab"));

      *widgets[i].tbw =
        new TripleButtonWidget(widgets[i].min, widgets[i].max,
                               widgets[i].small_step, widgets[i].dp,
                               widgets[i].tooltip, widgets[i].grid,
                          QString("tcp ")+widgets[i].label+QString(" tbw"));

      connect(*widgets[i].tbw,SIGNAL(valueChanged(double)),
              this, SLOT(tbwHasChanged()));
    }


  // -----------
  // Actions box
  // -----------

  MyQGroupBox* actionsbox = 
    new MyQGroupBox(5,Qt::Horizontal, "Actions",this, "tcp actions box");

  QPushButton* zero_all_pb = new QPushButton("Zero All",actionsbox,
					     "tcp zero all pb");
  QPushButton* zero_align_pb = new QPushButton("Zero Alignments",actionsbox,
					       "tcp zero align pb");
  QPushButton* load_pb = new QPushButton("Load",actionsbox,"tcp load pb");
  QPushButton* save_pb = new QPushButton("Save",actionsbox,"tcp save pb");
  QPushButton* record_pb = new QPushButton("Record Location",actionsbox,
					   "tcp record pb");
  
  connect(zero_all_pb,SIGNAL(clicked()),this,SLOT(zeroAll()));
  connect(zero_align_pb,SIGNAL(clicked()),this,SLOT(zeroAlignments()));
  connect(load_pb,SIGNAL(clicked()),this,SLOT(load()));
  connect(save_pb,SIGNAL(clicked()),this,SLOT(save()));
  connect(record_pb,SIGNAL(clicked()),this,SLOT(recordButtonPressed()));

  layout->addWidget(locationbox,0,0);
  layout->addWidget(m_offsetsbox,1,0);
  layout->addWidget(m_correctionsbox,2,0);
  layout->addWidget(m_vffbox,3,0);
  layout->addWidget(m_measurementbox,4,0);
  layout->addWidget(actionsbox,6,0);

  layout->setRowStretch(0,0);
  layout->setRowStretch(1,0);
  layout->setRowStretch(2,0);
  layout->setRowStretch(3,0);
  layout->setRowStretch(4,0);
  layout->setRowStretch(5,1);
  layout->setRowStretch(6,0);

  setParameters(tcp);
}

GUITCPPane::~GUITCPPane()
{
  // nothing to see here
}

void GUITCPPane::
setParameters(const CorrectionParameters& tcp)
{
  m_offsetsbox->setChecked(tcp.enable_offsets);
  m_correctionsbox->setChecked(tcp.enable_corrections);
  m_vffbox->setChecked(tcp.enable_vff);

  m_az_offset->setText(QString::number(Angle::toDeg(tcp.az_offset),'f',4));
  m_el_offset->setText(QString::number(Angle::toDeg(tcp.el_offset),'f',4));
  m_az_ratio->setText(QString::number(tcp.az_ratio,'f',4));
  m_el_ratio->setText(QString::number(tcp.el_ratio,'f',4));
  m_az_ns->setText(QString::number(Angle::toDeg(tcp.az_ns),'f',4));
  m_az_ew->setText(QString::number(Angle::toDeg(tcp.az_ew),'f',4));
  m_el_udew->setText(QString::number(Angle::toDeg(tcp.el_udew),'f',4));
  m_fp_az->setText(QString::number(Angle::toDeg(tcp.fp_az),'f',4));
  m_flex_el_A->setText(QString::number(Angle::toDeg(tcp.flex_el_A),'f',4));
  m_flex_el_B->setText(QString::number(Angle::toDeg(tcp.flex_el_B),'f',4));

  m_el_tbw->setValue(Angle::toDeg(tcp.el_offset));
  m_fp_tbw->setValue(Angle::toDeg(tcp.fp_az));

  m_el_pos_vff_s->setText(QString::number(tcp.el_pos_vff_s,'f',4));
  m_el_pos_vff_i->setText(QString::number(Angle::toDeg(tcp.el_pos_vff_t),'f',5));
  m_el_neg_vff_s->setText(QString::number(tcp.el_neg_vff_s,'f',4));
  m_el_neg_vff_i->setText(QString::number(Angle::toDeg(tcp.el_neg_vff_t),'f',5));
  m_az_pos_vff_s->setText(QString::number(tcp.az_pos_vff_s,'f',4));
  m_az_pos_vff_i->setText(QString::number(Angle::toDeg(tcp.az_pos_vff_t),'f',5));
  m_az_neg_vff_s->setText(QString::number(tcp.az_neg_vff_s,'f',4));
  m_az_neg_vff_i->setText(QString::number(Angle::toDeg(tcp.az_neg_vff_t),'f',5));

  bool e1=m_offsetsbox->isChecked();
  bool e2=e1&&m_correctionsbox->isChecked();
  bool e3=/*e1&&*/m_vffbox->isChecked();

  m_correctionsbox->setEnabled(e1);
  //m_vffbox->setEnabled(e1);
  m_az_offset->setEnabled(e1);
  m_el_offset->setEnabled(e1);
  m_az_ratio->setEnabled(e2);
  m_el_ratio->setEnabled(e2);
  m_az_ns->setEnabled(e2);
  m_az_ew->setEnabled(e2);
  m_el_udew->setEnabled(e2);
  m_fp_az->setEnabled(e2);
  m_flex_el_A->setEnabled(e2);
  m_flex_el_B->setEnabled(e2);

  m_el_pos_vff_s->setEnabled(e3);
  m_el_pos_vff_i->setEnabled(e3);
  m_el_neg_vff_s->setEnabled(e3);
  m_el_neg_vff_i->setEnabled(e3);
  m_az_pos_vff_s->setEnabled(e3);
  m_az_pos_vff_i->setEnabled(e3);
  m_az_neg_vff_s->setEnabled(e3);
  m_az_neg_vff_i->setEnabled(e3);

  m_measurementbox->setEnabled(e2);
  m_el_tbw->setEnabled(e2);
  m_fp_tbw->setEnabled(e2);
}

void GUITCPPane::tbwHasChanged()
{
  m_el_offset->setText(QString::number(m_el_tbw->value(),'f',4));
  m_fp_az->setText(QString::number(m_fp_tbw->value(),'f',4));
  emit parametersChanged();
}

void GUITCPPane::somethingHasChanged()
{
  bool e1=m_offsetsbox->isChecked(); 
  bool e2=e1&&m_correctionsbox->isChecked();
  bool e3=/*e1&&*/m_vffbox->isChecked();
  m_correctionsbox->setEnabled(e1);
  //m_vffbox->setEnabled(e1);
  m_az_offset->setEnabled(e1);
  m_el_offset->setEnabled(e1);
  m_az_ratio->setEnabled(e2);
  m_el_ratio->setEnabled(e2);
  m_az_ns->setEnabled(e2);
  m_az_ew->setEnabled(e2);
  m_el_udew->setEnabled(e2);
  m_fp_az->setEnabled(e2);
  m_flex_el_A->setEnabled(e2);
  m_flex_el_B->setEnabled(e2);
  m_el_pos_vff_s->setEnabled(e3);
  m_el_pos_vff_i->setEnabled(e3);
  m_el_neg_vff_s->setEnabled(e3);
  m_el_neg_vff_i->setEnabled(e3);
  m_az_pos_vff_s->setEnabled(e3);
  m_az_pos_vff_i->setEnabled(e3);
  m_az_neg_vff_s->setEnabled(e3);
  m_az_neg_vff_i->setEnabled(e3);
  m_measurementbox->setEnabled(e2);
  m_el_tbw->setEnabled(e2);
  m_fp_tbw->setEnabled(e2);
  bool valid;
  m_el_tbw->setValue(m_el_offset->text().toDouble(&valid));
  m_fp_tbw->setValue(m_fp_az->text().toDouble(&valid));
  emit parametersChanged();
}

CorrectionParameters 
GUITCPPane::getParameters() const
{
  CorrectionParameters tcp;
  bool valid;
  bool e1=m_offsetsbox->isChecked();
  bool e2=e1&&m_correctionsbox->isChecked();
  bool e3=/*e1&&*/m_vffbox->isChecked();
  tcp.enable_offsets=e1;
  tcp.enable_corrections=e2;
  tcp.enable_vff=e3;
  tcp.az_ratio=m_az_ratio->text().toDouble(&valid);
  tcp.el_ratio=m_el_ratio->text().toDouble(&valid);
  tcp.az_offset=Angle::frDeg(m_az_offset->text().toDouble(&valid));
  tcp.el_offset=Angle::frDeg(m_el_offset->text().toDouble(&valid));
  tcp.az_ns=Angle::frDeg(m_az_ns->text().toDouble(&valid));
  tcp.az_ew=Angle::frDeg(m_az_ew->text().toDouble(&valid));
  tcp.el_udew=Angle::frDeg(m_el_udew->text().toDouble(&valid));
  tcp.fp_az=Angle::frDeg(m_fp_az->text().toDouble(&valid));
  tcp.flex_el_A=Angle::frDeg(m_flex_el_A->text().toDouble(&valid));
  tcp.flex_el_B=Angle::frDeg(m_flex_el_B->text().toDouble(&valid));
  tcp.el_pos_vff_s=m_el_pos_vff_s->text().toDouble(&valid);
  tcp.el_pos_vff_t=Angle::frDeg(m_el_pos_vff_i->text().toDouble(&valid));
  tcp.el_neg_vff_s=m_el_neg_vff_s->text().toDouble(&valid);
  tcp.el_neg_vff_t=Angle::frDeg(m_el_neg_vff_i->text().toDouble(&valid));
  tcp.az_pos_vff_s=m_az_pos_vff_s->text().toDouble(&valid);
  tcp.az_pos_vff_t=Angle::frDeg(m_az_pos_vff_i->text().toDouble(&valid));
  tcp.az_neg_vff_s=m_az_neg_vff_s->text().toDouble(&valid);
  tcp.az_neg_vff_t=Angle::frDeg(m_az_neg_vff_i->text().toDouble(&valid));
  m_measurementbox->setEnabled(e2);
  m_el_tbw->setEnabled(e2);
  m_fp_tbw->setEnabled(e2);
  return tcp;
}

void GUITCPPane::save()
{
#if 0
  std::string default_fn = 
    CorrectionParameters::saveFilename(m_scope_id);

  QString fn = 
    QFileDialog::getSaveFileName(default_fn, "Data Files (*.dat)",
				 this, "save dialog", 
				 "Select Corrections File");
  if (!fn.isEmpty())
    {
      CorrectionParameters tcp = getParameters();
      if(!tcp.save(fn))
	QMessageBox::warning(0,"Save Corrections",
			     "Could not save tracking corrections",
			     QMessageBox::Ok,QMessageBox::NoButton);
    }
#else
  CorrectionParameters tcp = getParameters();
  if (GUICorrectionDialogs::save(tcp, m_scope_id, this, "save dialog") 
      == GUICorrectionDialogs::S_FAIL)
    {
      QMessageBox::warning(0,"Save Corrections",
			   "Could not save tracking corrections",
			   QMessageBox::Ok,QMessageBox::NoButton);
    }
#endif
}

void GUITCPPane::load()
{
#if 0
  std::string default_fn = 
    CorrectionParameters::loadFilename(m_scope_id);

  QString fn = 
    QFileDialog::getOpenFileName(default_fn, "Data Files (*.dat)",
				 this, "load dialog", 
				 "Select Corrections File");
  if (!fn.isEmpty())
    {
      CorrectionParameters tcp;
      if(tcp.load(fn))
	{
	  setParameters(tcp);
	  emit parametersChanged();
	}
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
      setParameters(tcp);
      emit parametersChanged();
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

void GUITCPPane::zeroAlignments()
{
  m_az_ratio->setText(QString::number(1.0,'f',4));
  m_el_ratio->setText(QString::number(1.0,'f',4));
  m_az_ns->setText(QString::number(0.0,'f',4));
  m_az_ew->setText(QString::number(0.0,'f',4));
  m_el_udew->setText(QString::number(0.0,'f',4));
  m_fp_az->setText(QString::number(0.0,'f',4));
  m_fp_tbw->setValue(0.0);
  m_flex_el_A->setText(QString::number(0.0,'f',4));
  m_flex_el_B->setText(QString::number(0.0,'f',4));
  emit parametersChanged();
}

void GUITCPPane::zeroAll()
{
  m_az_offset->setText(QString::number(0.0,'f',4));
  m_el_offset->setText(QString::number(0.0,'f',4));
  m_el_tbw->setValue(0.0);
  m_el_pos_vff_s->setText(QString::number(0.0,'f',4));
  m_el_pos_vff_i->setText(QString::number(0.0,'f',5));
  m_el_neg_vff_s->setText(QString::number(0.0,'f',4));
  m_el_neg_vff_i->setText(QString::number(0.0,'f',5));
  m_az_pos_vff_s->setText(QString::number(0.0,'f',4));
  m_az_pos_vff_i->setText(QString::number(0.0,'f',5));
  m_az_neg_vff_s->setText(QString::number(0.0,'f',4));
  m_az_neg_vff_i->setText(QString::number(0.0,'f',5));
  zeroAlignments();
}

void GUITCPPane::recordButtonPressed()
{
  emit recordPosition(m_last_raw_az, m_last_raw_el,
		      m_last_cor_az, m_last_cor_el);
}

void GUITCPPane::
update(const GUIUpdateData& ud)
{
  if(ud.tse.state != TelescopeController::TS_COM_FAILURE)
    {
      std::ostringstream azstream;
      azstream << std::fixed << std::showpos << std::setprecision(4)
	       << ud.tse.status.az.driveangle_deg;
      
      std::ostringstream elstream;
      elstream << std::fixed << std::showpos << std::setprecision(4)
	       << ud.tse.status.el.driveangle_deg;

      m_raw_az->setText(MAKEDEG(azstream.str().c_str()));
      m_raw_el->setText(MAKEDEG(elstream.str().c_str()));
      m_cor_az->setText(MAKEDEG(ud.tse.tel_azel.phi().degString(4)));
      m_cor_el->setText(MAKEDEG(ud.tse.tel_azel.latitude().degPM180String(4)));
      m_raw_az->setEnabled(true);
      m_raw_el->setEnabled(true);
      m_cor_az->setEnabled(true);
      m_cor_el->setEnabled(true);

      m_last_raw_az = Angle::frDeg(ud.tse.status.az.driveangle_deg);
      m_last_raw_el = Angle::frDeg(ud.tse.status.el.driveangle_deg);
      m_last_cor_az = ud.tse.tel_azel.phiRad();
      m_last_cor_el = ud.tse.tel_azel.latitudeRad();

      if(m_last_has_target)
	{
	  Angle del_az = ud.tse.tel_azel.phi() - m_last_tar_az;
	  Angle del_el = ud.tse.tel_azel.latitude() - m_last_tar_el;
	  m_del_az->setText(MAKEDEG(del_az.degPMString(4)));
	  m_del_el->setText(MAKEDEG(del_el.degPMString(4)));
	  m_del_az->setEnabled(true);
	  m_del_el->setEnabled(true);
	}
      else
	{
	  m_del_az->setText("");
	  m_del_el->setText("");
	  m_del_az->setEnabled(false);
	  m_del_el->setEnabled(false);
	}
    }
  else
    {
      m_raw_az->setText("");
      m_raw_el->setText("");
      m_cor_az->setText("");
      m_cor_el->setText("");
      m_raw_az->setEnabled(false);
      m_raw_el->setEnabled(false);
      m_cor_az->setEnabled(false);
      m_cor_el->setEnabled(false);
      m_del_az->setText("");
      m_del_el->setText("");
      m_del_az->setEnabled(false);
      m_del_el->setEnabled(false);
    }

  if(m_last_has_target)
    {
      m_tar_az->setText(MAKEDEG(m_last_tar_az.degString(4)));
      m_tar_el->setText(MAKEDEG(m_last_tar_el.degPM180String(4)));
      m_tar_az->setEnabled(true);
      m_tar_el->setEnabled(true);
    }
  else
    {
      m_tar_az->setText("");
      m_tar_el->setText("");
      m_tar_az->setEnabled(false);
      m_tar_el->setEnabled(false);
    }

  m_last_com_failure = (ud.tse.state == TelescopeController::TS_COM_FAILURE);

  m_last_last_tar_az = m_last_tar_az;
  m_last_last_tar_el = m_last_tar_el;
  m_last_last_has_target = m_last_has_target;
  
  m_last_tar_az = ud.tse.obj_azel.phi();
  m_last_tar_el = ud.tse.obj_azel.latitude();
  m_last_has_target = ud.tse.has_object;
}

