//-*-mode:c++; mode:font-lock;-*-

/**
 * \file GUIPixmaps.h
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
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_GUIPIXMAPS_H
#define VTRACKING_GUIPIXMAPS_H

#include<memory>
#include<vector>

#include<qpixmap.h>

class GUIPixmaps
{
public:
  virtual ~GUIPixmaps();

  static GUIPixmaps* instance();

  void setGoStopSize(unsigned size) { if(m_gostop_size==0)m_gostop_size=size; }
  void setTargetSize(unsigned size) { if(m_target_size==0)m_target_size=size; }
  void setIndSize(unsigned size) { if(m_ind_size==0)m_ind_size=size; }
  void setSWSize(unsigned size) { if(m_sw_size==0)m_sw_size=size; }
  void setSumGoStopSize(unsigned size) 
  { if(m_sum_gostop_size==0)m_sum_gostop_size=size; }
  void setSumAzElSize(unsigned size) { if(m_sum_azel_size==0)m_sum_azel_size=size; }

  const QPixmap* go_pixmaps(unsigned i);
  const QPixmap* stop_pixmaps(unsigned i);
  const QPixmap* target_pixmaps(unsigned i);
  const QPixmap* ind_go_pixmaps(unsigned i);
  const QPixmap* ind_cf_pixmaps(unsigned i);
  const QPixmap* ind_stop_pixmaps(unsigned i);
  const QPixmap* sw_pixmaps(unsigned i);

  const QPixmap* sum_go_pixmaps(unsigned i);
  const QPixmap* sum_stop_pixmaps(unsigned i);

#if 0
  const QPixmap* sum_el_pixmaps(unsigned i);
  const QPixmap* sum_az_pixmaps(unsigned i);
#endif

protected:
  GUIPixmaps();

private:
  GUIPixmaps(const GUIPixmaps&);				 
  GUIPixmaps& operator= (const GUIPixmaps&);

  static std::auto_ptr<GUIPixmaps> s_instance;

  unsigned m_gostop_size;
  unsigned m_target_size;
  unsigned m_ind_size;
  unsigned m_sw_size;
  unsigned m_sum_azel_size;
  unsigned m_sum_gostop_size;

  std::vector<QPixmap*> m_go_pixmaps;
  std::vector<QPixmap*> m_stop_pixmaps;
  std::vector<QPixmap*> m_target_pixmaps;
  std::vector<QPixmap*> m_ind_go_pixmaps;
  std::vector<QPixmap*> m_ind_cf_pixmaps;
  std::vector<QPixmap*> m_ind_stop_pixmaps;
  std::vector<QPixmap*> m_sw_pixmaps;
  std::vector<QPixmap*> m_sum_go_pixmaps;
  std::vector<QPixmap*> m_sum_stop_pixmaps;
  std::vector<QPixmap*> m_sum_az_pixmaps;
  std::vector<QPixmap*> m_sum_el_pixmaps;
};


#endif // VTRACKING_GUIPIXMAPS_H
