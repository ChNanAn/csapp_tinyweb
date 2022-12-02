CXX=g++
CFlags=-std=c++11 -I .. 
Target=adder

$(Target):*.cpp
	$(CXX) $(CFlags) $< -o $@


.PHONY:clean
clean:
	rm -f *.o adder *~
	
