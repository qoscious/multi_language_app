# MongoDB:
uri: mongodb://localhost:27017
database name: listdb
collection: lists

REST response format:
[
  {
    "_id": "67d0f0ce9621cb593d51e944",
    "list": "Some list"
  }
]


# PostgreSQL:
url: postgresql://listuser:listpassword@localhost:5432/listdb
database: listdb
table: lists
username: listuser
password: listpassword
lists:
  id SERIAL PRIMARY KEY,
  list VARCHAR(200) NOT NULL

REST response format:
[
  {
    "id": 7,
    "list": "Some list"
  }
]


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





curl -X POST http://localhost:3000/lists \
  -H "Content-Type: application/json" \
  -d '{"list": "Test list item"}'

curl -X GET http://localhost:3000/lists

curl -X GET http://localhost:3000/lists/<id>

curl -X PUT http://localhost:3000/lists/<id> \
  -H "Content-Type: application/json" \
  -d '{"list": "Updated list item"}'

curl -X DELETE http://localhost:3000/lists/<id>




Create Backend REST API to CRUD "list" database with rust actix and postgresql, CORS, the specifications is as follows:

url: postgresql://listuser:listpassword@localhost:5432/listdb
database: listdb
table: lists
username: listuser
password: listpassword
lists:
  id SERIAL PRIMARY KEY,
  list VARCHAR(200) NOT NULL

REST response format:
[
  {
    "id": 7,
    "list": "Some list"
  }
]

API endpoint methods and url formats:

curl -X POST http://localhost:3000/lists \
  -H "Content-Type: application/json" \
  -d '{"list": "Test list item"}'

curl -X GET http://localhost:3000/lists

curl -X GET http://localhost:3000/lists/<id>

curl -X PUT http://localhost:3000/lists/<id> \
  -H "Content-Type: application/json" \
  -d '{"list": "Updated list item"}'

curl -X DELETE http://localhost:3000/lists/<id>


Build frontend to crud this api http://localhost:3000/lists, the message format is [{ "_id": "67d0f0ce9621cb593d51e944", "list": "Some list"}].
There are 3 page:
1. home page which will show all list in table (no number column), every list row have edit and delete button.
2. Edit page
3. Add page

Create the router with history api.
Every page build with web component implementing custom element, shadow dom, and html template.
Demonstrate how vanilla js frontend can consist of web component and web sub component, create table sub component then attach to home, create form sub component then attach to edit and add page.



