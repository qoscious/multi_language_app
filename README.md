# 🗂️ Multi-Language Simple List Web App 

This repository contains simple backend (with frontend) applications written in various languages (Node.js, Python, C, C++, Java, Rust, C#) that implement a basic list manager using REST APIs and persistent storage.

---

## 📦 Project Structure

Each subdirectory contains the same backend logic implemented in a different programming language, connected to either MongoDB or PostgreSQL.

---

## 📚 Database Configuration

### 🟢 MongoDB

- **URI**: `mongodb://localhost:27017`
- **Database**: `listdb`
- **Collection**: `lists`

**Sample Response**:
```json
[
  {
    "_id": "67d0f0ce9621cb593d51e944",
    "list": "Some list"
  }
]

### 🟢 PostgreSQL

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



# Create a new list item
curl -X POST http://localhost:3000/lists \
  -H "Content-Type: application/json" \
  -d '{"list": "Test list item"}'

# Get all list items
curl -X GET http://localhost:3000/lists

# Get a specific list item
curl -X GET http://localhost:3000/lists/<id>

# Update a list item
curl -X PUT http://localhost:3000/lists/<id> \
  -H "Content-Type: application/json" \
  -d '{"list": "Updated list item"}'

# Delete a list item
curl -X DELETE http://localhost:3000/lists/<id>

