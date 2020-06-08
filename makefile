# To compile cpp

# $ g++ -I/usr/local/opt/openssl@1.1/include -L/usr/local/opt/openssl@1.1/lib Process.cpp Blockchain.cpp -o Process -lcrypto

# $ c++ -std=c++11 -I/usr/local/opt/openssl@1.1/include -L/usr/local/opt/openssl@1.1/lib Process.cpp Blockchain.cpp Msg.pb.cc -o Process -lcrypto `pkg-config --cflags --libs protobuf`
