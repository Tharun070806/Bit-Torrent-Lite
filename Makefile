CXX      = g++-15
CXXFLAGS = -std=c++23 -mmacosx-version-min=26.0 -pthread \
           -I/opt/homebrew/opt/openssl/include \
           -L/opt/homebrew/opt/openssl/lib \
           -lssl -lcrypto

all:
	$(CXX) $(CXXFLAGS) src/main.cpp src/parser/decoder.cpp src/utils/hasher.cpp src/utils/helper.cpp src/requestor/requestor.cpp src/utils/socket_utils.cpp src/listener/listener.cpp src/peer/peer.cpp src/peer/peer_messaging.cpp src/peer/peer_handshake.cpp src/lpd/lpd.cpp src/leecher/leecher.cpp src/seeder/seeder.cpp -o a.out