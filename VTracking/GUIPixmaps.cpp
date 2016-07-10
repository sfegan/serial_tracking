//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIPixmaps.cpp
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
 * $Date: 2007/02/04 19:01:55 $
 * $Revision: 2.2 $
 * $Tag$
 *
 **/

#include<iostream> 

#include<qimage.h>
#include<qbitmap.h>

#include"GUIMisc.h"
#include"GUIPixmaps.h"

#include"pixmaps/go01_pix_data.xpm"
#include"pixmaps/go02_pix_data.xpm"
#include"pixmaps/go03_pix_data.xpm"
#include"pixmaps/go04_pix_data.xpm"
#include"pixmaps/go05_pix_data.xpm"
#include"pixmaps/go06_pix_data.xpm"
#include"pixmaps/go07_pix_data.xpm"
#include"pixmaps/go08_pix_data.xpm"
#include"pixmaps/go09_pix_data.xpm"
#include"pixmaps/go10_pix_data.xpm"
#include"pixmaps/go11_pix_data.xpm"
#include"pixmaps/go12_pix_data.xpm"
#include"pixmaps/go13_pix_data.xpm"
#include"pixmaps/go14_pix_data.xpm"
#include"pixmaps/go15_pix_data.xpm"
#include"pixmaps/go16_pix_data.xpm"
#include"pixmaps/go17_pix_data.xpm"
#include"pixmaps/go18_pix_data.xpm"
#include"pixmaps/go19_pix_data.xpm"
#include"pixmaps/go20_pix_data.xpm"
#include"pixmaps/go21_pix_data.xpm"
#include"pixmaps/go22_pix_data.xpm"
#include"pixmaps/go23_pix_data.xpm"
#include"pixmaps/go24_pix_data.xpm"

#include"pixmaps/stop_pix_data.xpm"

#include"pixmaps/ind_cf01_pix_data.xpm"
#include"pixmaps/ind_cf02_pix_data.xpm"
#include"pixmaps/ind_cf03_pix_data.xpm"
#include"pixmaps/ind_cf04_pix_data.xpm"
#include"pixmaps/ind_cf05_pix_data.xpm"
#include"pixmaps/ind_cf06_pix_data.xpm"
#include"pixmaps/ind_cf07_pix_data.xpm"
#include"pixmaps/ind_cf08_pix_data.xpm"
#include"pixmaps/ind_cf09_pix_data.xpm"
#include"pixmaps/ind_cf10_pix_data.xpm"
#include"pixmaps/ind_cf11_pix_data.xpm"
#include"pixmaps/ind_cf12_pix_data.xpm"
#include"pixmaps/ind_cf13_pix_data.xpm"
#include"pixmaps/ind_cf14_pix_data.xpm"
#include"pixmaps/ind_cf15_pix_data.xpm"
#include"pixmaps/ind_cf16_pix_data.xpm"

#include"pixmaps/ind_go01_pix_data.xpm"
#include"pixmaps/ind_go02_pix_data.xpm"
#include"pixmaps/ind_go03_pix_data.xpm"
#include"pixmaps/ind_go04_pix_data.xpm"
#include"pixmaps/ind_go05_pix_data.xpm"
#include"pixmaps/ind_go06_pix_data.xpm"
#include"pixmaps/ind_go07_pix_data.xpm"
#include"pixmaps/ind_go08_pix_data.xpm"
#include"pixmaps/ind_go09_pix_data.xpm"
#include"pixmaps/ind_go10_pix_data.xpm"
#include"pixmaps/ind_go11_pix_data.xpm"
#include"pixmaps/ind_go12_pix_data.xpm"
#include"pixmaps/ind_go13_pix_data.xpm"
#include"pixmaps/ind_go14_pix_data.xpm"
#include"pixmaps/ind_go15_pix_data.xpm"
#include"pixmaps/ind_go16_pix_data.xpm"
#include"pixmaps/ind_go17_pix_data.xpm"
#include"pixmaps/ind_go18_pix_data.xpm"
#include"pixmaps/ind_go19_pix_data.xpm"
#include"pixmaps/ind_go20_pix_data.xpm"
#include"pixmaps/ind_go21_pix_data.xpm"
#include"pixmaps/ind_go22_pix_data.xpm"
#include"pixmaps/ind_go23_pix_data.xpm"
#include"pixmaps/ind_go24_pix_data.xpm"
#include"pixmaps/ind_go25_pix_data.xpm"
#include"pixmaps/ind_go26_pix_data.xpm"
#include"pixmaps/ind_go27_pix_data.xpm"
#include"pixmaps/ind_go28_pix_data.xpm"
#include"pixmaps/ind_go29_pix_data.xpm"
#include"pixmaps/ind_go30_pix_data.xpm"
#include"pixmaps/ind_go31_pix_data.xpm"
#include"pixmaps/ind_go32_pix_data.xpm"
#include"pixmaps/ind_go33_pix_data.xpm"
#include"pixmaps/ind_go34_pix_data.xpm"
#include"pixmaps/ind_go35_pix_data.xpm"
#include"pixmaps/ind_go36_pix_data.xpm"

#include"pixmaps/ind_stop_pix_data.xpm"

#include"pixmaps/sw01_pix_data.xpm"
#include"pixmaps/sw02_pix_data.xpm"
#include"pixmaps/sw03_pix_data.xpm"
#include"pixmaps/sw04_pix_data.xpm"
#include"pixmaps/sw05_pix_data.xpm"
#include"pixmaps/sw06_pix_data.xpm"
#include"pixmaps/sw07_pix_data.xpm"
#include"pixmaps/sw08_pix_data.xpm"
#include"pixmaps/sw09_pix_data.xpm"
#include"pixmaps/sw10_pix_data.xpm"
#include"pixmaps/sw11_pix_data.xpm"
#include"pixmaps/sw12_pix_data.xpm"
#include"pixmaps/sw13_pix_data.xpm"
#include"pixmaps/sw14_pix_data.xpm"
#include"pixmaps/sw15_pix_data.xpm"
#include"pixmaps/sw16_pix_data.xpm"
#include"pixmaps/sw17_pix_data.xpm"
#include"pixmaps/sw18_pix_data.xpm"
#include"pixmaps/sw19_pix_data.xpm"
#include"pixmaps/sw20_pix_data.xpm"
#include"pixmaps/sw21_pix_data.xpm"
#include"pixmaps/sw22_pix_data.xpm"
#include"pixmaps/sw23_pix_data.xpm"
#include"pixmaps/sw24_pix_data.xpm"
#include"pixmaps/sw25_pix_data.xpm"
#include"pixmaps/sw26_pix_data.xpm"
#include"pixmaps/sw27_pix_data.xpm"
#include"pixmaps/sw28_pix_data.xpm"
#include"pixmaps/sw29_pix_data.xpm"
#include"pixmaps/sw30_pix_data.xpm"
#include"pixmaps/sw31_pix_data.xpm"
#include"pixmaps/sw32_pix_data.xpm"
#include"pixmaps/sw33_pix_data.xpm"
#include"pixmaps/sw34_pix_data.xpm"
#include"pixmaps/sw35_pix_data.xpm"
#include"pixmaps/sw36_pix_data.xpm"
#include"pixmaps/sw37_pix_data.xpm"
#include"pixmaps/sw38_pix_data.xpm"
#include"pixmaps/sw39_pix_data.xpm"
#include"pixmaps/sw40_pix_data.xpm"
#include"pixmaps/sw41_pix_data.xpm"
#include"pixmaps/sw42_pix_data.xpm"
#include"pixmaps/sw43_pix_data.xpm"
#include"pixmaps/sw44_pix_data.xpm"
#include"pixmaps/sw45_pix_data.xpm"
#include"pixmaps/sw46_pix_data.xpm"
#include"pixmaps/sw47_pix_data.xpm"
#include"pixmaps/sw48_pix_data.xpm"
#include"pixmaps/sw49_pix_data.xpm"
#include"pixmaps/sw50_pix_data.xpm"
#include"pixmaps/sw51_pix_data.xpm"
#include"pixmaps/sw52_pix_data.xpm"
#include"pixmaps/sw53_pix_data.xpm"
#include"pixmaps/sw54_pix_data.xpm"
#include"pixmaps/sw55_pix_data.xpm"
#include"pixmaps/sw56_pix_data.xpm"
#include"pixmaps/sw57_pix_data.xpm"
#include"pixmaps/sw58_pix_data.xpm"
#include"pixmaps/sw59_pix_data.xpm"
#include"pixmaps/sw60_pix_data.xpm"
#include"pixmaps/sw61_pix_data.xpm"
#include"pixmaps/sw62_pix_data.xpm"
#include"pixmaps/sw63_pix_data.xpm"
#include"pixmaps/sw64_pix_data.xpm"
#include"pixmaps/sw65_pix_data.xpm"
#include"pixmaps/sw66_pix_data.xpm"
#include"pixmaps/sw67_pix_data.xpm"
#include"pixmaps/sw68_pix_data.xpm"
#include"pixmaps/sw69_pix_data.xpm"
#include"pixmaps/sw70_pix_data.xpm"
#include"pixmaps/sw71_pix_data.xpm"
#include"pixmaps/sw72_pix_data.xpm"

static char** go_xpms[24] = { go01_pix_data, go02_pix_data, go03_pix_data,
			      go04_pix_data, go05_pix_data, go06_pix_data,
			      go07_pix_data, go08_pix_data, go09_pix_data,
			      go10_pix_data, go11_pix_data, go12_pix_data,
			      go13_pix_data, go14_pix_data, go15_pix_data,
			      go16_pix_data, go17_pix_data, go18_pix_data,
			      go19_pix_data, go20_pix_data, go21_pix_data,
			      go22_pix_data, go23_pix_data, go24_pix_data };

static char** stop_xpms[1] = { stop_pix_data };

static char** ind_go_xpms[36] = { ind_go01_pix_data, ind_go02_pix_data, 
				  ind_go03_pix_data, ind_go04_pix_data, 
				  ind_go05_pix_data, ind_go06_pix_data, 
				  ind_go07_pix_data, ind_go08_pix_data, 
				  ind_go09_pix_data, ind_go10_pix_data, 
				  ind_go11_pix_data, ind_go12_pix_data, 
				  ind_go13_pix_data, ind_go14_pix_data, 
				  ind_go15_pix_data, ind_go16_pix_data, 
				  ind_go17_pix_data, ind_go18_pix_data, 
				  ind_go19_pix_data, ind_go20_pix_data, 
				  ind_go21_pix_data, ind_go22_pix_data, 
				  ind_go23_pix_data, ind_go24_pix_data, 
				  ind_go25_pix_data, ind_go26_pix_data, 
				  ind_go27_pix_data, ind_go28_pix_data, 
				  ind_go29_pix_data, ind_go30_pix_data, 
				  ind_go31_pix_data, ind_go32_pix_data, 
				  ind_go33_pix_data, ind_go34_pix_data, 
				  ind_go35_pix_data, ind_go36_pix_data };

static char** ind_cf_xpms[16] = { ind_cf01_pix_data, ind_cf02_pix_data, 
				  ind_cf03_pix_data, ind_cf04_pix_data, 
				  ind_cf05_pix_data, ind_cf06_pix_data, 
				  ind_cf07_pix_data, ind_cf08_pix_data, 
				  ind_cf09_pix_data, ind_cf10_pix_data, 
				  ind_cf11_pix_data, ind_cf12_pix_data, 
				  ind_cf13_pix_data, ind_cf14_pix_data, 
				  ind_cf15_pix_data, ind_cf16_pix_data };

static char** ind_stop_xpms[1] = { ind_stop_pix_data };

static char** sw_xpms[72] = { 
  sw01_pix_data, sw02_pix_data, sw03_pix_data, sw04_pix_data, 
  sw05_pix_data, sw06_pix_data, sw07_pix_data, sw08_pix_data, 
  sw09_pix_data, sw10_pix_data, sw11_pix_data, sw12_pix_data, 
  sw13_pix_data, sw14_pix_data, sw15_pix_data, sw16_pix_data, 
  sw17_pix_data, sw18_pix_data, sw19_pix_data, sw20_pix_data, 
  sw21_pix_data, sw22_pix_data, sw23_pix_data, sw24_pix_data, 
  sw25_pix_data, sw26_pix_data, sw27_pix_data, sw28_pix_data, 
  sw29_pix_data, sw30_pix_data, sw31_pix_data, sw32_pix_data, 
  sw33_pix_data, sw34_pix_data, sw35_pix_data, sw36_pix_data, 
  sw37_pix_data, sw38_pix_data, sw39_pix_data, sw40_pix_data, 
  sw41_pix_data, sw42_pix_data, sw43_pix_data, sw44_pix_data, 
  sw45_pix_data, sw46_pix_data, sw47_pix_data, sw48_pix_data, 
  sw49_pix_data, sw50_pix_data, sw51_pix_data, sw52_pix_data, 
  sw53_pix_data, sw54_pix_data, sw55_pix_data, sw56_pix_data, 
  sw57_pix_data, sw58_pix_data, sw59_pix_data, sw60_pix_data, 
  sw61_pix_data, sw62_pix_data, sw63_pix_data, sw64_pix_data, 
  sw65_pix_data, sw66_pix_data, sw67_pix_data, sw68_pix_data, 
  sw69_pix_data, sw70_pix_data, sw71_pix_data, sw72_pix_data };

std::auto_ptr<GUIPixmaps> GUIPixmaps::s_instance;

#define N(x) (sizeof(x)/sizeof(*x))

GUIPixmaps::GUIPixmaps(): 
  m_gostop_size(0), m_target_size(0), m_ind_size(0), m_sw_size(0), 
  m_sum_azel_size(0), m_sum_gostop_size(0),
  m_go_pixmaps(), m_stop_pixmaps(), m_target_pixmaps(),
  m_ind_go_pixmaps(), m_ind_cf_pixmaps(),
  m_ind_stop_pixmaps(), m_sw_pixmaps(), m_sum_go_pixmaps(), 
  m_sum_stop_pixmaps(), m_sum_az_pixmaps(), m_sum_el_pixmaps()
{
  m_go_pixmaps.resize(N(go_xpms), 0);
  m_stop_pixmaps.resize(N(stop_xpms), 0);
  m_target_pixmaps.resize(1, 0);
  m_ind_go_pixmaps.resize(N(ind_go_xpms), 0);
  m_ind_cf_pixmaps.resize(N(ind_cf_xpms), 0);
  m_ind_stop_pixmaps.resize(N(ind_stop_xpms), 0);
  m_sw_pixmaps.resize(N(sw_xpms), 0);
  m_sum_go_pixmaps.resize(N(go_xpms), 0);
  m_sum_stop_pixmaps.resize(N(stop_xpms), 0);
  m_sum_az_pixmaps.resize(7200);
  m_sum_el_pixmaps.resize(7200);
}

GUIPixmaps::~GUIPixmaps()
{
  for(unsigned i=0;i<m_go_pixmaps.size();i++)
    delete m_go_pixmaps[i];
  for(unsigned i=0;i<m_stop_pixmaps.size();i++)
    delete m_stop_pixmaps[i];
  for(unsigned i=0;i<m_ind_go_pixmaps.size();i++)
    delete m_ind_go_pixmaps[i];
  for(unsigned i=0;i<m_ind_cf_pixmaps.size();i++)
    delete m_ind_cf_pixmaps[i];
  for(unsigned i=0;i<m_ind_stop_pixmaps.size();i++)
    delete m_ind_stop_pixmaps[i];
  for(unsigned i=0;i<m_sw_pixmaps.size();i++)
    delete m_sw_pixmaps[i];
  for(unsigned i=0;i<m_sum_go_pixmaps.size();i++)
    delete m_sum_go_pixmaps[i];
  for(unsigned i=0;i<m_sum_stop_pixmaps.size();i++)
    delete m_sum_stop_pixmaps[i];
  for(unsigned i=0;i<m_sum_az_pixmaps.size();i++)
    delete m_sum_az_pixmaps[i];
  for(unsigned i=0;i<m_sum_el_pixmaps.size();i++)
    delete m_sum_el_pixmaps[i];
}

const QPixmap* GUIPixmaps::go_pixmaps(unsigned i)
{
  if(m_gostop_size==0)m_gostop_size=40;
  i=i%m_go_pixmaps.size();
  if(m_go_pixmaps[i]==0)
    {
      QImage im(go_xpms[i]);
      QImage sm=im.smoothScale(m_gostop_size*im.width()/im.height(),
			       m_gostop_size, QImage::ScaleMin);
      m_go_pixmaps[i]=new QPixmap(sm);
    }
  return m_go_pixmaps[i];
}

const QPixmap* GUIPixmaps::stop_pixmaps(unsigned i)
{
  if(m_gostop_size==0)m_gostop_size=40;
  i=i%m_stop_pixmaps.size();
  if(m_stop_pixmaps[i]==0)
    {
      QImage im(stop_xpms[i]);
      QImage sm=im.smoothScale(m_gostop_size*im.width()/im.height(),
			       m_gostop_size, QImage::ScaleMin);
      m_stop_pixmaps[i]=new QPixmap(sm);
    }
  return m_stop_pixmaps[i];
}

const QPixmap* GUIPixmaps::target_pixmaps(unsigned i)
{
  if(m_target_size==0)m_target_size=40;
  i=i%m_target_pixmaps.size();
  if(m_target_pixmaps[i]==0)
    {
      unsigned pixwidth = 300;
      unsigned pixheight = 300;
      unsigned delta = 50;
      QPainter painter;

      QPixmap pix(pixwidth,pixheight);
      pix.fill();
      painter.begin(&pix);
      QColor c(225,31,35);
      painter.setPen(QPen(c,0));
      painter.setBrush(QBrush(c));
      painter.drawEllipse(0,0,pixwidth,pixheight);
      painter.setPen(QPen(Qt::white,0));
      painter.setBrush(QBrush(Qt::white));
      painter.drawEllipse(delta,delta,pixwidth-2*delta,pixheight-2*delta);
      painter.setPen(QPen(c,0));
      painter.setBrush(QBrush(c));
      painter.drawEllipse(2*delta,2*delta,pixwidth-4*delta,pixheight-4*delta);
      painter.flush();
      painter.end();

      QBitmap mask(pixwidth,pixheight);
      mask.fill(Qt::color0);      
      painter.begin(&mask);
      QColor mc = Qt::color1;
      painter.setPen(QPen(mc,0));
      painter.setBrush(QBrush(mc));
      painter.drawEllipse(0,0,pixwidth,pixheight);
      painter.flush();
      painter.end();

      pix.setMask(mask);

      QImage im(pix.convertToImage());
      QImage sm=im.smoothScale(m_target_size*im.width()/im.height(),
			       m_target_size, QImage::ScaleMin);
      m_target_pixmaps[i]=new QPixmap(sm);
    }
  return m_target_pixmaps[i];
}

const QPixmap* GUIPixmaps::ind_go_pixmaps(unsigned i)
{
  if(m_ind_size==0)m_ind_size=20;
  i=i%m_ind_go_pixmaps.size();
  if(m_ind_go_pixmaps[i]==0)
    {
      QImage im(ind_go_xpms[i]);
      QImage sm=im.smoothScale(m_ind_size*im.width()/im.height(),m_ind_size,
			       QImage::ScaleMin);
      m_ind_go_pixmaps[i]=new QPixmap(sm);
    }
  return m_ind_go_pixmaps[i];
}

const QPixmap* GUIPixmaps::ind_cf_pixmaps(unsigned i)
{
  if(m_ind_size==0)m_ind_size=20;
  i=i%m_ind_cf_pixmaps.size();
  if(m_ind_cf_pixmaps[i]==0)
    {
      QImage im(ind_cf_xpms[i]);
      QImage sm=im.smoothScale(m_ind_size*im.width()/im.height(),m_ind_size,
			       QImage::ScaleMin);
      m_ind_cf_pixmaps[i]=new QPixmap(sm);
    }
  return m_ind_cf_pixmaps[i];
}

const QPixmap* GUIPixmaps::ind_stop_pixmaps(unsigned i)
{
  if(m_ind_size==0)m_ind_size=20;
  i=i%m_ind_stop_pixmaps.size();
  if(m_ind_stop_pixmaps[i]==0)
    {
      QImage im(ind_stop_xpms[i]);
      QImage sm=im.smoothScale(m_ind_size*im.width()/im.height(),m_ind_size,
			       QImage::ScaleMin);
      m_ind_stop_pixmaps[i]=new QPixmap(sm);
    }
  return m_ind_stop_pixmaps[i];
}
  
const QPixmap* GUIPixmaps::sw_pixmaps(unsigned i)
{
  if(m_sw_size==0)m_sw_size=32;
  i=i%m_sw_pixmaps.size();
  if(m_sw_pixmaps[i]==0)
    {
      QImage im(sw_xpms[i]);
      QImage sm=im.smoothScale(m_sw_size*im.width()/im.height(),m_sw_size,
			       QImage::ScaleMin);
      m_sw_pixmaps[i]=new QPixmap(sm);
    }
  return m_sw_pixmaps[i];
}

const QPixmap* GUIPixmaps::sum_go_pixmaps(unsigned i)
{
  if(m_sum_gostop_size==0)m_sum_gostop_size=40;
  i=i%m_sum_go_pixmaps.size();
  if(m_sum_go_pixmaps[i]==0)
    {
      QImage im(go_xpms[i]);
      QImage sm=im.smoothScale(m_sum_gostop_size*im.width()/im.height(),
			       m_sum_gostop_size, QImage::ScaleMin);
      m_sum_go_pixmaps[i]=new QPixmap(sm);
    }
  return m_sum_go_pixmaps[i];
}

const QPixmap* GUIPixmaps::sum_stop_pixmaps(unsigned i)
{
  if(m_sum_gostop_size==0)m_sum_gostop_size=40;
  i=i%m_sum_stop_pixmaps.size();
  if(m_sum_stop_pixmaps[i]==0)
    {
      QImage im(stop_xpms[i]);
      QImage sm=im.smoothScale(m_sum_gostop_size*im.width()/im.height(),
			       m_sum_gostop_size, QImage::ScaleMin);
      m_sum_stop_pixmaps[i]=new QPixmap(sm);
    }
  return m_sum_stop_pixmaps[i];
}

#if 0
const QPixmap* GUIPixmaps::sum_el_pixmaps(unsigned i)
{
  static TelescopeIndicator* elind = 
    new TelescopeIndicator(270,0,360,-1,89.5,QColor(128,128,128),
			   color_bg_warn,0);
  if(m_sum_azel_size==0)m_sum_azel_size=40;
  i=i%m_sum_el_pixmaps.size();
  if(m_sum_el_pixmaps[i]==0)
    {
      QSize s(m_sum_azel_size*4,m_sum_azel_size*4);
      QImage im = elind->draw(double(int(i)-3600)/10.0,s).convertToImage();
      QImage sm=im.smoothScale(m_sum_azel_size,m_sum_azel_size,
			       QImage::ScaleMin);
      m_sum_el_pixmaps[i]=new QPixmap(sm);
    }
  return m_sum_el_pixmaps[i];
}

const QPixmap* GUIPixmaps::sum_az_pixmaps(unsigned i)
{
  static TelescopeIndicator* azind = 
    new TelescopeIndicator(0,0,360,-260,260,QColor(128,128,128),
			   color_bg_warn,0);
  if(m_sum_azel_size==0)m_sum_azel_size=40;
  i=i%m_sum_az_pixmaps.size();
  if(m_sum_az_pixmaps[i]==0)
    {
      QSize s(m_sum_azel_size*4,m_sum_azel_size*4); 
      QImage im = azind->draw(double(int(i)-3600)/10.0,s).convertToImage();
      QImage sm=im.smoothScale(m_sum_azel_size,m_sum_azel_size,
			       QImage::ScaleMin);
      m_sum_az_pixmaps[i]=new QPixmap(sm);
    }
  return m_sum_az_pixmaps[i];
}
#endif

GUIPixmaps* GUIPixmaps::instance()
{
  if(s_instance.get()==0)s_instance.reset(new GUIPixmaps);
  return s_instance.get();
}
