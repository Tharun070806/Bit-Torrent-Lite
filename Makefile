CXX      = g++-15
CXXFLAGS = -std=c++23 -mmacosx-version-min=26.0 -pthread

SRC = src/tracker.cpp src/peer.cpp 

tracker: $(SRC) src/tracker.cpp
	$(CXX) $(CXXFLAGS) $(SRC) src/tracker.cpp -o tracker

peer: $(SRC) src/peer.cpp
	$(CXX) $(CXXFLAGS) $(SRC) src/peer.cpp -o peer