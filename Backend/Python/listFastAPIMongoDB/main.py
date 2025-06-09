from fastapi import FastAPI, HTTPException, Request, status
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from motor.motor_asyncio import AsyncIOMotorClient
from pydantic import BaseModel, Field
from bson import ObjectId
import os

# Helper to convert ObjectId to string
def obj_id_to_str(document):
    document["_id"] = str(document["_id"])
    return document

# Pydantic model for input validation
class ListItemIn(BaseModel):
    list: str = Field(..., min_length=1, max_length=200)

# Pydantic model for output
class ListItemOut(BaseModel):
    id: str = Field(..., alias="_id")
    list: str

    class Config:
        allow_population_by_field_name = True

# Initialize FastAPI app
app = FastAPI()

# Add CORS middleware (allow all origins, methods, headers)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["GET", "POST", "PUT", "DELETE", "OPTIONS"],
    allow_headers=["Content-Type", "Authorization"],
)

# Connect to MongoDB
MONGO_DETAILS = os.getenv("MONGO_DETAILS", "mongodb://localhost:27017")
client = AsyncIOMotorClient(MONGO_DETAILS)
db = client["listdb"]
collection = db["lists"]


# --- CRUD Endpoints ---

# Create a new list item
@app.post("/lists", response_model=ListItemOut, status_code=201)
async def create_list_item(item: ListItemIn):
    trimmed_list = item.list.strip()
    if not trimmed_list:
        raise HTTPException(status_code=400, detail="List field is required")
    if len(trimmed_list) > 200:
        raise HTTPException(status_code=400, detail="List field exceeds maximum length of 200")
    doc = {"list": trimmed_list}
    result = await collection.insert_one(doc)
    new_item = await collection.find_one({"_id": result.inserted_id})
    return obj_id_to_str(new_item)

# Get all list items
@app.get("/lists", response_model=list[ListItemOut])
async def get_list_items():
    items = []
    cursor = collection.find({})
    async for document in cursor:
        items.append(obj_id_to_str(document))
    return items

# Get a single list item by ID
@app.get("/lists/{id}", response_model=ListItemOut)
async def get_list_item(id: str):
    try:
        obj_id = ObjectId(id)
    except Exception:
        raise HTTPException(status_code=400, detail="Invalid list ID format")
    item = await collection.find_one({"_id": obj_id})
    if item is None:
        raise HTTPException(status_code=404, detail="List not found")
    return obj_id_to_str(item)

# Update a list item by ID
@app.put("/lists/{id}", response_model=ListItemOut)
async def update_list_item(id: str, update: ListItemIn):
    try:
        obj_id = ObjectId(id)
    except Exception:
        raise HTTPException(status_code=400, detail="Invalid list ID format")
    trimmed_list = update.list.strip()
    if not trimmed_list:
        raise HTTPException(status_code=400, detail="List field is required for update")
    if len(trimmed_list) > 200:
        raise HTTPException(status_code=400, detail="List field exceeds maximum length of 200")
    update_result = await collection.update_one({"_id": obj_id}, {"$set": {"list": trimmed_list}})
    if update_result.matched_count == 0:
        raise HTTPException(status_code=404, detail="List not found for update")
    updated_item = await collection.find_one({"_id": obj_id})
    return obj_id_to_str(updated_item)

# Delete a list item by ID
@app.delete("/lists/{id}", response_model=dict)
async def delete_list_item(id: str):
    try:
        obj_id = ObjectId(id)
    except Exception:
        raise HTTPException(status_code=400, detail="Invalid list ID format")
    delete_result = await collection.delete_one({"_id": obj_id})
    if delete_result.deleted_count == 0:
        raise HTTPException(status_code=404, detail="List not found for deletion")
    return {"message": "List deleted successfully"}
