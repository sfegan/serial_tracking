//-*-mode:c++; mode:font-lock;-*-

/**
 * \file test.cpp
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
 * $Date: 2006/04/04 17:06:58 $
 * $Revision: 2.0 $
 * $Tag$
 *
 **/

#include<sstream>
#include<iostream>
#include<iomanip>
#include<cmath>

#include"Angle.h"
#include"SphericalCoords.h"

using namespace SEphem;

int main(int argc, char**argv)
{
#if 0
  double b=90;
  double l=0;
  if(argc>1)std::istringstream(*(argv+1)) >> b;
  if(argc>2)std::istringstream(*(argv+2)) >> l;
  SphericalCoords z=SphericalCoords::makeDeg(b,l);
  for(int i=0;i<=10;i++)
    {
      double t = double(i)*M_PI/10.0;
      Angle ta(t);
      for(int j=-10;j<=10;j++)
	{
	  double p = double(j)*M_PI/10.0;
	  Angle pa(p);
	  SphericalCoords c(ta,pa);
	  //	  c.rotate(z,0);//SphericalCoords::makeDeg(45,-90),0);

	  Angle s;
	  Angle d;
	  c.separationAndDirectionTo(z,s,d);
	  std::cout << c.latitudeDeg() << '\t' 
		    << Angle::toDeg(Angle::toRadPM(c.longitudeRad())) << '\t'
		    << s.deg() << '\t'
		    << d.deg() << '\t'
		    << std::endl;
	}
    }
#endif
#if 0
  for(int i=-1;i<40;i++)
    {
      //      Angle th=Angle::sc_Pi-exp(-double(i));
      Angle th=exp(-double(i));
      SphericalCoords c;
      c.rotate(0,th,0);
      Angle di=c.separation(SphericalCoords());
      Angle di2=SphericalCoords().separation(c);
      std::cout << std::fixed 
		<< std::setw(20) << std::setprecision(18) << cos(th) << ' ' 
		<< std::setw(20) << std::setprecision(18) << th << ' ' 
		<< std::setw(20) << std::setprecision(18) << c.theta() << ' ' 
		<< std::setw(20) << std::setprecision(18) << di << ' ' 
		<< std::setw(20) << std::setprecision(18) << di2 << ' ' 
		<< std::setw(6) << std::setprecision(2) 
		<< 100*fabs(th-di)/(th) << std::endl;
    }
#endif
#if 0
  SphericalCoords c0 = SphericalCoords::makeDeg(30,0);
  SphericalCoords cE = SphericalCoords::makeDeg(30,1);
  SphericalCoords cN = SphericalCoords::makeDeg(29,0);
  SphericalCoords cW = SphericalCoords::makeDeg(30,-1);
  SphericalCoords cS = SphericalCoords::makeDeg(31,0);
  std::cout << "E: " << c0.compassDirectionTo(cE).deg() << std::endl
	    << "N: " << c0.compassDirectionTo(cN).deg() << std::endl
	    << "W: " << c0.compassDirectionTo(cW).deg() << std::endl
	    << "S: " << c0.compassDirectionTo(cS).deg() << std::endl;
#endif
#if 1
  for(double d=0;d<1.0; d+=0.01)
    {
      Angle a = Angle::makeDeg(d);
      unsigned long bar = a.bar(16,false);
      Angle b = Angle::makeBAR(bar,16,false);
      std::cerr << a.degString(4) << ' ' 
		<< std::hex << std::setw(4) << std::setfill('0') << bar << ' '
		<< b.degString(4) 
		<< std::endl;
    }
#endif
  return 0;
}
      
  
  
