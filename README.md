# 🗂️ Multi-Language Simple List Web App

This repository demonstrates a simple CRUD-based list manager implemented in various programming languages, including **Node.js, Python, C, C++, Java, Rust**, and **C#**, with **MongoDB** or **PostgreSQL** as the backend database.

Each backend exposes a RESTful API and shares consistent functionality and data structure. All of that backend can be connected to the same minimal frontend implementation.

---

## 📁 Project Structure

The project is organized by language and database type:

```
Backend/
  ├── C/
  ├── C++/
  ├── Go/
  ├── Java/
  ├── NodeJS/
  ├── PHP/
  ├── Python/
  ├── Rust/

Frontend/
  ├── React/
  ├── VanillaJS/
```

---

## 🛠️ Database Configuration

### 🟢 MongoDB

- **URI**: `mongodb://localhost:27017`
- **Database**: `listdb`
- **Collection**: `lists`

**Sample MongoDB Response**:
```json
[
  {
    "_id": "67d0f0ce9621cb593d51e944",
    "list": "Some list"
  }
]
```

---

### 🟣 PostgreSQL

```sql
-- Connection URL
postgresql://listuser:listpassword@localhost:5432/listdb

-- Credentials
Username: listuser
Password: listpassword

-- Database: listdb
-- Table: lists
CREATE TABLE lists (
  id SERIAL PRIMARY KEY,
  list VARCHAR(200) NOT NULL
);

-- Sample Response (JSON)
[
  {
    "id": 7,
    "list": "Some list"
  }
]
```

---

## 📡 Sample API Usage

```bash
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
```
