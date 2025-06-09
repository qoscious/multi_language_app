sudo apt-get update
sudo apt-get install libboost-all-dev libpqxx-dev

Download Crow from https://github.com/CrowCpp/Crow, copy include folder to this project root dir. (already done for this project)

g++ -std=c++17 -DCROW_USE_BOOST=1 -I./include -I/usr/local/include main.cpp -lpqxx -lpq -pthread -o list_api