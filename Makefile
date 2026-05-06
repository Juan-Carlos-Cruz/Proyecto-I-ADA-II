CXX ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -pedantic -pthread -Ibackend/include

BACKEND_BINARY := backend/bin/riego_backend
BACKEND_SOURCES := $(sort $(wildcard backend/src/*.cpp))
BACKEND_HEADERS := $(sort $(wildcard backend/include/*.hpp))

.PHONY: backend clean-backend

backend: $(BACKEND_BINARY)

$(BACKEND_BINARY): $(BACKEND_SOURCES) $(BACKEND_HEADERS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(BACKEND_SOURCES) -o $@

clean-backend:
	rm -rf backend/bin
