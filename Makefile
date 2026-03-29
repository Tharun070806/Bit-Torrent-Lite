CXX      = g++-15
CXXFLAGS = -std=c++23 -mmacosx-version-min=26.0 -pthread \
           -I/opt/homebrew/opt/openssl/include \
           -L/opt/homebrew/opt/openssl/lib \
           -lssl -lcrypto

all:
	$(CXX) $(CXXFLAGS) src/main.cpp src/Parser/decoder.cpp src/utils/hasher.cpp src/Requestor/requestor.cpp src/utils/socket_utils.cpp -o a.out