
-include config.mk

CXXFLAGS += -MMD -MP -Ilib
LIBS += -lsdk_m2m -lsdk_aos -laosgen -lm2mgen -lcoap -lcommon -lappfw

EXE = aos_metering_app

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

all: $(EXE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

-include $(DEPS)

clean:
	rm -f $(EXE) $(OBJS) $(DEPS) *.aos

.PHONY: all clean

