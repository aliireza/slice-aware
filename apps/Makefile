CC= gcc
CFLAGS=
LIST= mapping_finder L3_access poormans_multicore_slice poormans_multicore_noslice
LIBDIR= ../lib
LIB= ${LIBDIR}/memory-utils.c ${LIBDIR}/msr-utils.c ${LIBDIR}/cache-utils.c
TARGETDIR=build
SHELL:=/bin/bash

compile: ${LIST}

mapping_finder: check_cpu mapping_finder.c ${LIB}
	@mkdir -p $(TARGETDIR)
	${CC} ${CFLAGS} -o $(TARGETDIR)/mapping_finder mapping_finder.c 

L3_access: check_cpu L3_access_measurement.c ${LIB}
	@mkdir -p $(TARGETDIR)
	${CC} ${CFLAGS} -o $(TARGETDIR)/L3_access L3_access_measurement.c 

poormans_multicore_slice: check_cpu poormans_multicore_slice.c ${LIB}
	@mkdir -p $(TARGETDIR)
	${CC} ${CFLAGS} -pthread -o $(TARGETDIR)/poormans_multicore_slice poormans_multicore_slice.c

poormans_multicore_noslice: check_cpu poormans_multicore_noslice.c ${LIB}
	@mkdir -p $(TARGETDIR)
	${CC} ${CFLAGS} -pthread -o $(TARGETDIR)/poormans_multicore_noslice poormans_multicore_noslice.c

check_cpu:
	source ${LIBDIR}/check_cpu.sh && ${LIBDIR}/check_cpu.sh
clean:
	rm -fr ${TARGETDIR}
