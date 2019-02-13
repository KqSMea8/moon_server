CXX:= g++
CXXFLAGS:= -Wall -W -D__STDC_LIMIT_MACROS -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H
LDFLAGS:=
DEBUGFLAGS:= -g3 -gdwarf-2 -ggdb
OPTFLAG:= -O2
AR:= ar
ARFLAGS:= rcs

includedir:=  -I ../include/mysql
libdir:= -L ../lib/mysql
#libs:= -lmysqlclient -lpthread -lm -lssl -ldl
libs:= -lpthread -lm -lssl -ldl

progname:= moonserver
libarchive := libmoon.a
moonobj:= moon_util.o moon_config.o moon_log.o moon_server.o 
mainobj:= main.o


all: $(progname) 

$(progname): $(moonobj) $(mainobj)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(DEBUGFLAGS) $(includedir) $(libdir) $(libs)
%.o: %.cc
	$(CXX) -c $(CXXFLAGS) $(DEBUGFLAGS) $(includedir) $<


clean:
	rm -rf moonserver *.o
moon_util.o: moon_util.cc moon_util.cc  moon_log.h
moon_config.o: moon_config.cc moon_config.h moon_log.h
moon_server.o: moon_server.cc moon_server.h moon_log.h
main.o: main.cc moon_config.h moon_log.h moon_server.h
