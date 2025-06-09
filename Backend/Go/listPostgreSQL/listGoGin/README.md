postgres=# CREATE DATABASE listdb;
CREATE DATABASE
postgres=# CREATE USER listuser WITH PASSWORD 'listpassword';
CREATE ROLE
postgres=# GRANT CONNECT ON DATABASE listdb TO listuser;
GRANT
postgres=# \c listdb
You are now connected to database "listdb" as user "postgres".
listdb=# GRANT USAGE, CREATE ON SCHEMA public TO listuser;
GRANT
listdb=# ALTER ROLE listuser SET search_path = public;
ALTER ROLE
listdb=# CREATE TABLE lists (
  id SERIAL PRIMARY KEY,
  list VARCHAR(200) NOT NULL
);
CREATE TABLE
listdb=# GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE lists TO listuser;
GRANT
listdb=# GRANT ALL PRIVILEGES ON SEQUENCE lists_id_seq TO listuser;
GRANT
listdb=# \q



go mod init ListGoGinPostgreSQL
go get github.com/gin-gonic/gin
go get gorm.io/gorm
go get gorm.io/driver/postgres
go mod tidy

go run main.go



curl -X POST http://localhost:3000/lists \
  -H "Content-Type: application/json" \
  -d '{"list": "Test list item"}'

curl http://localhost:3000/lists

curl http://localhost:3000/lists/<id>

curl -X PUT http://localhost:3000/lists/<id> \
  -H "Content-Type: application/json" \
  -d '{"list": "Updated list item"}'

curl -X DELETE http://localhost:3000/lists/<id>
