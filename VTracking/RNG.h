//-*-mode:c++; mode:font-lock;-*-

/**
 * \file RNG.h
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
 * $Date: 2006/04/04 17:06:58 $
 * $Revision: 2.0 $
 * $Tag$
 *
 **/

#ifndef RNG_H
#define RNG_H

#include<string>
#include<limits>
#include<memory>

namespace RNG
{
  
  class RNG
  {
  public:
    class InverseTransformation
    {
    public:
      virtual ~InverseTransformation();
      virtual double transform(double) = 0;
    };

    virtual ~RNG();

    double linDev() { return deviate(); }
    double expDev();
    double gasDev();
    double trnDev(InverseTransformation& trn);
    double gamDev(unsigned ia);
    unsigned poiDev(double xm);
    unsigned bnlDev(double pp, int n);

    virtual double linDevMax() = 0;

  private:
    virtual double deviate() = 0;
  };

  class NRRan2: public RNG
  {
  public:
    NRRan2(int seed_num=0);
    NRRan2(const char* seed_filename, int fallback_seed_num=0);

    virtual ~NRRan2();
    virtual double linDevMax();

    void seed(int seed);
    bool readSeedFile(const char* filename);
    void writeSeedFile(const char* filename);

    static NRRan2* instance()
    {
      if(s_instance.get()==0)s_instance.reset(new NRRan2(2345));
      return s_instance.get();
    }

    static double makeLinDev() { return instance()->linDev(); }
    static double makeExpDev() { return instance()->expDev(); }
    static double makeGasDev() { return instance()->gasDev(); }
    static double makeTrnDev(InverseTransformation& trn)
    { return instance()->trnDev(trn); }
    static double makeGamDev(unsigned ia) { return instance()->gamDev(ia); }
    static unsigned makePoiDev(double xm) { return instance()->poiDev(xm); }
    static unsigned makeBnlDev(double pp, int n)
    { return instance()->bnlDev(pp,n); }

  private:
    virtual double deviate();

    static const int NTAB   = 32;
    static const int A1     = 40014;
    static const int A2     = 40692;
    static const int Q1     = 53668;
    static const int Q2     = 52774;
    static const int R1     = 12211;
    static const int R2     = 3791;
    static const int M1     = 2147483563;
    static const int M2     = 2147483399;

    inline static double RNMX() 
    { return (1.0-std::numeric_limits<double>::epsilon()); }

    int m_idum1;
    int m_idum2;
    int m_iy;
    int m_iv[NTAB];

    std::string m_seed_filename;

    static std::auto_ptr<NRRan2> s_instance;
  };

}; // namespace RNG

#endif // defined RNG_H
