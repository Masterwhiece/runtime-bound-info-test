
  
#Makefile for llmpx runtime library
CFLAGS=-fPIC -Wall -g -Ofast -march=native

all: static dynamic

dynamic:
	${CC} ${CFLAGS} -fPIC -DPIC -c compareOffsetNSize.c -o compareOffsetNSizeOut
	${CC} ${CFLAGS} -fPIC -DPIC -c exitFunc.c -o exitFuncOut
	${CC} ${LDFLAGS} -shared compareOffsetNSize.o -o compareOffsetNSize.so
	${CC} ${LDFLAGS} -shared exitFunc.o -o exitFunc.so

static:
	${CC} ${CFLAGS} -c compareOffsetNSize.c -o compareOffsetNSize.o
	${CC} ${CFLAGS} -c exitFunc.c -o exitFunc.o
	rm -f libllmpx_rt.a
	${AR} -rcs compareOffsetNSize.a compareOffsetNSize.o
	${AR} -rcs exitFunc.a exitFunc.o
	ranlib compareOffsetNSize.a
	ranlib exitFunc.a

clean:
	rm -f compareOffsetNSize.o compareOffsetNSize.so
	rm -f exitFunc.o exitFunc.so
	rm -f compareOffsetNSize.o compareOffsetNSize.a
	rm -f exitFunc.o exitFunc.a
