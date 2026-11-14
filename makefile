# makefile of the TEM core
# 20261114 Changed: sources are in src/physics, src/io and src/driver (was tem_core/); file names are unique across the three.
# 20241109 Changed: one makefile builds xtem, serial and MPI (replaces the
#          Makefile_*.xtem files).
# 20241109 Updated: build options stamped per build directory, tests.
# 20241110 Changed: the driver is xtem_gds.cpp.
# 20241110 Changed: site/, COMPILE, RTS and the MODE and COMPILER options removed; CXXFLAGS and CXX set the compiler.
# 20241109 Changed: MAXCYCLE option, the daily target and build_daily/ removed: one build runs monthly and
#          daily steps.
#
#   make                serial build: build/xtem
#   make mpi            MPI build: build_mpi/xtem (cells are shared between ranks)
#   make test           build and run the unit tests
#   make clean          remove build and build_mpi
#
# Compile-time options, for example: make mpi -j16 OPEN_N=0 STORM=0
#   OPEN_N     1 open nitrogen cycle (default), 0 closed
#   STORM      1 include stochastic storm and hurricane damage (default), 0 omit it
#   FIRE       1 include the wildfire code (default), 0 omit it; a FIRE=0 executable stops when veg_fire is 1
#   NETCDF_PAR 1 reads the inputs with the HDF5 MPI-IO driver when use_netcdf_mpi is set (make mpi, parallel NetCDF); default 0
#   CXXFLAGS   default -O2; an unoptimised build gives different numbers. CXX defaults to g++ (mpicxx for make mpi)
# Changing any option rebuilds the objects of that build directory.
#
# Needs a C++17 compiler, libyaml and libnetcdf (C); an MPI C++ compiler for mpi.

OPEN_N   ?= 1
STORM    ?= 1
FIRE     ?= 1
BUILD    ?= build
MPI      ?= 0
NETCDF_PAR ?= 0

CXXFLAGS ?= -O2

ifeq ($(MPI),1)
  CXX     = mpicxx
  MPIFILE = mpi_parallel
else
  ifeq ($(origin CXX),default)
    CXX = g++
  endif
  MPIFILE = mpi_serial
endif

ifneq ($(STORM),0)
  ifneq ($(STORM),1)
    $(error STORM must be 0 or 1)
  endif
endif

ifneq ($(FIRE),0)
  ifneq ($(FIRE),1)
    $(error FIRE must be 0 or 1)
  endif
endif

CPPFLAGS += -DTEM_ENABLE_FIRE=$(FIRE) -DTEM_ENABLE_STORM=$(STORM) -Isrc/physics -Isrc/io -Isrc/driver $(shell pkg-config --cflags yaml-0.1 netcdf 2>/dev/null)
ifeq ($(OPEN_N),0)
  CPPFLAGS += -DTEM_NO_OPENN
endif
ifeq ($(NETCDF_PAR),1)
  CPPFLAGS += -DTEM_NETCDF_PAR=1
endif
CXXFLAGS += -std=c++17 -Wall -Wno-unused-variable -Wno-unused-but-set-variable -MMD -MP
LIBS     := $(shell pkg-config --libs yaml-0.1 netcdf 2>/dev/null || echo -lyaml -lnetcdf)

vpath %.cpp src/physics src/io src/driver

# the driver and its Telm45 class are not part of the common objects
DRIVER   = xtem_gds telm_gds
ALL      = $(basename $(notdir $(wildcard src/*/*.cpp)))
COMMON   = $(filter-out $(DRIVER) mpi_serial mpi_parallel,$(ALL))
OBJ      = $(addprefix $(BUILD)/,$(addsuffix .o,$(COMMON) $(MPIFILE)))

.PHONY: all mpi test clean
all: $(BUILD)/xtem

$(BUILD)/xtem: $(BUILD)/xtem_gds.o $(BUILD)/telm_gds.o $(OBJ)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LIBS)

# The stamp holds the compile-time options of a build directory. It is rewritten only when they
# change, and every object depends on it, so a changed option rebuilds the objects.
STAMP = NETCDF_PAR=$(NETCDF_PAR) OPEN_N=$(OPEN_N) STORM=$(STORM) FIRE=$(FIRE) MPI=$(MPI) CXX=$(CXX) CXXFLAGS=$(CXXFLAGS)

.PHONY: FORCE
FORCE:

$(BUILD)/.options: FORCE
	@mkdir -p $(BUILD)
	@if [ ! -f $@ ] || [ "$$(cat $@)" != "$(STAMP)" ]; then printf '%s\n' "$(STAMP)" > $@; fi

$(BUILD)/%.o: %.cpp $(BUILD)/.options
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/timestep_test: tests/timestep_test.cpp $(BUILD)/tem_time.o $(BUILD)/.options
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< $(BUILD)/tem_time.o -o $@

$(BUILD)/yaml_test: tests/yaml_test.cpp $(BUILD)/tem_yaml.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@ $(LIBS)

$(BUILD)/diag_test: tests/diag_test.cpp $(BUILD)/diag_manager.o $(BUILD)/$(MPIFILE).o $(BUILD)/.options
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/diag_test.cpp $(BUILD)/diag_manager.o $(BUILD)/$(MPIFILE).o -o $@

mpi:
	$(MAKE) BUILD=build_mpi MPI=1 all

test: $(BUILD)/timestep_test $(BUILD)/yaml_test $(BUILD)/diag_test
	$(BUILD)/timestep_test
	$(BUILD)/yaml_test
	$(BUILD)/diag_test

clean:
	rm -rf build build_mpi

-include $(wildcard $(BUILD)/*.d)
