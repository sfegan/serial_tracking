OBJECTS = Angle.o Astro.o CorrectionParameters.o

CXXFLAGS = -g -Wall
LDFLAGS = -L.
#LD = g++
#CXX = g++

all: libSEphem.a

moon.h: moon1.dat moon2.dat moon.pl
	./moon.pl > $@

moon_r.h: moon1.dat moon_r.pl	
	./moon_r.pl > $@

nutation.h: nutation.dat nutation.pl
	./nutation.pl > $@

Astro.o: moon.h moon_r.h nutation.h

test: test.o 
	$(LD) $(LDFLAGS) -o $@ $^ -lSEphem

clean:
	$(RM) $(OBJECTS) libSEphem.a moon.h nutation.h *~

libSEphem.a: $(OBJECTS)
	$(AR) r $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<
