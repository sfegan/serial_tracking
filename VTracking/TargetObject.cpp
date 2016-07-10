//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TargetObject.cpp
 * \ingroup VTracking
 * \brief This is a one-line description of this cpp file.
 *
 * Here is a tedious verbose multi-line description of all
 * the details of the code, more than you would
 * ever want to read. Generally, all the important documentation
 * goes in the .cpp files.
 *
 * Original Author: Stephen Fegan
 * $Author: aune $
 * $Date: 2011/10/06 02:14:28 $
 * $Revision: 2.10 $
 * $Tag$
 *
 **/

#include<iostream>
#include<fstream>
#include<sstream>
#include<iomanip>
#include<cmath>

#include<Angle.h>
#include<SphericalCoords.h>
#include<Astro.h>
#include<Debug.h>
#include<Exception.h>

#include"TargetObject.h"

using namespace SEphem;
using namespace VMessaging;
using namespace VTracking;

// ============================================================================
//
// STATIC FUNCTIONS
//
// ============================================================================

static inline std::string 
doubleToString(const double& x, unsigned precision = 1)
{
  std::ostringstream stream;
  stream << std::setprecision(precision) << std::fixed;
  stream << x;
  return stream.str();
}

// ============================================================================
//
// VTRACKING GLOBAL FUNCTIONS
//
// ============================================================================

void VTracking::targetSpeed(SphericalCoords azel, 
			    const SphericalCoords& pos,
			    double& az_speed, double& el_speed)
{
  // Hour-angle speed - 360 degrees in ( 86400 seconds / Sidereal Rate )
  const double S = 360.0/(86400.0/1.00273790935);
  double h = azel.latitude();   // Elevation
  double p = pos.latitude();    // Latitude
  Astro::azElToRaDec(0,pos,azel);
  double H = azel.longitude();  // Hour-angle
  double d = azel.latitude();   // Declination
  // Parallactic angle - Astronomical Algorithms Page 98
  double q = atan2(sin(H),tan(p)*cos(d)-sin(d)*cos(H)); 
  // Target angular speed is S*cos(declination) in the E-W direction
  // In azimuthal direction: component is S*cos(dec)*cos(parallactic angle)
  //                         speed is 1/cos(elevation) of above
  az_speed = S*cos(d)*cos(q)/cos(h); 
  // In elevation direction: component is S*cos(dec)*sin(parallactic angle)
  el_speed = S*cos(d)*sin(q);

#if 0
  Debug::stream()
    << Angle::makeRad(p).degPMString(3) << ' '
    << Angle::makeRad(A).degString(3) << ' '
    << Angle::makeRad(h).degPMString(3) << ' '
    << Angle::makeRad(H).degString(3) << ' '
    << Angle::toDeg(d) << ' '
    << Angle::makeRad(d).degPMString(3) << ' '
    << Angle::makeRad(q).degString(3) << ' '
    << std::fixed 
    << std::setw(8) << std::setprecision(5) << az_speed << ' '
    << std::setw(8) << std::setprecision(5) << el_speed << std::endl;
#endif
}

// ============================================================================
//
// TARGET OBJECT
//
// ============================================================================

TargetObject::~TargetObject()
{
  // nothing to see here
}
  
// ============================================================================
//
// RA/DEC TARGET OBJECT
//
// ============================================================================

RaDecObject::~RaDecObject()
{
  delete m_offset;
}

SphericalCoords 
RaDecObject::getDBReportedRaDec(double mjd, const Angle& lmst, 
			 const SphericalCoords& earthPos) const
{
  SphericalCoords coords(m_coords);
  Astro::precess(mjd,coords,m_epoch);
  if(m_offset)m_offset->applyOffsetForDB(mjd,lmst,earthPos,coords);
  return coords;
}

SphericalCoords 
RaDecObject::getGalCoord(const double& mjd, const Angle& lmst, 
			 const SphericalCoords& earthPos) const
{
  SphericalCoords coords(m_coords);
  Astro::precess(mjd,coords,m_epoch);
  if(m_offset)m_offset->applyOffset(mjd,lmst,earthPos,coords);
  Astro::raDecToGal(mjd, coords);
  return coords;
}

SphericalCoords 
RaDecObject::getRaDec(const double& mjd, const Angle& lmst, 
		      const SphericalCoords& earthPos) const
{
  SphericalCoords coords(m_coords);
  Astro::precess(mjd,coords,m_epoch);
  if(m_offset)m_offset->applyOffset(mjd,lmst,earthPos,coords);
  Astro::raDecMeanToApparent(mjd, coords);
  return coords;
}

SphericalCoords 
RaDecObject::getAzEl(const double& mjd, const Angle& lmst, 
		     const SphericalCoords& earthPos) const
{
  SphericalCoords c=getRaDec(mjd,lmst,earthPos);
  Astro::apparentRaDecToAzEl(mjd, lmst, earthPos, c);
  return c;
}

bool RaDecObject::objectMovesInAzEl() const
{
  return true;
}

bool RaDecObject::useCorrections() const
{
  return true;
}

TargetObject* RaDecObject::copy() const
{
  return new RaDecObject(*this);
}

std::string RaDecObject::targetName(double mjd) const
{
  std::string name;
  if(!m_name.empty())
    {
      name += m_name;
    }
  else
    {
      name += std::string("RA:")+m_coords.longitude().hmsString();
      name += std::string(" DEC:")+m_coords.latitude().dmsString();
      name += std::string(" J")+doubleToString(Astro::mjdToJulianEpoch(m_epoch),1);
    }
  
  if(m_offset == 0)
    {
      name += std::string("  /  ON");
    }
  else
    {
      name += std::string("  /  ");
      name += m_offset->offsetName(mjd);
    }
  return name;
}

std::string RaDecObject::targetObjectType() const
{
  return std::string("RA/Dec");
}

// ============================================================================
//
// "FIXED" TARGET OBJECT
//
// ============================================================================

FixedObject::~FixedObject()
{
  // nothing to see here
}

SphericalCoords 
FixedObject::getGalCoord(const double& mjd, const Angle& lmst, 
			const SphericalCoords& earthPos) const
{
  SphericalCoords c=getRaDec(mjd,lmst,earthPos);
#if 1
  double T = Astro::Tdy(mjd);
  Angle dpsi;
  Angle deps;
  Angle eps0;
  Astro::nutationAngles(T,dpsi,deps);
  Astro::meanObliquity(T,eps0);
  Angle eps = Angle::makeRad(eps0.rad()+deps.radPM());
  SphericalCoords ecl = c;
  Astro::raDecToEcliptic(eps,ecl);
  Angle ablong;
  Angle ablat;
  // First iteration through aberation with apparent position
  Astro::aberration(T,ecl,ablong,ablat);
  ecl.setPhi(ecl.phi().rad()-dpsi.radPM()-ablong.radPM());
  ecl.setTheta(ecl.theta().rad()+ablat.radPM());
  // Second iteration with guess at mean position from above
  Astro::aberration(T,ecl,ablong,ablat);
  ecl=c;
  Astro::raDecToEcliptic(eps,ecl);
  ecl.setPhi(ecl.phi().rad()-dpsi.radPM()-ablong.radPM());
  ecl.setTheta(ecl.theta().rad()+ablat.radPM());
  Astro::eclipticToRaDec(eps0,ecl);
  c=ecl;
#endif
  Astro::raDecToGal(mjd,c);
  return c;
}

SphericalCoords 
FixedObject::getRaDec(const double& mjd, const Angle& lmst, 
		     const SphericalCoords& earthPos) const
{
  SphericalCoords c=getAzEl(mjd,lmst,earthPos);
#if 1
  double T = Astro::Tdy(mjd);
  Angle dpsi;
  Angle deps;
  Angle eps0;
  Astro::nutationAngles(T,dpsi,deps);
  Astro::meanObliquity(T,eps0);
  Astro::azElToRaDec(lmst.rad()+dpsi.radPM()*cos(eps0+deps),earthPos,c);
#else
  Astro::azElToRaDec(lmst,earthPos,c);
#endif
  return c;
}

SphericalCoords 
FixedObject::getAzEl(const double& mjd, const Angle& lmst, 
	const SphericalCoords& earthPos) const
{
  return m_coords;
}

bool FixedObject::objectMovesInAzEl() const
{
  return !m_stop_at_target;
}

bool FixedObject::useCorrections() const
{
  return m_use_corrections;
}

// ============================================================================
//
// UNKNOWN TARGET OBJECT
//
// ============================================================================

UnknownObject::~UnknownObject()
{
  // nothing to see here
}

TargetObject* UnknownObject::copy() const
{
  return new UnknownObject(*this);
}

std::string UnknownObject::targetName(double mjd) const
{
  std::string name;
  name = std::string("Unknown  /  ");
  name += std::string("AZ:")+m_coords.longitude().degString(3);
  name += std::string("  EL:")+m_coords.latitude().degPMString(3);
  return name;
}

std::string UnknownObject::targetObjectType() const
{
  return std::string("Unknown");
}

// ============================================================================
//
// AZ/EL TARGET OBJECT
//
// ============================================================================

AzElObject::~AzElObject()
{
  // nothing to see here
}

TargetObject* AzElObject::copy() const
{
  return new AzElObject(*this);
}

std::string AzElObject::targetName(double mjd) const
{
  std::string name;
  name = std::string("Fixed  /  ");
  if(!m_use_corrections)name += "Encoder ";
  name += std::string("AZ:")+m_coords.longitude().degString(3);
  name += std::string("  EL:")+m_coords.latitude().degPMString(3);
  return name;
}

std::string AzElObject::targetObjectType() const
{
  return std::string("Az/El");
}

// ============================================================================
//
// STOW TARGET OBJECT
//
// ============================================================================

StowObject::~StowObject()
{
  // nothing to see here
}

TargetObject* StowObject::copy() const
{
  return new StowObject(*this);
}

std::string StowObject::targetName(double mjd) const
{
  std::string name;
  name = m_name + std::string("  /  ");
  if(!m_use_corrections)name += "Encoder ";
  name += std::string("AZ:")+m_coords.longitude().degString(3);
  name += std::string("  EL:")+m_coords.latitude().degPMString(3);
  return name;
}

std::string StowObject::targetObjectType() const
{
  return std::string("Stow");
}

// ============================================================================
//
// CONSTANT VELOCITY TARGET OBJECT
//
// ============================================================================

CVObject::~CVObject()
{
  // nothing to see here
}
    
SphericalCoords 
CVObject::getAzEl(const double& mjd, const Angle& lmst, 
		  const SphericalCoords& earthPos) const
{
  SphericalCoords coords = FixedObject::getAzEl(mjd, lmst, earthPos);
  double el_deg = 
    coords.latitudeDeg()+m_el_speed*(mjd-m_mjd_zero)*86400;
  double az_deg = 
    coords.longitudeDeg()+m_az_speed*(mjd-m_mjd_zero)*86400;
  if(el_deg>89)el_deg=89;
  if(el_deg<1)el_deg=1;
  coords.setLatLongDeg(el_deg,az_deg);
  return coords;
}

TargetObject* CVObject::copy() const
{
  return new CVObject(*this);
}

std::string CVObject::targetName(double mjd) const
{
  std::string name;
  name = std::string("Constant velocity  /  ");
  name += std::string("AZ:")+doubleToString(m_az_speed,4);
  name += std::string("  EL:")+doubleToString(m_el_speed,4);
  return name;
}

std::string CVObject::targetObjectType() const
{
  return std::string("Constant velocity");
}

// ============================================================================
//
// SUN OBJECT - NEVER POINT AT ME
//
// ============================================================================

SunObject::~SunObject()
{
  // nothing to see here
}

SphericalCoords 
SunObject::getGalCoord(const double& mjd, const Angle& lmst, 
		       const SphericalCoords& earthPos) const
{
  SphericalCoords c = getRaDec(mjd,lmst,earthPos);
  Astro::raDecToGal(mjd, c);
  return c;
}

SphericalCoords 
SunObject::getRaDec(const double& mjd, const Angle& lmst, 
		    const SphericalCoords& earthPos) const
{
  SphericalCoords c;
  Astro::sunRaDecApparent(mjd, c);
  return c;
}

SphericalCoords 
SunObject::getAzEl(const double& mjd, const Angle& lmst, 
		   const SphericalCoords& earthPos) const
{
  SphericalCoords c=getRaDec(mjd,lmst,earthPos);
  Astro::apparentRaDecToAzEl(mjd, lmst, earthPos, c);
  return c;
}

bool SunObject::objectMovesInAzEl() const
{
  return true;
}

bool SunObject::useCorrections() const
{
  return true;
}

TargetObject* SunObject::copy() const
{
  return new SunObject(*this);
}

std::string SunObject::targetName(double mjd) const
{
  return std::string("SUN");
}

std::string SunObject::targetObjectType() const
{
  return std::string("SUN");  
}

// ============================================================================
//
// MOON TARGET OBJECT -- NEVER POINT AT ME!!
//
// ============================================================================

MoonObject::~MoonObject()
{
  // nothing to see here
}

SphericalCoords 
MoonObject::getGalCoord(const double& mjd, const Angle& lmst, 
		       const SphericalCoords& earthPos) const
{
  SphericalCoords c = getRaDec(mjd,lmst,earthPos);
  Astro::raDecToGal(mjd, c);
  return c;
}

SphericalCoords 
MoonObject::getRaDec(const double& mjd, const Angle& lmst, 
		    const SphericalCoords& earthPos) const
{
  SphericalCoords c;
  Astro::moonRaDecApparent(mjd, c);
  return c;
}

SphericalCoords 
MoonObject::getAzEl(const double& mjd, const Angle& lmst, 
		   const SphericalCoords& earthPos) const
{
  SphericalCoords c=getRaDec(mjd,lmst,earthPos);
  Astro::apparentRaDecToAzEl(mjd, lmst, earthPos, c);
  return c;
}

bool MoonObject::objectMovesInAzEl() const
{
  return true;
}

bool MoonObject::useCorrections() const
{
  return true;
}

TargetObject* MoonObject::copy() const
{
  return new MoonObject(*this);
}

std::string MoonObject::targetName(double mjd) const
{
  return std::string("MOON");
}

std::string MoonObject::targetObjectType() const
{
  return std::string("MOON");  
}

// ============================================================================
//
// OFFSETS
//
// ============================================================================

CoordinateOffset::~CoordinateOffset()
{
  // nothing to see here
}

void CoordinateOffset::applyOffsetForDB(const double& mjd, const SEphem::Angle& lmst, 
					const SEphem::SphericalCoords& earthPos,
					SEphem::SphericalCoords& coords) const
{
  applyOffset(mjd,lmst,earthPos,coords);
}

OnOffOffset::~OnOffOffset()
{
  // nothing to see here
}

void OnOffOffset::applyOffset(const double& mjd, const SEphem::Angle& lmst, 
			     const SEphem::SphericalCoords& earthPos,
			      SEphem::SphericalCoords& coords) const
{
  coords.rotatePhi(m_offset_time);
}  

CoordinateOffset* OnOffOffset::copy() const
{
  return new OnOffOffset(*this);
}

std::string OnOffOffset::offsetName(double mjd) const
{
  if(m_offset_time.rad() == 0)return std::string("ON");

  std::string name("OFF ");
  name += doubleToString(m_offset_time.hrsPM()*60/SID_RATE);
  name += std::string("min");
  return name;
}

WobbleOffset::~WobbleOffset()
{
  // nothing to see here
}

void WobbleOffset::applyOffset(const double& mjd, const SEphem::Angle& lmst, 
			     const SEphem::SphericalCoords& earthPos,
			       SEphem::SphericalCoords& coords) const
{
  SphericalCoords offset_coords(m_wobble_coords);
  offset_coords.rotateRad(coords.phi().rad(), coords.theta().rad(), 0);
  coords = offset_coords;
}

CoordinateOffset* WobbleOffset::copy() const
{
  return new WobbleOffset(*this);
}

std::string WobbleOffset::offsetName(double mjd) const
{
  if(m_wobble_coords.thetaRad() == 0)return std::string("ON");

  std::string name("WOBBLE ");
  name += doubleToString(m_wobble_coords.thetaDeg(),2);
  name += std::string("@");
  name += doubleToString(fmod(540-m_wobble_coords.phiDeg(),360),0);
  return name;
}


ElAzOffset::~ElAzOffset()
{
  // nothing to see here
}

void ElAzOffset::applyOffset(const double& mjd, const SEphem::Angle& lmst, 
			     const SEphem::SphericalCoords& earthPos,
			       SEphem::SphericalCoords& coords) const
{
  Astro::raDecToAzEl(lmst, earthPos, coords);
  // move mirrors by 1/2 the distance that we want the image moved on the camera.
  Angle hack;
  hack.setDeg(10.0);
  coords.setRad( coords.thetaRad() - 0.5*(m_elaz_coords.thetaRad()- hack.rad()), 
		 coords.phiRad() + 0.5*(m_elaz_coords.phiRad()-hack.rad())/sin(coords.thetaRad()) );
  Astro::azElToRaDec(lmst, earthPos, coords);

}

CoordinateOffset* ElAzOffset::copy() const
{
  return new ElAzOffset(*this);
}

std::string ElAzOffset::offsetName(double mjd) const
{
  if(m_elaz_coords.thetaRad() == 0 && m_elaz_coords.phiRad() == 0 )return std::string("ON");

  std::string name("UP/SIDE_OFFSET ");
  name += std::string("up=");
  name += doubleToString(m_elaz_coords.thetaDeg()-10.0,1);
  name += std::string(", side=");
  name += doubleToString(m_elaz_coords.phiDeg()-10.0,1);
  return name;
}


OrbitOffset::~OrbitOffset()
{
  // nothing to see here
}

void OrbitOffset::applyOffsetForDB(const double& mjd, const SEphem::Angle& lmst, 
				   const SEphem::SphericalCoords& earthPos,
				   SEphem::SphericalCoords& coords) const
{
  // nothing to see here
}

void OrbitOffset::applyOffset(const double& mjd, const SEphem::Angle& lmst, 
			     const SEphem::SphericalCoords& earthPos, 
			      SEphem::SphericalCoords& coords) const
{
  SphericalCoords offset_coords(m_orbit_coords);
  offset_coords.rotateRad(coords.phi().rad(), coords.theta().rad(), 
			  -SEphem::Angle::makeRot(mjd/m_orbit_period_day));
  coords = offset_coords;
}

CoordinateOffset* OrbitOffset::copy() const
{
  return new OrbitOffset(*this);
}

std::string OrbitOffset::offsetName(double mjd) const
{
  if(m_orbit_coords.thetaRad() == 0)return std::string("ON");

  std::string name("ORBIT ");
  name += doubleToString(fabs(m_orbit_period_day)*24.0*60.0);
  if(m_orbit_period_day<0)
    name += "min (CCW) - ";
  else
    name += "min - ";
  name += doubleToString(m_orbit_coords.thetaDeg(),2);
  name += "@"; 
  SEphem::Angle a(M_PI - (m_orbit_coords.phi() 
			  - SEphem::Angle::makeRot(mjd/m_orbit_period_day)));
  name += doubleToString(a.deg(),1);
  return name;  
}

// ============================================================================
//
// TARGET LIST
//
// ============================================================================

TargetList::~TargetList()
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);
  clear();
}

bool TargetList::loadFromFile(const std::string& filename, bool silent)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  clear();
  
  std::ifstream stream(filename.c_str());
  if(!stream.good())return false;

  while(stream)
    {
      std::vector<std::string> components;
      std::string component;
      char c;
      bool comment=false;

      stream.get(c);
      while((stream)&&(c!='\n'))
	{
	  if(c=='#')comment=true;

	  if((c==' ')||(c=='\t'))
	    {
	      if(component.length() > 0)
		{
		  components.push_back(component);
		  component.clear();
		}
	    }
	  else if(!comment)component.push_back(c);

	  stream.get(c);
	}
   
      if(component.length() > 0)
	{
	  components.push_back(component);
	  component.clear();
	}

      if(components.size()>=8)
	{
	  Angle ra;
	  Angle dec;
	  double epoch;

	  Angle p_ra;
	  Angle p_dec;
	  double p_epoch;

	  std::string name;

	  bool pnt=true;
	  
	  if(!(std::istringstream(components[components.size()-1]) >> p_epoch))
	    pnt=false;

	  if(!p_dec.setFromDMSString(components[components.size()-2]))
	    pnt=false;

	  if(!p_ra.setFromHMSString(components[components.size()-3]))
	    pnt=false;

	  if(!(std::istringstream(components[components.size()-4]) >> epoch))
	    continue;

	  if(!dec.setFromDMSString(components[components.size()-5]))
	    continue;

	  if(!ra.setFromHMSString(components[components.size()-6]))
	    continue;

	  // id=components[0]; - obsolete
	  name=components[1];
	  for(unsigned i=2; i<components.size()-6; i++)
	    {
	      name.append(" ");
	      name.append(components[i]);
	    }

#if 0
	  Debug::stream()
	    << '>' << name << '<' << ' ' 
	    << ra.hmsString() << ' ' << dec.dmsString() << ' ' 
	    << epoch << ' ' 
	    << p_ra.hmsString() << ' ' << p_dec.dmsString() << ' ' 
	    << p_epoch << std::endl;
#endif

	  RaDecObject* obj = 
	    new RaDecObject(SphericalCoords::makeLatLong(dec,ra),
			    Astro::julianEpochToMJD(epoch),name);

	  RaDecObject* p_obj=0;
	  if(pnt)p_obj = 
		   new RaDecObject(SphericalCoords::makeLatLong(p_dec,p_ra),
				   Astro::julianEpochToMJD(p_epoch),
				   std::string("Pointing [")+
				   name+std::string("]"));
	  
	  addTarget(name,"",obj,p_obj);
	}
    }
  sortTargets();
  return true;
}

void TargetList::clear()
{ 
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  for(iterator itarget=begin(); itarget!=end();itarget++)
    {
      delete((*itarget)->m_obj);
      delete((*itarget)->m_pnt);
      delete(*itarget);
    }
  m_list.clear();
}

const TargetList& TargetList::operator= (const TargetList& o)
{
  RegisterThisFunction fnguard(__PRETTY_FUNCTION__);

  clear();
  m_list.reserve(o.m_list.size());
  for(const_iterator itarget = o.begin(); itarget!=o.end(); itarget++)
    {
      RaDecObject* obj = new RaDecObject(*(*itarget)->m_obj);
      RaDecObject* pnt = 0;
      if((*itarget)->m_pnt)pnt = new RaDecObject(*(*itarget)->m_pnt);
      TargetListItem* item =
	new TargetListItem((*itarget)->m_name, (*itarget)->m_desc, obj, pnt);
      m_list.push_back(item);
    }
  return *this;
}

#if TESTMAIN
#include<iostream>

int main()
{
  double T = (2462088.69-2451545)/36525;
  //double T = (2446895.5-2451545)/36525;

  Angle dpsi;
  Angle deps;
  Angle eps0;
  
  nutation_angles(T,dpsi,deps);
  mean_obliquity(T,eps0);

  Angle eps = Angle::makeRad(eps0.rad()+deps.rad());

  Angle ra(0);
  Angle dec(Angle::sc_halfPi);
  ra.setFromHMSString("2:46:11.331");
  dec.setFromDMSString("+49:20:54.54");
  SphericalCoords eq0 = SphericalCoords::makeLatLong(dec,ra);
  SphericalCoords eq1 = eq0;

  raDecToEcliptic(eps0,eq1);

  Angle ablong;
  Angle ablat;
  aberration(T,eq1,ablong,ablat);

  eq1.setPhi(eq1.phi()+dpsi+ablong);
  eq1.setTheta(eq1.theta()-ablat);
  eclipticToRaDec(eps,eq1);

  Debug::stream()
    << "T:     " << T << std::endl
    << "DPsi:  " << dpsi.dmsString(3) << std::endl
    << "DEps:  " << deps.dmsString(3) << std::endl
    << "Eps0:  " << eps0.dmsString(3) << std::endl
    << "Eps:   " << eps.dmsString(3) << std::endl
    << "ALong: " << ablong.dmsString(3) << std::endl
    << "ALat:  " << ablat.dmsString(3) << std::endl
    << "RA0:   " << eq0.phi().hmsString(3) << std::endl
    << "Dec0:  " << eq0.latitude().dmsString(3) << std::endl
    << "RA1:   " << eq1.phi().hmsString(3) << std::endl
    << "Dec1:  " << eq1.latitude().dmsString(3) << std::endl
    << "DRa:   " << Angle::makeRad(eq1.phi()-eq0.phi()).dmsString(3) 
    << std::endl
    << "DDec:  " <<Angle::makeRad(eq0.theta()-eq1.theta()).dmsString(3)
    << std::endl;
  
  return 1;
}

#endif

