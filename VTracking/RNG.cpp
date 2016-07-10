//-*-mode:c++; mode:font-lock;-*-

/**
 * \file RNG.cpp
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
 * $Date: 2006/04/10 18:01:11 $
 * $Revision: 2.1 $
 * $Tag$
 *
 **/

#include<fstream>
#include<iostream>
#include<cstdlib>
#include<cmath>

#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include<time.h>

#include<Debug.h>

#include"RNG.h"

using namespace VMessaging;

/* Returns the value ln[Gamma(xx)] for xx > 0. */
static float gammln(float xx)
{
  double x,y,tmp,ser;
  static double cof[6]={76.18009172947146, -86.50532032941677,
			24.01409824083091, -1.231739572450155,
			0.1208650973866179e-2, -0.5395239384953e-5};
  int j;
  
  if (xx <= 0) {
    Debug::stream()
      << "Error in gammln: argument " << xx << " not > 0"
      << std::endl;
    abort();
  }
  
  y=x=xx;
  tmp=x+5.5;
  tmp-=(x+0.5)*log(tmp);
  ser=1.000000000190015;
  for (j=0;j<=5;j++) ser+=cof[j]/++y;
  return -tmp+log(2.5066282746310005*ser/x);
}

// ----------------------------------------------------------------------------
// RNG CLASS STARTS HERE
// ----------------------------------------------------------------------------

RNG::RNG::InverseTransformation::~InverseTransformation()
{
  // nothing to see here
}

RNG::RNG::~RNG()
{
  // nothing to see here
}

// Returns an exponentially distributed, positive, random deviate of
// unit mean.  Waiting times between independent Poisson-random events
// is exponentially distributed, for example.
double RNG::RNG::expDev()
{
  double dum;
  do dum=deviate();
  while (dum == 0.0);
  return -log(dum);
}

// Returns a normally distributed deviate with zero mean and unit
// variance. Algorithm is based on the Box-Muller transformation to
// get two normal deviates.
double RNG::RNG::gasDev()
{
  static bool iset=0;
  static double gset;
  double fac,rsq,v1,v2;
  
  if(!iset) 
    {
      do {
	v1=2.0*deviate()-1.0; 
	v2=2.0*deviate()-1.0;
	rsq=v1*v1+v2*v2;
      } while (rsq >= 1.0 || rsq == 0.0);
    fac=sqrt(-2.0*log(rsq)/rsq);
    gset=v1*fac;
    iset=1;
    return v2*fac;
  } 
  else {
    iset=0;
    return gset;
  }
}

// Returns arbitrary distributed deviate using transformation method.
// Inverse function is an argument of this routine.
double RNG::RNG::trnDev(RNG::RNG::InverseTransformation& trn)
{
  double dum;  
  dum=deviate();
  return trn.transform(dum);
}

// Returns a deviate distributed as a gamma distribution of integer
// order ia, i.e., a waiting time to the iath event in a Poisson
// process of unit mean. pdf=x^(ia-1)*exp(-x)/Gamma(ia)
double RNG::RNG::gamDev(unsigned ia)
{
  double am,e,s,v1,v2,x,y;

  if (ia < 1) {
    Debug::stream()
      << "Error in RNG::gamdev: integer order " << ia << " not > 0"
      << std::endl;
    abort();
  }

  if(ia < 6) {
    x=1.0;
    for (unsigned j=1;j<=ia;j++) x *= deviate();
    x = -log(x);
  } 
  else {
    do {
      do {
	do { 
	  v1=deviate();
            v2=2.0*deviate()-1.0;
	} while (v1*v1+v2*v2 > 1.0);
	y=v2/v1;
	am=ia-1;
	s=sqrt(2.0*am+1.0);
	x=s*y+am; 
      } while (x <= 0.0);
      e=(1.0+y*y)*exp(am*log(x/am)-s*y);
    } while (deviate() > e);
  }
  return x;
}

// Returns random deviate drawn from a Poisson distribution of mean xm
unsigned RNG::RNG::poiDev(double xm)
{
  static double sq,alxm,g,oldm=(-1.0);
  double em,t,y;
  
  if (xm < 12.0) {           /* Use direct method */
    if (xm != oldm) {
      oldm=xm;
      g=exp(-xm);
    }
    em = -1;
    t=1.0;
    do {
      ++em;
      t *= deviate();
    } while (t > g);
  } 
  else {                     /* Use rejection method */
    if (xm != oldm) { 
      oldm=xm;
      sq=sqrt(2.0*xm);
      alxm=log(xm);
      g=xm*alxm-gammln(xm+1.0);
    }
    do {
      do {
	y=tan(M_PI*deviate());
	em=sq*y+xm;
      } while (em < 0.0);
      em=floor(em);
      t=0.9*(1.0+y*y)*exp(em*alxm-gammln(em+1.0)-g);
    } while (deviate() > t);
    }
  return (unsigned)em;
}

// Returns an integer value that is a random deviate drawn from a
// binomial distribution of n trials each of probability pp
unsigned RNG::RNG::bnlDev(double pp, int n)
{
  int j;
  static int nold=(-1);
  float am,em,g,angle,p,bnl,sq,t,y;
  static float pold=(-1.0),pc,plog,pclog,en,oldg;
  
  p=(pp <= 0.5 ? pp : 1.0-pp);
  am=n*p;
  if (n < 25) {              /* Use direct method */
    bnl=0.0;
    for (j=1;j<=n;j++) if (deviate() < p) ++bnl;
  } 
  else if (am < 1.0) {
    g=exp(-am);
    t=1.0;
    for (j=0;j<=n;j++) {
      t *= deviate();
      if (t < g) break;
    }
    bnl=(j <= n ? j : n);
  } 
  else {                     /* Use rejection method */
    if (n != nold) { 
      en=n;
      oldg=gammln(en+1.0);
      nold=n;
    } 
    if (p != pold) { 
      pc=1.0-p;
      plog=log(p);
      pclog=log(pc);
      pold=p;
    }
    sq=sqrt(2.0*am*pc);
    do {
      do {
	angle=M_PI*deviate();
	y=tan(angle);
	em=sq*y+am;
      } while (em < 0.0 || em >= (en+1.0));
      em=floor(em);
      t=1.2*sq*(1.0+y*y)*exp(oldg-gammln(em+1.0)
			     -gammln(en-em+1.0)+em*plog+(en-em)*pclog);
    } while (deviate() > t);
    bnl=em;
  }
  if (p != pp) bnl=n-bnl;
  return (unsigned)bnl;
}

// ----------------------------------------------------------------------------
// NRRAN2 CLASS
// ----------------------------------------------------------------------------

RNG::NRRan2::NRRan2(int seed_num)
  : RNG(), m_idum1(), m_idum2(), m_iy(), m_iv(), m_seed_filename()
{
  seed(seed_num);
}

RNG::NRRan2::NRRan2(const char* seed_filename, int fallback_seed_num)
  : RNG(), m_idum1(), m_idum2(), m_iy(), m_iv(), m_seed_filename(seed_filename)
{
  if(!readSeedFile(seed_filename))seed(fallback_seed_num);
}

RNG::NRRan2::~NRRan2()
{
  if(m_seed_filename!="")writeSeedFile(m_seed_filename.c_str());
}

double RNG::NRRan2::linDevMax()
{ 
  return RNMX();
}

//  Long period (>2e+18) random number generator of L'Ecuyer with  
//  Bays-Durham shuffle. Returns a uniform random deviate between 0.0 and 
//  1.0 (exclusive of the endpoint values). 
double RNG::NRRan2::deviate()
{
  int    j;
  long   k;
  double am=(1.0/M1);
  long   imm1=(M1-1);
  long   ndiv=(1+(M1-1)/NTAB);
  double tmp;
  
  k=m_idum1/Q1;
  m_idum1=A1*(m_idum1-k*Q1)-R1*k; 
  if (m_idum1<0) m_idum1+=M1;
  k=m_idum2/Q2;
  m_idum2=A2*(m_idum2-k*Q2)-R2*k;
  if (m_idum2<0) m_idum2+=M2;
  j=m_iy/ndiv;
  m_iy=m_iv[j]-m_idum2;
  m_iv[j]=m_idum1;
  if(m_iy<1) m_iy+=imm1;
  tmp=(double)(am*m_iy);
  if(tmp>RNMX()) return RNMX();
  else return tmp;
}

void RNG::NRRan2::seed(int seed)
{
  if(seed==0)
    {
      int fd=0;
      int nr=0;
      
      fd=open("/dev/random",O_RDONLY);
      if(fd!=-1)
	nr=read(fd,&seed,sizeof(seed)), close(fd);
      if(nr!=sizeof(seed))seed=(int)time(0);
    }

  if(seed<0)seed=-seed;
  
  m_idum2=seed;
  for (int j=NTAB+7;j>=0;j--) 
    {
      long k=seed/Q1;
      seed=A1*(seed-k*Q1)-R1*k; 
      if (seed<0)seed+=M1;
      if (j<NTAB)m_iv[j]= seed;
    }

  m_iy=m_iv[0];
  m_idum1=seed;
}

bool RNG::NRRan2::readSeedFile(const char* seed_filename)
{
  std::ifstream stream(seed_filename);
    
  if(stream)
    {
      stream >> m_idum1 >> m_idum2 >>  m_iy;
      for(int j=0; j<NTAB; j++)stream >> m_iv[j];
    }

  if(stream)return true;
  else return false;
}

void RNG::NRRan2::writeSeedFile(const char* seed_filename)
{
  std::ofstream stream(seed_filename);
  if(stream)
    {
      stream << m_idum1 << std::endl
	     << m_idum2 << std::endl
	     << m_iy << std::endl;
      for(int j=0; j<NTAB; j++)stream << m_iv[j] << std::endl;
    }
}

std::auto_ptr<RNG::NRRan2> RNG::NRRan2::s_instance(0);

