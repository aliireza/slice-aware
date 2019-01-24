CC= gcc
CXX=  g++
CFLAGS= -lm
CXXFLAGS= -std=c++14
LIST= uniform_gen zipf_gen
TARGETDIR=build

compile: ${LIST}

uniform_gen: uniform_gen.cc
	@mkdir -p ${TARGETDIR} 
	${CXX} ${CXXFLAGS} uniform_gen.cc -o ${TARGETDIR}/uniform_gen

zipf_gen: zipf.h zipf-gen-mica.c
	@mkdir -p ${TARGETDIR} 
	${CC} zipf-gen-mica.c ${CFLAGS} -o ${TARGETDIR}/zipf

clean:
	rm -fr ${TARGETDIR}
