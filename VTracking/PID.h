//-*-mode:c++; mode:font-lock;-*-

/**
 * \file PID.h
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
 * $Date: 2007/01/23 01:36:05 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_PID_H
#define VTRACKING_PID_H

#include <iostream>
#include <iomanip>
#include <list>

namespace VTracking
{
  template<typename T> class PID
  {
  public:
    PID(const T& p=0, const T& i=0, const T& d=0, unsigned l=0);
    virtual ~PID();

    //! Return - Kp*error - Kd*Derror/dt + Ki*SumError*dt
    inline T update(const T& error);
    void reset();

    T pGain() const { return m_pGain; }
    T iGain() const { return m_iGain; }
    T dGain() const { return m_dGain; }
    unsigned iLimit() const { return m_iLim; }

    void setPGain(const T& p) { m_pGain=p; }
    void setIGain(const T& i) { m_iGain=i; }
    void setDGain(const T& d) { m_dGain=d; }
    inline void setILimit(unsigned l);

  private:
    T             m_pGain;       // Proportional gain
    T             m_iGain;       // Integral gain
    T             m_dGain;       // Derivative gain

    unsigned      m_iLim;        // Number of samples to sum in integration

    T m_iState;                  // Integrator state
    std::list<T>  m_ihist;       // Error hostory for integrator

    bool          m_dStateValid; // Have we got a position input yet
    T             m_dState;      // Last position input
  };

  template<typename T> class PIDVff: private PID<T>
  {
  public:
    PIDVff(const T& p=0, const T& i=0, const T& d=0, unsigned l=0,
	   const T& vff=0, const T& aff=0): 
      PID<T>(p,i,d,l), m_vffGain(vff) , m_affGain(aff),
      m_hasPLast(), m_pLast(), m_hasVLast(), m_vLast()
    { /* nothing to see here */ }
    virtual ~PIDVff();

    //! Return - Kp*error - Kd*Derror/dt + Ki*SumError*dt
    inline T update(const T& error, const T& target_position);
    void reset();

    T pGain() const { return PID<T>::pGain(); }
    T iGain() const { return PID<T>::iGain(); }
    T dGain() const { return PID<T>::dGain(); }
    unsigned iLimit() const { return PID<T>::iLimit(); }
    T vffGain() const { return m_vffGain; }
    T affGain() const { return m_affGain; }

    void setPGain(const T& p) { PID<T>::setPGain(p); }
    void setIGain(const T& i) { PID<T>::setIGain(i); }
    void setDGain(const T& d) { PID<T>::setDGain(d); }
    void setILimit(unsigned l) { PID<T>::setILimit(l); }
    void setVffGain(const T& vff) { m_vffGain=vff; }
    void setAffGain(const T& aff) { m_affGain=aff; }

  private:
    T             m_vffGain;     // Velocity feed-forward gain
    T             m_affGain;     // Velocity feed-forward gain

    bool          m_hasPLast;    // Has a position sample 
    T             m_pLast;       // Last position
    bool          m_hasVLast;    // Has a velocity sample
    T             m_vLast;       // Last velocity
  };


} // namespace VTracking

template<typename T> 
VTracking::PID<T>::PID(const T& p, const T& i, const T& d, unsigned l):
  m_pGain(p), m_iGain(i), m_dGain(d), m_iLim(l), 
  m_iState(0), m_ihist(), m_dStateValid(false), m_dState(0)
{
  // nothing to see here
}

template<typename T> 
VTracking::PID<T>::~PID()
{
  // nothing to see here
}

template<typename T> 
void VTracking::PID<T>::reset()
{
  m_iState=0;
  m_ihist.clear();
  m_dState=0;
  m_dStateValid=false;
}

template<typename T> inline void VTracking::PID<T>::setILimit(unsigned l)
{ 
  m_iLim=l;
  if(m_ihist.size()>m_iLim)
    {
      typename std::list<T>::iterator isample = m_ihist.begin();
      m_iState=0;
      for(unsigned icount=0;icount<m_iLim;icount++)m_iState+=*(isample++);
      m_ihist.erase(isample,m_ihist.end());
    }
}

template<typename T> 
inline T VTracking::PID<T>::update(const T& error)
{
  T pTerm = T();
  T dTerm = T();
  T iTerm = T();
  
  // --------------------------------------------------------------------------
  // P R O P O R T I O N A L
  // --------------------------------------------------------------------------
  pTerm = m_pGain * error;

  // --------------------------------------------------------------------------
  // I N T E G R A L
  // --------------------------------------------------------------------------
  if(m_iLim)
    {
      m_iState += error;
      if(m_ihist.size()==m_iLim)
	{
	  m_iState -= m_ihist.back();
	  typename std::list<T>::iterator isample = m_ihist.end();
	  isample--;
	  m_ihist.erase(isample);
	}
      m_ihist.insert(m_ihist.begin(),error);
      iTerm = m_iGain * m_iState;

      T temp = T();
      for(typename std::list<T>::iterator isample = m_ihist.begin();
	  isample != m_ihist.end(); isample++)temp += *isample;
    }
  
  // --------------------------------------------------------------------------
  // D E R I V A T I V E
  // --------------------------------------------------------------------------
  if(m_dStateValid)dTerm = m_dGain * (error - m_dState);
  else m_dStateValid=true;
  m_dState = error;

#if 0
  std::cerr << std::fixed 
	    << std::setw(9) << std::setprecision(5) << error << ' ' 
	    << std::setw(9) << std::setprecision(6) << m_iState << ' ' 
	    << std::setw(9) << std::setprecision(6) << m_dState << ' '
	    << std::setw(9) << std::setprecision(6) << pTerm << ' ' 
	    << std::setw(9) << std::setprecision(6) << iTerm << ' ' 
	    << std::setw(9) << std::setprecision(6) << dTerm << ' '
	    << std::setw(9) << std::setprecision(6) << pTerm+iTerm-dTerm 
	    << std::endl;
#endif

  return pTerm + iTerm + dTerm;
}

template<typename T> VTracking::PIDVff<T>::~PIDVff()
{
  // nothing to see here
}

template<typename T> 
void VTracking::PIDVff<T>::reset()
{
  PID<T>::reset();
  m_hasPLast = false;
  m_pLast = 0;
  m_hasVLast = false;
  m_vLast = 0;
}

template<typename T> 
inline T VTracking::PIDVff<T>::update(const T& error, const T& target_position)
{
  T vTerm = T();
  T aTerm = T();

  if(m_hasPLast)
    {
      // ----------------------------------------------------------------------
      // V E L O C I T Y   F E E D   F O R W A R D
      // ----------------------------------------------------------------------
      T v = target_position-m_pLast;
      vTerm = m_vffGain*v;
      if(m_hasVLast)
	{
	  // ------------------------------------------------------------------
	  // A C C E L E R A T I O N   F E E D   F O R W A R D
	  // ------------------------------------------------------------------
	  T a = v-m_vLast;
	  aTerm = m_affGain*a;
	}
      else m_hasVLast=true;
      m_vLast = v;
    }
  else m_hasPLast=true;
  m_pLast = target_position;
  
  return PID<T>::update(error)+vTerm+aTerm;
}

#endif // VTRACKING_PID_H
