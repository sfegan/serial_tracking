//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Astro.cpp
 * \ingroup SEphem
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

#include<sys/time.h>

#include<sstream>
#include<iomanip>

#include"Astro.h"

using namespace SEphem;

// SJF 2010-10-28: TAI-UTC change from 33 to 34 (2 years late!)
double Astro::m_delta_t = 32.184 + 34 + 0.25; // 32.184 + (TAI-UTC) + (UT1-UTC)

#if 0
static void autoSetDeltaT(bool now, double mjd)
{
  if(now)
    {
      gettimeofday(&tv,0);
      mjd = 40587.0+(double(tv.tv_sec)+double(tv.tv_usec)/1000000)/86400;
    }
  
}
#endif

// ============================================================================
// EARTH FRAME
// ============================================================================

void Astro::nutationAnglesRigourous(double T, 
				    Angle& nutationlong, Angle& nutationobl)
{
  // Astronomical Algorithms Pages 143-146

  double T2 = T*T;
  double T3 = T2*T;
  
  Angle D =
    Angle::frDeg(297.85036 + 445267.111480*T - 0.0019142*T2 + T3/189474.0);

  Angle M =
    Angle::frDeg(357.52772 + 35999.050340*T - 0.0001603*T2 - T3/300000.0);

  Angle MP =
    Angle::frDeg(134.96298 + 477198.867398*T + 0.0086972*T2 + T3/56250.0);

  Angle F =
    Angle::frDeg(93.27191 + 483202.017538*T - 0.0036825*T2 + T3/327270.0);

  Angle S =
    Angle::frDeg(125.04452 - 1934.136261*T + 0.0020708*T2 + T3/450000.0);

  double D1 = D.rad();
  double D2 = D*2.0;
  //double D3 = D*3.0;
  //double D4 = D*4.0;

  double M1 = M.rad();
  double M2 = M*2.0;
  //double M3 = M*3.0;
  //double M4 = M*4.0;

  double MP1 = MP.rad();
  double MP2 = MP*2.0;
  double MP3 = MP*3.0;
  //double MP4 = MP*4.0;

  //double F1 = F.rad();
  double F2 = F*2.0;
  //double F3 = F*3.0;
  //double F4 = F*4.0;

  double S1 = S.rad();
  double S2 = S*2.0;
  //double S3 = S*3.0;
  //double S4 = S*4.0;

  double dpsi = 0;
  double deps = 0;

#include "nutation.h"

#if 0
  std::cerr << "T    = " << T << std::endl
	    << "D    = " << D.degString(4) << std::endl
	    << "M    = " << M.degString(4) << std::endl
	    << "MP   = " << MP.degString(4) << std::endl
	    << "F    = " << F.degString(4) << std::endl
	    << "S    = " << S.degString(4) << std::endl
#endif

  nutationlong.setDeg(dpsi*0.0001/3600.0);
  nutationobl.setDeg(deps*0.0001/3600.0);
}

// ============================================================================
// SUN
// ============================================================================

void Astro::sunRaDecApparent(double mjd, SphericalCoords& c)
{
  // Approximation from Astronomical Algorithms Pages 163-165, 167

  double T = Tdy(mjd);

  double e;
  Angle Sl;
  Angle nu;
  double R;
  sunGeocentric(T, e, Sl, nu, R);

  Angle dpsi;
  Angle deps;
  Angle eps0;
  nutationAngles(T,dpsi,deps);
  meanObliquity(T,eps0);
  Angle eps = Angle::makeRad(eps0.rad()+deps.radPM());

  Angle lambda = Sl + dpsi - Angle::frDeg(20.4898/3600);

  c.setLatLong(0,lambda);
  eclipticToRaDec(eps, c);
}

// ============================================================================
// MOON
// ============================================================================

void Astro::moonLatLongApparent(double mjd, SphericalCoords& c)
{
  // Approximation from Astronomical Algorithms Pages 337-342
  
  double T = Tdy(mjd + m_delta_t/86400.0);

  //T = -0.077221081451; // -- use to reproduce worked example in AA

  double T2 = T*T;
  double T3 = T2*T;
  double T4 = T3*T;

  Angle LP = Angle::frDeg(218.3164477);
  LP += Angle::frDeg(481267.88123421*T);
  LP -= Angle::frDeg(0.0015786*T2);
  LP += Angle::frDeg(T3/538841.0);
  LP -= Angle::frDeg(T4/65194000.0);

  Angle D = Angle::frDeg(297.8501921);
  D += Angle::frDeg(445267.1114034*T);
  D -= Angle::frDeg(0.0018819*T2);
  D += Angle::frDeg(T3/545868.0);
  D -= Angle::frDeg(T4/113065000.0);

  Angle M = Angle::frDeg(357.5291092);
  M += Angle::frDeg(35999.0502909*T);
  M -= Angle::frDeg(0.0001536*T2);
  M += Angle::frDeg(T3/24490000.0);

  Angle MP = Angle::frDeg(134.9633964);
  MP += Angle::frDeg(477198.8675055*T);
  MP += Angle::frDeg(0.0087414*T2);
  MP += Angle::frDeg(T3/69699.0);
  MP -= Angle::frDeg(T4/14712000.0);

  Angle F = Angle::frDeg(93.2720950);
  F += Angle::frDeg(483202.0175233*T);
  F -= Angle::frDeg(0.0036539*T2);
  F -= Angle::frDeg(T3/3526000.0);
  F += Angle::frDeg(T4/863310000.0);

  double D1 = D.rad();
  double D2 = D*2.0;
  double D3 = D*3.0;
  double D4 = D*4.0;

  double M1 = M.rad();
  double M2 = M*2.0;
  //double M3 = M*3.0;
  //double M4 = M*4.0;

  double MP1 = MP.rad();
  double MP2 = MP*2.0;
  double MP3 = MP*3.0;
  double MP4 = MP*4.0;

  double F1 = F.rad();
  double F2 = F*2.0;
  double F3 = F*3.0;
  //double F4 = F*4.0;

  double E = 1.0 - 0.002516*T - 0.0000074*T2;
  double E2 = E*E;

  double Sl = 0;
  double Sb = 0;
  //double Sr = 0;

#include "moon.h"

  Angle A1 = Angle::frDeg(119.75);  A1 += Angle::frDeg(131.849 * T);
  Angle A2 = Angle::frDeg(53.09);   A2 += Angle::frDeg(479264.290 * T);
  //Angle A3 = Angle::frDeg(313.45);  A3 += Angle::frDeg(481266.484 * T);
  Angle A3 = Angle::frDeg(313.45 + 481266.484 * T);

  Sl += 0.003958*sin(A1);
  Sl += 0.001962*sin(LP-F);
  Sl += 0.000318*sin(A2);

  Sb -= 0.002235*sin(LP);
  Sb += 0.000382*sin(A3);
  Sb += 0.000175*sin(A1-F);
  Sb += 0.000175*sin(A1+F);
  Sb += 0.000127*sin(LP-MP);
  Sb -= 0.000115*sin(LP+MP);

#if 0
  std::cerr << "T  = " << T << std::endl
	    << "LP = " << LP.degString(6) << std::endl
	    << "D  = " << D.degString(6) << std::endl
	    << "M  = " << M.degString(6) << std::endl
	    << "MP = " << MP.degString(6) << std::endl
	    << "F  = " << F.degString(6) << std::endl
	    << "A1 = " << A1.degString(6) << std::endl
	    << "A2 = " << A2.degString(6) << std::endl
	    << "A3 = " << A3.degString(6) << std::endl
	    << "E  = " << std::setprecision(6) << E << std::endl
	    << "Sl = " << Angle::makeDeg(Sl).degPMString(6) << std::endl
	    << "Sb = " << Angle::makeDeg(Sb).degPMString(6) << std::endl;
#endif

  c.setLatLong(Angle::frDeg(Sb), LP + Angle::frDeg(Sl));
}

void Astro::moonRaDecApparent(double mjd, SphericalCoords& c)
{
  moonLatLongApparent(mjd,c);

  double T = Tdy(mjd + m_delta_t/86400.0);

  Angle dpsi;
  Angle deps;
  Angle eps0;
  nutationAngles(T,dpsi,deps);
  meanObliquity(T,eps0);
  Angle eps = Angle::makeRad(eps0.rad()+deps.radPM());

  c.setPhi(c.phi() + dpsi);

#if 0
  std::cerr << "dpsi   = " << dpsi.degString(6) << std::endl
	    << "eps0   = " << eps0.degString(6) << std::endl
	    << "deps   = " << deps.degString(6) << std::endl
	    << "lambda = " << c.longitude().degString(6) << std::endl
	    << "beta   = " << c.latitude().degPMString(6) << std::endl;
#endif

  eclipticToRaDec(eps, c);

#if 0
  std::ostringstream test;
  test << std::fixed << std::setprecision(8) << mjd;
  std::cerr << "mjd    = " << test.str() << std::endl
	    << "ra     = " << c.phi().hmsString(2) << std::endl
	    << "dec    = " << c.latitude().dmsString(2) << std::endl;

#endif
}

double Astro::moonDistance(double mjd)
{
  // Approximation from Astronomical Algorithms Pages 337-342
  
  double T = Tdy(mjd + m_delta_t/86400.0);

  //T = -0.077221081451; // -- use to reproduce worked example in AA

  double T2 = T*T;
  double T3 = T2*T;
  double T4 = T3*T;

  Angle LP = Angle::frDeg(218.3164477);
  LP += Angle::frDeg(481267.88123421*T);
  LP -= Angle::frDeg(0.0015786*T2);
  LP += Angle::frDeg(T3/538841.0);
  LP -= Angle::frDeg(T4/65194000.0);

  Angle D = Angle::frDeg(297.8501921);
  D += Angle::frDeg(445267.1114034*T);
  D -= Angle::frDeg(0.0018819*T2);
  D += Angle::frDeg(T3/545868.0);
  D -= Angle::frDeg(T4/113065000.0);

  Angle M = Angle::frDeg(357.5291092);
  M += Angle::frDeg(35999.0502909*T);
  M -= Angle::frDeg(0.0001536*T2);
  M += Angle::frDeg(T3/24490000.0);

  Angle MP = Angle::frDeg(134.9633964);
  MP += Angle::frDeg(477198.8675055*T);
  MP += Angle::frDeg(0.0087414*T2);
  MP += Angle::frDeg(T3/69699.0);
  MP -= Angle::frDeg(T4/14712000.0);

  Angle F = Angle::frDeg(93.2720950);
  F += Angle::frDeg(483202.0175233*T);
  F -= Angle::frDeg(0.0036539*T2);
  F -= Angle::frDeg(T3/3526000.0);
  F += Angle::frDeg(T4/863310000.0);

  double D1 = D.rad();
  double D2 = D*2.0;
  double D3 = D*3.0;
  double D4 = D*4.0;

  double M1 = M.rad();
  double M2 = M*2.0;
  //double M3 = M*3.0;
  //double M4 = M*4.0;

  double MP1 = MP.rad();
  double MP2 = MP*2.0;
  double MP3 = MP*3.0;
  double MP4 = MP*4.0;

  //double F1 = F.rad();
  double F2 = F*2.0;
  //double F3 = F*3.0;
  //double F4 = F*4.0;

  double E = 1.0 - 0.002516*T - 0.0000074*T2;
  double E2 = E*E;

  double Sr = 0;

#include "moon_r.h"

#if 0
  std::cerr << "T  = " << T << std::endl
	    << "LP = " << LP.degString(6) << std::endl
	    << "D  = " << D.degString(6) << std::endl
	    << "M  = " << M.degString(6) << std::endl
	    << "MP = " << MP.degString(6) << std::endl
	    << "F  = " << F.degString(6) << std::endl
	    << "E  = " << std::setprecision(6) << E << std::endl
	    << "Sr = " << std::setprecision(9) << Sr << std::endl;
#endif

  return 385000.56 + Sr*0.001;
}

Angle Astro::moonAngle(double mjd)
{
  // Algorithm from Astronomical Algorithms Pages 345-347

  // SUN ----------------------------------------------------------------------

  SphericalCoords sun;
  sunRaDecApparent(mjd, sun);

  double T = Tdy(mjd);
  double e;
  Angle Sl;
  Angle nu;
  double Rsun;
  sunGeocentric(T, e, Sl, nu, Rsun);
  Rsun *= astronomicalUnitKM();

  // MOON ---------------------------------------------------------------------

  SphericalCoords moon;
  moonRaDecApparent(mjd, moon);

  double Rmoon = moonDistance(mjd);

  // ANGLE --------------------------------------------------------------------

  Angle psi = sun.separation(moon);
  return atan2(Rsun*sin(psi),Rmoon-Rsun*cos(psi));
}

#ifdef TESTMAIN

// Test case from Meeus, Page 148

#include<VSTime.hpp>

int main()
{
  double T = -0.127296372348;

  Angle dpsi;
  Angle deps;

  Astro::nutationAnglesRigourous(T, dpsi, deps);
  std::cerr << "dpsi = " << dpsi.dmsString(3) << std::endl
	    << "deps = " << deps.dmsString(3) << std::endl;

  Astro::nutationAngles(T, dpsi, deps);
  std::cerr << "dpsi = " << dpsi.dmsString(3) << std::endl
	    << "deps = " << deps.dmsString(3) << std::endl;

  double mjd = VERITAS::VSTime::now().getMJDDbl();

  std::cerr << std::setprecision(9) << Astro::moonDistance(mjd) << std::endl
	    << Astro::moonAngle(mjd).degString(4) << std::endl
	    << std::setprecision(9) << Astro::moonPhase(mjd) << std::endl;
}

#endif

