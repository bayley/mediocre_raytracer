CXX=g++
CPPFLAGS=-O3 -I.
DEPS = geom.h bmp.h
OBJ = main.o bmp.o geom.o
LIBS = -lm -lembree3

%.o: %.c $(DEPS)
	$(CXX) -c -o $@ $< $(CPPFLAGS)

embree_test: $(OBJ)
	$(CXX) -o $@ $^ $(CPPFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o embree_test
