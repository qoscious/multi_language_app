source venv/bin/activate
pip install -r requirements.txt

to run:
uvicorn main:app --reload --port 3000


curl -X POST http://localhost:3000/lists \
  -H "Content-Type: application/json" \
  -d '{"list": "Test list item"}'

curl http://localhost:3000/lists

curl http://localhost:3000/lists/<id>

curl -X PUT http://localhost:3000/lists/<id> \
  -H "Content-Type: application/json" \
  -d '{"list": "Updated list item"}'

curl -X DELETE http://localhost:3000/lists/<id>