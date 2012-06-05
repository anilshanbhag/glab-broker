# ------------------------------------------------
# Environment variable WISELIB_PATH_TESTING needed
# ------------------------------------------------

#all: pc
#all: isense
all: isense.5148
#all: shawn
# all: scw_msb
# all: contiki_msb
# all: contiki_micaz
# all: contiki_sky
# all: isense
# all: tinyos-tossim
# all: tinyos-micaz
# all: tinyos-telosb

#export APP_SRC=tuple_store_feature_test.cpp
#export BIN_OUT=tuple_store_feature_test

#export PC_CXX_FLAGS=-m32
export PC_CXX_FLAGS=-m32 -Wno-format -Wno-unused-variable
export PC_COMPILE_DEBUG=1

#export APP_SRC=iot_test.cpp
#export BIN_OUT=iot_test

#export APP_SRC=broker_client.cpp
#export BIN_OUT=broker_client

export APP_SRC=local_test.cpp
export BIN_OUT=local_test

#export APP_SRC=nothing.cc
#export BIN_OUT=nothing

export WISELIB_EXIT_MAIN=0

include ../Makefile.local
include $(WISELIB_GENERIC_APPS)/Makefile
