const express = require('express');
const { MongoClient, ObjectId } = require('mongodb');

const app = express();
const port = 3000; // You can choose any port
const mongoUrl = 'mongodb://localhost:27017'; // Default MongoDB URL, change if needed
const dbName = 'listdb'; // Database name
const collectionName = 'lists'; // Collection name

app.use(express.json()); // Middleware to parse JSON request bodies

let db; // Database connection instance
let listsCollection; // Collection instance

async function connectToMongo() {
    try {
        const client = new MongoClient(mongoUrl);
        await client.connect();
        db = client.db(dbName);
        listsCollection = db.collection(collectionName);
        console.log('Connected to MongoDB');
    } catch (error) {
        console.error('MongoDB connection error:', error);
        process.exit(1); // Exit process if database connection fails
    }
}

// Error handling middleware
app.use((err, req, res, next) => {
    console.error('ERROR HANDLER:', err); // Log the error for debugging
    if (err instanceof SyntaxError && err.status === 400 && 'body' in err) {
        return res.status(400).send({ error: 'Invalid JSON format in request body' });
    }
    res.status(500).send({ error: 'Something went wrong!', details: err.message });
});

// --- CRUD Operations for Lists ---

// 1. Create List
app.post('/lists', async (req, res, next) => {
    try {
        const { list } = req.body;
        if (!list) {
            return res.status(400).send({ error: 'List field is required in the request body' });
        }
        if (typeof list !== 'string') { // Basic validation to ensure 'list' is a string
            return res.status(400).send({ error: 'List field must be a string' });
        }

        const result = await listsCollection.insertOne({ list });
        res.status(201).send({ message: 'List created successfully', insertedId: result.insertedId });

    } catch (error) {
        next(error); // Pass error to error handling middleware
    }
});

// 2. Get All Lists
app.get('/lists', async (req, res, next) => {
    try {
        const lists = await listsCollection.find().toArray();
        res.status(200).send(lists);
    } catch (error) {
        next(error);
    }
});

// 3. Get List by ID
app.get('/lists/:id', async (req, res, next) => {
    try {
        const id = req.params.id;
        if (!ObjectId.isValid(id)) {
            return res.status(400).send({ error: 'Invalid list ID format' });
        }
        const list = await listsCollection.findOne({ _id: new ObjectId(id) });
        if (!list) {
            return res.status(404).send({ error: 'List not found' });
        }
        res.status(200).send(list);
    } catch (error) {
        next(error);
    }
});

// 4. Update List by ID
app.put('/lists/:id', async (req, res, next) => {
    try {
        const id = req.params.id;
        const { list } = req.body;

        if (!ObjectId.isValid(id)) {
            return res.status(400).send({ error: 'Invalid list ID format' });
        }
        if (!list) {
            return res.status(400).send({ error: 'List field is required in the request body for update' });
        }
        if (typeof list !== 'string') { // Basic validation to ensure 'list' is a string
            return res.status(400).send({ error: 'List field must be a string for update' });
        }

        const result = await listsCollection.updateOne(
            { _id: new ObjectId(id) },
            { $set: { list } }
        );

        if (result.matchedCount === 0) {
            return res.status(404).send({ error: 'List not found for update' });
        }
        res.status(200).send({ message: 'List updated successfully', modifiedCount: result.modifiedCount });

    } catch (error) {
        next(error);
    }
});

// 5. Delete List by ID
app.delete('/lists/:id', async (req, res, next) => {
    try {
        const id = req.params.id;
        if (!ObjectId.isValid(id)) {
            return res.status(400).send({ error: 'Invalid list ID format' });
        }

        const result = await listsCollection.deleteOne({ _id: new ObjectId(id) });

        if (result.deletedCount === 0) {
            return res.status(404).send({ error: 'List not found for deletion' });
        }
        res.status(200).send({ message: 'List deleted successfully', deletedCount: result.deletedCount });

    } catch (error) {
        next(error);
    }
});


// Start the server and connect to MongoDB
async function startServer() {
    await connectToMongo();
    app.listen(port, () => {
        console.log(`Server listening on port http://localhost:${port}`);
    });
}

startServer();