//-*-mode:c++; mode:font-lock;-*-

/**
 * \file Astro.h
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
 * $Date: 2008/01/31 21:24:13 $
 * $Revision: 2.6 $
 * $Tag$
 *
 **/

#ifndef SEPHEM_ASTRO_H
#define SEPHEM_ASTRO_H

#include<iostream>

#include"Angle.h"
#include"SphericalCoords.h"

namespace SEphem {

  class Astro
  {
  public:

    // TIME FUNCTIONS ---------------------------------------------------------
    
    static inline double Tdy(const double mjd);
    static inline double Tut(const double mjd);
    static inline double julianEpochToMJD(double epoch);
    static inline double mjdToJulianEpoch(double mjd);

    static inline Angle mjdToLMST(double mjd, double earth_long_rad = 0);

    static double deltaT() { return m_delta_t; }
    static void setDeltaT(double dt) { m_delta_t=dt; }
    //static void autoSetDeltaT(bool now=true, double mjd=0);
    static double siderealRate() { return 1.00273790935; }

    // COORDINATE TRANSFORMS --------------------------------------------------

    static inline 
    void raDecToAzEl(const Angle& lmst, const SphericalCoords& earthPos,
		     SphericalCoords& c);
    static inline 
    void azElToRaDec(const Angle& lmst, const SphericalCoords& earthPos,
		     SphericalCoords& c);
    static inline 
    void azElToMeanRaDec(const Angle& lmst, double mjd,
			 const SphericalCoords& earthPos, SphericalCoords& c);
    static inline void raDecToGal(double epoch, SphericalCoords& c);
    static inline void galToRaDec(double epoch, SphericalCoords& c);
    static inline 
    void raDecToEcliptic(const Angle& epsilon, SphericalCoords& c);
    static inline
    void eclipticToRaDec(const Angle& epsilon, SphericalCoords& c);

    static inline
    void raDecMeanToApparent(double mjd, SphericalCoords& c);
    
    static inline
    void apparentRaDecToAzEl(double mjd, const Angle& lmst, 
			     const SphericalCoords& earthPos,
			     SphericalCoords& c);

    // EARTH FRAME ------------------------------------------------------------
    
    static inline 
    void precessionalAngles(double to, double from, 
			    Angle& phi, Angle& theta, Angle& psi);

    static inline void precess(double to, SphericalCoords& c, double from);

    static inline void nutationAngles(double T, 
				      Angle& nutationlong, Angle& nutationobl);

    static void nutationAnglesRigourous(double T, 
					Angle& nutationlong, 
					Angle& nutationobl);
    
    static inline void meanObliquity(double T, Angle& obliquity);

    // SUN --------------------------------------------------------------------

    static inline double astronomicalUnitKM() { return 1.4959787e+08; }

    static inline 
    void sunGeocentric(double T, double & e,
		       Angle& sunTrueLongitude, Angle& sunTrueAnomoly,
		       double& sunDistance);
      
    static inline
    void aberration(double T, const SphericalCoords& c,
		    Angle& aberrationlong, Angle& aberrationlat);
    
    static void sunRaDecApparent(double mjd, SphericalCoords& c);

    // MOON -------------------------------------------------------------------
    
    static void moonLatLongApparent(double mjd, SphericalCoords& c);

    static void moonRaDecApparent(double mjd, SphericalCoords& c);

    static double moonDistance(double mjd);

    static Angle moonAngle(double mjd);

    static double moonPhase(double mjd) 
    { return 0.5*(1.0+cos(moonAngle(mjd))); }

  private:
    static double m_delta_t;
  };

  // ==========================================================================
  // TIME FUNCTIONS
  // ==========================================================================
  
  inline double Astro::Tdy(double mjd)
  {
    return (mjd+m_delta_t/86400.0-51544.5)/36525;
  }

  inline double Astro::Tut(double mjd)
  {
    return (mjd-51544.5)/36525;
  }
  
  inline double Astro::julianEpochToMJD(double epoch)
  {
    return 51544.5+365.25*(epoch-2000.0);
  }
  
  inline double Astro::mjdToJulianEpoch(double mjd)
  {
    return (mjd-51544.5)/365.25+2000.0;
  }
  
  inline Angle Astro::mjdToLMST(double mjd, double earth_long_rad)
  {
    const double mjd_int = floor(mjd);
    const double mjd_rem = mjd-mjd_int;
    const double T = Tut(mjd_int);
    const double th
      = 100.46061837+36000.770053608*T+0.000387933*T*T-T*T*T/38710000;
    return Angle(Angle::makeDeg(th)
		 + Angle::makeRot(mjd_rem)*siderealRate()
		 + earth_long_rad);
  }

  // ==========================================================================
  // COORDINATE TRANSFORMS
  // ==========================================================================

  inline 
  void Astro::raDecToAzEl(const Angle& lmst, const SphericalCoords& earthPos,
			  SphericalCoords& c)
  {
    c.rotate(Angle::sc_Pi,-earthPos.theta(),-lmst);
    c.setThetaPhi(c.theta(),-c.phi());
  }
  
  inline 
  void Astro::azElToRaDec(const Angle& lmst, const SphericalCoords& earthPos,
			  SphericalCoords& c)
  {
    c.setThetaPhi(c.theta(),-c.phi());
    c.rotate(lmst,earthPos.theta(),Angle::sc_Pi);
  }

  inline 
  void Astro::azElToMeanRaDec(const Angle& lmst, double mjd,
			      const SphericalCoords& earthPos,
			      SphericalCoords& c)
  {
    // See Astronomical Algorithms Chapter 23, Pages 149-159
    double T = Tdy(mjd);

    // Trivial conversion
    azElToRaDec(lmst, earthPos, c);
    
    // Get nutation angle, mean and actual obliquity
    Angle dpsi;
    Angle deps;
    Angle eps0;
    nutationAngles(T,dpsi,deps);
    meanObliquity(T,eps0);
    Angle eps = Angle::makeRad(eps0.rad()+deps.radPM());
    
    // Convert to ecliptic coordinates to add aberration
    raDecToEcliptic(eps,c);

    // Find aberration
    Angle ablong;
    Angle ablat;
    aberration(T,c,ablong,ablat);

    // Subtract aberration
    c.setPhi(c.phi().rad()-ablong.radPM());
    c.setTheta(c.theta().rad()+ablat.radPM());

    // Convert back to equatorial coordinates
    eclipticToRaDec(eps0,c);
  }
  
  inline
  void Astro::raDecToGal(double epoch, SphericalCoords& c)
  {
    // Probably not very accurate
    precess(33282.0,c,epoch);
    c.rotateDeg(123, 62.6, /*167.75*/ 347.75);
  }
  
  inline
  void Astro::galToRaDec(double epoch, SphericalCoords& c)
  {
    // Probably not very accurate
    c.rotateDeg(-347.75, -62.6, -123);
    precess(epoch,c,33282.0);
  }
  
  inline
  void Astro::raDecToEcliptic(const Angle& epsilon, SphericalCoords& c)
  {
    c.rotate(Angle::sc_halfPi,epsilon,-Angle::sc_halfPi);
  }
  
  inline
  void Astro::eclipticToRaDec(const Angle& epsilon, SphericalCoords& c)
  {
    c.rotate(Angle::sc_halfPi,-epsilon,-Angle::sc_halfPi);
  }

  inline
  void Astro::raDecMeanToApparent(double mjd, SphericalCoords& c)
  {
    // See Astronomical Algorithms Chapter 23, Pages 149-159
    double T = Tdy(mjd);
    
    // Get nutation angle, mean and actual obliquity
    Angle dpsi;
    Angle deps;
    Angle eps0;
    nutationAngles(T,dpsi,deps);
    meanObliquity(T,eps0);
    Angle eps = Angle::makeRad(eps0.rad()+deps.radPM());

    // Convert to ecliptic coordinates to add aberration
    SphericalCoords ecl = c;
    raDecToEcliptic(eps0,ecl);

    // Find aberration amount and add it to coordinates
    Angle ablong;
    Angle ablat;
    aberration(T,ecl,ablong,ablat);
    ecl.setPhi(ecl.phi().rad()+dpsi.radPM()+ablong.radPM());
    ecl.setTheta(ecl.theta().rad()-ablat.radPM());

    // Convert back to equatorial coordinates
    eclipticToRaDec(eps,ecl);
    c=ecl;
   }

  inline
  void Astro::apparentRaDecToAzEl(double mjd, const Angle& lmst, 
				  const SphericalCoords& earthPos,
				  SphericalCoords& c)
  {
    // See Astronomical Algorithms Chapter 23, Pages 149-159

    double T = Tdy(mjd);

    // Get nutation angle, mean and actual obliquity
    Angle dpsi;
    Angle deps;
    Angle eps0;
    nutationAngles(T,dpsi,deps);
    meanObliquity(T,eps0);

    // Convert to Az/El
    raDecToAzEl(lmst.rad()+dpsi.radPM()*cos(eps0+deps),earthPos,c);
  }
  
  // ==========================================================================
  // EARTH FRAME
  // ==========================================================================

  inline void Astro::precessionalAngles(double to, double from, 
					Angle& phi, Angle& theta, Angle& psi)
  {
    // Approximation from Astronomical Algorithms Page 134-135
    const double T = Tut(from);
    const double t = Tut(to) - T;
    const double T2 = T*T;
    const double t2 = t*t;
    const double t3 = t2*t;
    
    const double xi = ((2306.2181 + 1.39656*T + 0.000139*T2)*t +
		       (0.30188 - 0.000344*T)*t2 + 0.017998*t3)/3600;
    const double za = ((2306.2181 + 1.39656*T + 0.000139*T2)*t +
		       (1.09468 + 0.000066*T)*t2 + 0.018203*t3)/3600;
    const double th = ((2004.3109 - 0.85330*T - 0.000217*T2)*t -
		       (0.42665 + 0.000217*T)*t2 - 0.041833*t3)/3600;
    
    phi   = Angle::makeDeg(za);
    theta = Angle::makeDeg(-th);
    psi   = Angle::makeDeg(xi);
  }
  
  inline void Astro::precess(double to, SphericalCoords& c, double from)
  {
    Angle phi;
    Angle theta;
    Angle psi;
    precessionalAngles(to, from, phi, theta, psi);
    c.rotate(phi, theta, phi);
  }

  inline
  void Astro::nutationAngles(double T, 
			     Angle& nutationlong, Angle& nutationobl)
  {
    // Approximation from Astronomical Algorithms Page 144
    // Accuracy is 0.5" in longitude and 0.1" in obliquity
    double S = Angle::frDeg(125.04452 - 1934.136261*T);
    double Ls = Angle::frDeg(280.4665 +  36000.7698*T);
    double Lm = Angle::frDeg(218.3165 + 481267.8813*T);
    nutationlong.setDeg((-17.20*sin(S) -1.32*sin(2*Ls) 
			 -0.23*sin(2*Lm) +0.21*sin(2*S))/3600);
    nutationobl.setDeg((+9.20*cos(S) +0.57*cos(2*Ls)
			+0.10*cos(2*Lm) -0.09*cos(2*S))/3600);
  }

  inline
  void Astro::meanObliquity(double T, Angle& obliquity)
  {
    // Approximation from Astronomical Algorithms Page 147
    // Error reaches 1" over period of 2000 years
    obliquity.setDeg((84381.448+(-46.8150+(-0.00059+0.001813*T)*T)*T)/3600);
  }

  // ==========================================================================
  // SUN
  // ==========================================================================

  inline
  void Astro::sunGeocentric(double T, double & e,
			    Angle& sunTrueLongitude, Angle& sunTrueAnomoly,
			    double& sunDistance)
  {
    // Approximation from Astronomical Algorithms Pages 163-164
    // Accuracy is 0.01 degrees

    double L0 = Angle::frDeg(280.46646 + (36000.76983 + 0.0003032*T)*T);
    double M = Angle::frDeg(357.52911 + (35999.05029 + 0.0001537*T)*T);
    double C = Angle::frDeg((1.914602-(0.004817-0.000014*T)*T)*sin(M)+
			    (0.019993-0.000101*T)*sin(2*M)+
			    0.000289*sin(3*M));
    
    e = 0.016708634 + (-0.000042037 - 0.0000001267*T)*T;
    sunTrueLongitude.setRad(L0+C);
    sunTrueAnomoly.setRad(M+C);

    sunDistance = 1.000001018*(1-e*e)/(1+e*cos(sunTrueAnomoly.rad()));
  }
		
  inline
  void Astro::aberration(double T, const SphericalCoords& c,
			 Angle& aberrationlong, Angle& aberrationlat)
  {
    // Astronomical Algorithms Chapter 23, Pages 149-151
    
    double e;
    Angle Sl;
    Angle nu;
    double R;
    sunGeocentric(T, e, Sl, nu, R);
    
    double pi = Angle::frDeg(102.93735 + (1.71946 + 0.00046*T)*T);
    double k = Angle::frDeg(20.49552/3600);
    aberrationlong = 
      (-k*cos(Sl-c.longitude())+e*k*cos(pi-c.longitude()))/cos(c.latitude());
    aberrationlat = 
      -k*sin(c.latitude())*(sin(Sl-c.longitude())-e*sin(pi-c.longitude()));
    
#if 0
    Debug::stream()
      << " e:   " << e << std::endl
      << " Sl:  " << Angle::makeRad(Sl).deg() << std::endl
      << " pi:  " << Angle::toDeg(pi) << std::endl;
#endif
  }
  
} // namespace

#endif // defined SEPHEM_ASTRO_H
