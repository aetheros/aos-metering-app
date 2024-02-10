
-include config.mk

CXXFLAGS += -MMD -MP
LIBS += $(AOSSDKLIBS)

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

