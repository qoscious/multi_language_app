sudo apt-get update
sudo apt-get install libboost-all-dev

install mongodb C++ driver

Download Crow from https://github.com/CrowCpp/Crow, copy include folder to this project root dir. (already done for this project)


g++ -std=c++17 -DCROW_USE_BOOST=1 -I./include -I/usr/local/include main.cpp -lmongocxx -lbsoncxx -lpthread -o list_api
