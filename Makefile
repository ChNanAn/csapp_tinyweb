CXX=g++
CXXFLAGS= -O2 -Wall -std=c++11 -I .
src=$(wildcard ./*.c)
objs=$(patsubst %.c, %.o, $(src))
target=tiny


all:$(target) cgi

$(target):$(objs) 
	$(CXX) $(CXXFLAGS)  $(objs) -o $(target)
%.o: %.c
	$(CXX) -c $(CXXFLAGS)  $< -o $@

cgi:
	(cd cgi-bin;make)

#tinyweb.o:tinyweb.c
#$(CXX) -c $(CXXFLAGS)  tinyweb.c -o tinyweb.o

#tinymain.o:tinymain.c
#$(CXX) -c $(CXXFLAGS) tinymain.c -o tinymain.o
#.PHONY:clean
clean:
	rm -f *.o tiny *~
	(cd cgi-bin;make clean)