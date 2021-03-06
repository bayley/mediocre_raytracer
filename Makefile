CXX=g++
CPPFLAGS=-O3 -I.
DEPS = geom.h bmp.h bmpc.h brdf.h RTObject.h
OBJ = main.o bmp.o geom.o bmpc.o brdf.o RTObject.o
LIBS = -lm -lembree3

%.o: %.c $(DEPS)
	$(CXX) -c -o $@ $< $(CPPFLAGS)

embree_test: $(OBJ)
	$(CXX) -o $@ $^ $(CPPFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o embree_test
