
sudo apt-get install libjansson-dev libmongoc-dev libbson-dev libmicrohttpd-dev

gcc main.c -o list_api $(pkg-config --cflags libmongoc-1.0 libbson-1.0) -lmicrohttpd -ljansson $(pkg-config --libs libmongoc-1.0 libbson-1.0)

./list_api




curl -X GET http://localhost:3000/lists

curl -X GET http://localhost:3000/lists/<id>

curl -X POST http://localhost:3000/lists -H "Content-Type: application/json" -d '{"list": "some item"}'

curl -X PUT http://localhost:3000/lists/<id> -H "Content-Type: application/json" -d '{"list": "updated item"}'

curl -X DELETE http://localhost:3000/lists/<id>

