DPU_DIR := dpu
HOST_DIR := host
BUILDDIR ?= build
NR_TASKLETS ?= 1
NR_DPUS ?= 1

FLAGS ?= 

define conf_filename
	${BUILDDIR}/.NR_DPUS_$(1)_NR_TASKLETS_$(2).conf
endef
CONF := $(call conf_filename,${NR_DPUS},${NR_TASKLETS})

HOST_TARGET := ${BUILDDIR}/host
DPU_TARGET := ${BUILDDIR}/wfa_dpu

COMMON_INCLUDES := common
HOST_SOURCES := $(wildcard ${HOST_DIR}/*.c)
DPU_SOURCES := $(wildcard ${DPU_DIR}/wfa.c ${DPU_DIR}/wfa_backtracing.c ${DPU_DIR}/dpu_allocator_*.c)

.PHONY: all clean test

__dirs := $(shell mkdir -p ${BUILDDIR})

COMMON_FLAGS := -I${COMMON_INCLUDES} -Wall
HOST_FLAGS := ${COMMON_FLAGS} -std=c11 -O3 `dpu-pkg-config --cflags --libs dpu` -DNR_TASKLETS=${NR_TASKLETS} -DNR_DPUS=${NR_DPUS} ${FLAGS}
DPU_FLAGS := ${COMMON_FLAGS} -O3 ${FLAGS} -DNR_TASKLETS=${NR_TASKLETS}

all: ${HOST_TARGET}  ${DPU_TARGET}

${CONF}:
	$(RM) $(call conf_filename,*,*)
	touch ${CONF}

${HOST_TARGET}: ${HOST_SOURCES} ${COMMON_INCLUDES} ${CONF}
	$(CC) -o $@ ${HOST_SOURCES} ${HOST_FLAGS}

${DPU_TARGET}: ${DPU_SOURCES} ${COMMON_INCLUDES} ${CONF}
	dpu-upmem-dpurte-clang ${DPU_FLAGS} -o $@ ${DPU_SOURCES}

TEST_HOST_TARGET := ${BUILDDIR}/test_transfer_host
TEST_DPU_TARGET := ${BUILDDIR}/test_transfer_dpu

test_transfer: ${TEST_HOST_TARGET} ${TEST_DPU_TARGET}
	./${TEST_HOST_TARGET}

${TEST_HOST_TARGET}: host/test_transfer.cpp ${CONF}
	g++ -o $@ host/test_transfer.cpp -I${COMMON_INCLUDES} -std=c++11 -O3 `dpu-pkg-config --cflags --libs dpu` -DNR_TASKLETS=${NR_TASKLETS} -DNR_DPUS=${NR_DPUS} ${FLAGS}

${TEST_DPU_TARGET}: dpu/test_transfer.c ${CONF}
	dpu-upmem-dpurte-clang -I${COMMON_INCLUDES} -O3 -DNR_TASKLETS=${NR_TASKLETS} -o $@ dpu/test_transfer.c

clean:
	$(RM) -r $(BUILDDIR)

test_c: ${HOST_TARGET} ${DPU_TARGET}
	./${HOST_TARGET}


test: test_c

