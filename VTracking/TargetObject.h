//-*-mode:c++; mode:font-lock;-*-

/**
 * \file TargetObject.h
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
 * $Revision: 2.12 $
 * $Tag$
 *
 **/

#ifndef VTRACKING_TARGETOBJECT_H
#define VTRACKING_TARGETOBJECT_H

#include<string>
#include<vector>
#include<algorithm>

#include<Angle.h>
#include<SphericalCoords.h>

#define SID_RATE 1.00273790935

namespace VTracking
{
  // ==========================================================================
  // TARGET OBJECT
  // ==========================================================================

  class TargetObject
  {
  public:
    virtual ~TargetObject();

    virtual SEphem::SphericalCoords 
    getGalCoord(const double& mjd, const SEphem::Angle& lmst, 
		const SEphem::SphericalCoords& earthPos) const = 0;

    virtual SEphem::SphericalCoords 
    getRaDec(const double& mjd, const SEphem::Angle& lmst, 
	     const SEphem::SphericalCoords& earthPos) const = 0;

    virtual SEphem::SphericalCoords 
    getAzEl(const double& mjd, const SEphem::Angle& lmst, 
	     const SEphem::SphericalCoords& earthPos) const = 0;
	   

    virtual bool objectMovesInAzEl() const = 0;
    virtual bool useCorrections() const = 0;
    virtual TargetObject* copy() const = 0;

    virtual std::string targetName(double mjd) const = 0;
    virtual std::string targetObjectType() const = 0;
  };

  class CoordinateOffset
  {
  public:
    virtual ~CoordinateOffset();
    virtual void applyOffsetForDB(const double& mjd, const SEphem::Angle& lmst, 
			     const SEphem::SphericalCoords& earthPos,
				  SEphem::SphericalCoords& coords) const;
    virtual void applyOffset(const double& mjd, const SEphem::Angle& lmst, 
			     const SEphem::SphericalCoords& earthPos,
			     SEphem::SphericalCoords& coords) const = 0;
    virtual CoordinateOffset* copy() const = 0;
    virtual std::string offsetName(double mjd) const = 0;
  };
  
  // ==========================================================================
  // RA/DEC TARGET OBJECT
  // ==========================================================================


  class RaDecObject: public TargetObject
  {
  public:
    RaDecObject(): 
      TargetObject(), m_coords(), m_epoch(), m_name(),
      m_offset() { /* nothing to see here */ }
    RaDecObject(const RaDecObject& obj): 
      TargetObject(), m_coords(obj.m_coords), m_epoch(obj.m_epoch),
      m_name(obj.m_name), m_offset(0) 
    { if(obj.m_offset)m_offset = obj.m_offset->copy(); }
    RaDecObject(const SEphem::SphericalCoords& coords, double epoch,
		const std::string& name = std::string(),
		CoordinateOffset* offset = 0):
      TargetObject(), m_coords(coords), m_epoch(epoch), m_name(name),
      m_offset(offset) { }
    virtual ~RaDecObject();

    virtual SEphem::SphericalCoords 
    getGalCoord(const double& mjd, const SEphem::Angle& lmst, 
		const SEphem::SphericalCoords& earthPos) const;

    virtual SEphem::SphericalCoords 
    getRaDec(const double& mjd, const SEphem::Angle& lmst, 
	     const SEphem::SphericalCoords& earthPos) const;

    virtual SEphem::SphericalCoords 
    getAzEl(const double& mjd, const SEphem::Angle& lmst, 
	    const SEphem::SphericalCoords& earthPos) const;

    virtual bool objectMovesInAzEl() const;
    virtual bool useCorrections() const;
    virtual TargetObject* copy() const;

    SEphem::SphericalCoords getDBReportedRaDec(double mjd, const SEphem::Angle& lmst, 
	    const SEphem::SphericalCoords& earthPos) const; 
    
    const SEphem::SphericalCoords& coords() const { return m_coords; }
    const double& epoch() const { return m_epoch; }
    const CoordinateOffset* offset() const { return m_offset; }
    const std::string& name() const { return m_name; }

    virtual std::string targetName(double mjd) const;
    virtual std::string targetObjectType() const;

  protected:
    SEphem::SphericalCoords           m_coords;
    double                            m_epoch;
    std::string                       m_name;
    CoordinateOffset*                 m_offset;
  };

  // ==========================================================================
  // "FIXED" TARGET OBJECT
  // ==========================================================================

  class FixedObject: public TargetObject
  {
  public:
    virtual ~FixedObject();

    virtual SEphem::SphericalCoords 
    getGalCoord(const double& mjd, const SEphem::Angle& lmst, 
		const SEphem::SphericalCoords& earthPos) const;

    virtual SEphem::SphericalCoords 
    getRaDec(const double& mjd, const SEphem::Angle& lmst, 
	     const SEphem::SphericalCoords& earthPos) const;

    virtual SEphem::SphericalCoords 
    getAzEl(const double& mjd, const SEphem::Angle& lmst, 
	    const SEphem::SphericalCoords& earthPos) const;

    virtual bool objectMovesInAzEl() const;
    virtual bool useCorrections() const;

    const SEphem::SphericalCoords& coords() const { return m_coords; }

  protected:
    FixedObject(): 
      TargetObject(), m_coords(), m_use_corrections(), m_stop_at_target() {}
    FixedObject(const FixedObject& obj):
      TargetObject(), m_coords(obj.m_coords), 
      m_use_corrections(obj.m_use_corrections),
      m_stop_at_target(obj.m_stop_at_target) {}
    FixedObject(const SEphem::SphericalCoords& coords, 
	       bool use_corrections=true, bool stop_at_target=true):
      TargetObject(), m_coords(coords), m_use_corrections(use_corrections),
      m_stop_at_target(stop_at_target) {}

    SEphem::SphericalCoords           m_coords;
    bool                              m_use_corrections;
    bool                              m_stop_at_target;
  };

  // ==========================================================================
  // UNKNOWN TARGET OBJECT
  // ==========================================================================

  class UnknownObject: public FixedObject
  {
  public:
    UnknownObject(): FixedObject() { }
    UnknownObject(const UnknownObject& obj): FixedObject(obj) { }
    UnknownObject(const SEphem::SphericalCoords& coords, 
	       bool use_corrections=true, bool stop_at_target=true):
      FixedObject(coords,use_corrections,stop_at_target) {}
    virtual ~UnknownObject();
    virtual TargetObject* copy() const;

    virtual std::string targetName(double mjd) const;
    virtual std::string targetObjectType() const;
  };

  // ==========================================================================
  // AZ/EL TARGET OBJECT
  // ==========================================================================

  class AzElObject: public FixedObject
  {
  public:
    AzElObject(): FixedObject() { }
    AzElObject(const AzElObject& obj): FixedObject(obj) { }
    AzElObject(const SEphem::SphericalCoords& coords, 
	       bool use_corrections=true, bool stop_at_target=true):
      FixedObject(coords,use_corrections,stop_at_target) {}
    virtual ~AzElObject();
    virtual TargetObject* copy() const;

    virtual std::string targetName(double mjd) const;
    virtual std::string targetObjectType() const;
  };

  // ==========================================================================
  // STOW TARGET OBJECT
  // ==========================================================================

  class StowObject: public FixedObject
  {
  public:
    StowObject(): FixedObject(), m_name() { /* N T S H */ }
    StowObject(const std::string& name, const StowObject& obj): 
      FixedObject(obj), m_name(name) { /* N T S H */ }
    StowObject(const std::string& name, const SEphem::SphericalCoords& coords):
      FixedObject(coords, false, true), m_name(name) { /* N T S H */ }
    virtual ~StowObject();

    virtual TargetObject* copy() const;

    virtual std::string targetName(double mjd) const;
    virtual std::string targetObjectType() const;

    const std::string& name() const { return m_name; }
  private:
    std::string                       m_name;
  };

  // ==========================================================================
  // CONSTANT VELOCITY TARGET OBJECT
  // ==========================================================================

  typedef std::vector<StowObject> StowObjectVector;

  // constant velocity object
  class CVObject: public FixedObject
  {
  public:
    CVObject(): FixedObject(), m_az_speed(0), m_el_speed(0), m_mjd_zero(0) { }
    CVObject(const CVObject& obj): 
      FixedObject(obj), 
      m_az_speed(obj.m_az_speed), m_el_speed(obj.m_el_speed),
      m_mjd_zero(obj.m_mjd_zero) { }
    CVObject(const SEphem::SphericalCoords& initial_coords,
	     double az_speed, double el_speed, double mjd_zero):
      FixedObject(initial_coords, false, false), 
      m_az_speed(az_speed), m_el_speed(el_speed), m_mjd_zero(mjd_zero) { }
    virtual ~CVObject();
    
    virtual SEphem::SphericalCoords 
    getAzEl(const double& mjd, const SEphem::Angle& lmst, 
	    const SEphem::SphericalCoords& earthPos) const;
    virtual TargetObject* copy() const;

    double azSpeed() const { return m_az_speed; }
    double elSpeed() const { return m_el_speed; }
    double mjdZero() const { return m_mjd_zero; }

    virtual std::string targetName(double mjd) const;
    virtual std::string targetObjectType() const;

  protected:
    double                            m_az_speed;
    double                            m_el_speed;
    double                            m_mjd_zero;
  };

  // ==========================================================================
  // SUN TARGET OBJECT -- NEVER POINT AT ME!!
  // ==========================================================================

  class SunObject: public TargetObject
  {
  public:
    SunObject(): TargetObject() { /* nothing to see here */ }
    SunObject(const SunObject& obj): 
      TargetObject(*this) { /* nothing to see here */ }

    virtual ~SunObject();

    virtual SEphem::SphericalCoords 
    getGalCoord(const double& mjd, const SEphem::Angle& lmst, 
		const SEphem::SphericalCoords& earthPos) const;

    virtual SEphem::SphericalCoords 
    getRaDec(const double& mjd, const SEphem::Angle& lmst, 
	     const SEphem::SphericalCoords& earthPos) const;

    virtual SEphem::SphericalCoords 
    getAzEl(const double& mjd, const SEphem::Angle& lmst, 
	    const SEphem::SphericalCoords& earthPos) const;

    virtual bool objectMovesInAzEl() const;
    virtual bool useCorrections() const;
    virtual TargetObject* copy() const;

    virtual std::string targetName(double mjd) const;
    virtual std::string targetObjectType() const;
  };

  // ==========================================================================
  // MOON TARGET OBJECT -- NEVER POINT AT ME WITH HV ON
  // ==========================================================================

  class MoonObject: public TargetObject
  {
  public:
    MoonObject(): TargetObject() { /* nothing to see here */ }
    MoonObject(const MoonObject& obj): 
      TargetObject(*this) { /* nothing to see here */ }

    virtual ~MoonObject();

    virtual SEphem::SphericalCoords 
    getGalCoord(const double& mjd, const SEphem::Angle& lmst, 
		const SEphem::SphericalCoords& earthPos) const;

    virtual SEphem::SphericalCoords 
    getRaDec(const double& mjd, const SEphem::Angle& lmst, 
	     const SEphem::SphericalCoords& earthPos) const;

    virtual SEphem::SphericalCoords 
    getAzEl(const double& mjd, const SEphem::Angle& lmst, 
	    const SEphem::SphericalCoords& earthPos) const;

    virtual bool objectMovesInAzEl() const;
    virtual bool useCorrections() const;
    virtual TargetObject* copy() const;

    virtual std::string targetName(double mjd) const;
    virtual std::string targetObjectType() const;
  };

  // ===========================================================================
  // OFFSETS
  // ===========================================================================

  class OnOffOffset: public CoordinateOffset
  {
  public:
    OnOffOffset(const SEphem::Angle& offset_time):
      CoordinateOffset(), m_offset_time(offset_time) { }
    OnOffOffset(const OnOffOffset& o):
      CoordinateOffset(*this), m_offset_time(o.m_offset_time) { }
    virtual ~OnOffOffset();
    virtual void applyOffset(const double& mjd, const SEphem::Angle& lmst, 
			     const SEphem::SphericalCoords& earthPos,
			     SEphem::SphericalCoords& coords) const;
    virtual CoordinateOffset* copy() const;
    virtual std::string offsetName(double mjd) const;
    const SEphem::Angle& getOffsetTime() const { return m_offset_time; }
  private:
    OnOffOffset& operator=(const OnOffOffset&);
    SEphem::Angle m_offset_time;
  };

  class WobbleOffset: public CoordinateOffset
  {
  public:
    WobbleOffset(const SEphem::SphericalCoords& wobble_coords):
      CoordinateOffset(), m_wobble_coords(wobble_coords) { }
    WobbleOffset(const WobbleOffset& o):
      CoordinateOffset(*this), m_wobble_coords(o.m_wobble_coords) { }
    virtual ~WobbleOffset();
    virtual void applyOffset(const double& mjd, const SEphem::Angle& lmst, 
			     const SEphem::SphericalCoords& earthPos,
			     SEphem::SphericalCoords& coords) const;
    virtual CoordinateOffset* copy() const;
    virtual std::string offsetName(double mjd) const;
    const SEphem::SphericalCoords& getWobbleCoords() const 
    { return m_wobble_coords; }
  private:
    WobbleOffset& operator=(const WobbleOffset&);
    SEphem::SphericalCoords m_wobble_coords;
  };
 
  class ElAzOffset: public CoordinateOffset
  {
  public:
    ElAzOffset(const SEphem::SphericalCoords& elaz_coords):
      CoordinateOffset(), m_elaz_coords(elaz_coords) { }
    ElAzOffset(const ElAzOffset& o):
      CoordinateOffset(*this), m_elaz_coords(o.m_elaz_coords) { }
    virtual ~ElAzOffset();
    virtual void applyOffset(const double& mjd, const SEphem::Angle& lmst, 
			     const SEphem::SphericalCoords& earthPos,
			     SEphem::SphericalCoords& coords) const;
    virtual CoordinateOffset* copy() const;
    virtual std::string offsetName(double mjd) const;
    const SEphem::SphericalCoords& getElAzCoords() const 
    { return m_elaz_coords; }
  private:
    ElAzOffset& operator=(const ElAzOffset&);
    SEphem::SphericalCoords m_elaz_coords;
  };

  class OrbitOffset: public CoordinateOffset
  {
  public:
    OrbitOffset(const SEphem::SphericalCoords& orbit_coords,
		double orbit_period_day):
      CoordinateOffset(), m_orbit_coords(orbit_coords),
      m_orbit_period_day(orbit_period_day) { }
    OrbitOffset(const OrbitOffset& o):
      CoordinateOffset(), m_orbit_coords(o.m_orbit_coords),
      m_orbit_period_day(o.m_orbit_period_day) { }
    virtual ~OrbitOffset();
    virtual void applyOffsetForDB(const double& mjd, const SEphem::Angle& lmst, 
			     const SEphem::SphericalCoords& earthPos,
				  SEphem::SphericalCoords& coords) const;
    virtual void applyOffset(const double& mjd, const SEphem::Angle& lmst, 
			     const SEphem::SphericalCoords& earthPos,
			     SEphem::SphericalCoords& coords) const;
    virtual CoordinateOffset* copy() const;
    virtual std::string offsetName(double mjd) const;
    const SEphem::SphericalCoords& getOrbitCoords() const 
    { return m_orbit_coords; }
    double getOffsetPeriodDay() const { return m_orbit_period_day; }
  private:
    WobbleOffset& operator=(const WobbleOffset&);
    SEphem::SphericalCoords m_orbit_coords;
    double m_orbit_period_day;
  };
 
  // ==========================================================================
  // TARGET LIST
  // ==========================================================================

  class TargetList
  {
  public:
    class TargetListItem
    {
    public:
      TargetListItem(const std::string& name, const std::string& desc, 
		     RaDecObject* obj, RaDecObject* p_obj)
	: m_name(name), m_desc(desc), m_obj(obj), m_pnt(p_obj) {}
      std::string                     m_name;
      std::string                     m_desc;
      VTracking::RaDecObject*         m_obj;
      VTracking::RaDecObject*         m_pnt;
      bool operator<(const TargetListItem& o) const 
      { return lt(m_name, o.m_name); }

      static bool lt(const std::string& a, const std::string& b)
      {
	const unsigned na = a.size();
	const unsigned nb = b.size();
	unsigned ia = 0;
	unsigned ib = 0;
	while(1)
	  {
	    while(ia<na && isspace(a[ia]))ia++;
	    while(ib<nb && isspace(b[ib]))ib++;
	    if(ia<na && ib<nb)
	      {
		int ca = tolower(a[ia]);
		int cb = tolower(b[ib]);
		if(ca < cb)return true;
		else if(cb < ca)return false;
		ia++;
		ib++;
	      }
	    else break;
	  }

	while(ia<na && isspace(a[ia]))ia++;
	while(ib<nb && isspace(b[ib]))ib++;

	if(ia==na && ib<nb)return true;
	else return false;
      }
    };

    class TargetListItemCmp
    {
    public:
      bool operator()(const TargetListItem* a, const TargetListItem* b)
      {
	return *a < *b;
      }
    };

    typedef std::vector<TargetListItem*>::iterator iterator;
    typedef std::vector<TargetListItem*>::const_iterator const_iterator;

    TargetList(): m_list() {}
    virtual ~TargetList();
    
    unsigned size() const { return m_list.size(); }
    void clear();

    iterator begin() { return m_list.begin(); }
    const_iterator begin() const { return m_list.begin(); }
    iterator end() { return m_list.end(); }
    const_iterator end() const { return m_list.end(); }
    void push_back(TargetListItem* item) { m_list.push_back(item); }
    const TargetListItem* operator[] (std::vector<TargetListItem>::size_type i)
      const { return m_list[i]; }

    void addTargetItem(TargetListItem* item) { push_back(item); }
    void addTarget(const std::string& name, const std::string& desc,
		   RaDecObject* obj, RaDecObject* p_obj)
    { push_back(new TargetListItem(name,desc,obj,p_obj)); }
    
    void deleteTarget(unsigned i) 
    { delete m_list[i]; m_list.erase(begin()+i); }

    const TargetListItem* target(unsigned i) const { return m_list[i]; }
    
    bool loadFromFile(const std::string& filename, bool silent=false);

    void sortTargets() { std::sort(begin(),end(),TargetListItemCmp()); }

    const TargetList& operator= (const TargetList& o);

  private:
    TargetList(const TargetList& o);

    std::vector<TargetListItem*> m_list;
  };
  
  void targetSpeed(SEphem::SphericalCoords target_azel,
		   const SEphem::SphericalCoords& target_pos,
		   double& az_speed, double& el_speed);

} // namespace VTracking

#endif // VTRACKING_TARGETOBJECT_H
