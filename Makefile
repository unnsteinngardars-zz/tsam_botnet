CC =gcc
CXX =g++
CXXFLAGS =-g -Wall --std=c++11

SERVER = $(wildcard ./server/*.cpp)
UTILITIES = $(wildcard ./utilities/*.cpp)
CLIENT = $(wildcard ./client/*.cpp) -lncurses

.PHONY: server client

server: $(SERVER) $(UTILITIES) 
	$(CXX) $(CXXFLAGS) -o tsamgroup33 $(SERVER) $(UTILITIES)

client: $(CLIENT) 
	$(CXX) $(CXXFLAGS) -o chat_client $(CLIENT) $(UTILITIES)
