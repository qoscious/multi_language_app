from fastapi import FastAPI, HTTPException, status
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field
import sqlalchemy
from sqlalchemy import Table, Column, Integer, String, MetaData
import databases

# Database URL (adjust username, password, host, port, and database name as needed)
DATABASE_URL = "postgresql://listuser:listpassword@localhost:5432/listdb"

# Initialize the databases connection (async)
database = databases.Database(DATABASE_URL)

# SQLAlchemy metadata and table definition
metadata = MetaData()

lists = Table(
    "lists",
    metadata,
    Column("id", Integer, primary_key=True),
    Column("list", String(200), nullable=False),
)

# Create the database engine and table (synchronously)
engine = sqlalchemy.create_engine(DATABASE_URL)
metadata.create_all(engine)

# Pydantic models for input and output
class ListItemIn(BaseModel):
    list: str = Field(..., min_length=1, max_length=200)

class ListItemOut(BaseModel):
    id: int
    list: str

# Initialize the FastAPI app
app = FastAPI()

# Add CORS middleware (allow all origins, methods, headers)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["GET", "POST", "PUT", "DELETE", "OPTIONS"],
    allow_headers=["Content-Type", "Authorization"],
)

# Connect to the database on startup and disconnect on shutdown
@app.on_event("startup")
async def startup():
    await database.connect()

@app.on_event("shutdown")
async def shutdown():
    await database.disconnect()

# CRUD Endpoints

# 1. Create a new list item
@app.post("/lists", response_model=ListItemOut, status_code=status.HTTP_201_CREATED)
async def create_list_item(item: ListItemIn):
    trimmed_list = item.list.strip()
    query = lists.insert().values(list=trimmed_list)
    last_record_id = await database.execute(query)
    query = lists.select().where(lists.c.id == last_record_id)
    new_item = await database.fetch_one(query)
    return new_item

# 2. Get all list items
@app.get("/lists", response_model=list[ListItemOut])
async def get_list_items():
    query = lists.select()
    return await database.fetch_all(query)

# 3. Get a single list item by ID
@app.get("/lists/{id}", response_model=ListItemOut)
async def get_list_item(id: int):
    query = lists.select().where(lists.c.id == id)
    item = await database.fetch_one(query)
    if item is None:
        raise HTTPException(status_code=404, detail="List item not found")
    return item

# 4. Update a list item by ID
@app.put("/lists/{id}", response_model=ListItemOut)
async def update_list_item(id: int, update: ListItemIn):
    query = lists.select().where(lists.c.id == id)
    item = await database.fetch_one(query)
    if item is None:
        raise HTTPException(status_code=404, detail="List item not found")
    trimmed_list = update.list.strip()
    query = lists.update().where(lists.c.id == id).values(list=trimmed_list)
    await database.execute(query)
    query = lists.select().where(lists.c.id == id)
    updated_item = await database.fetch_one(query)
    return updated_item

# 5. Delete a list item by ID
@app.delete("/lists/{id}")
async def delete_list_item(id: int):
    query = lists.select().where(lists.c.id == id)
    item = await database.fetch_one(query)
    if item is None:
        raise HTTPException(status_code=404, detail="List item not found")
    query = lists.delete().where(lists.c.id == id)
    await database.execute(query)
    return {"message": "List deleted successfully"}
