const express = require('express');
const mongoose = require('mongoose');

const app = express();
const port = 3000; // You can choose any port
const mongoUrl = 'mongodb://localhost:27017/listdb'; // MongoDB connection URL with database name

app.use(express.json()); // Middleware to parse JSON request bodies

app.use((req, res, next) => {
    res.setHeader("Access-Control-Allow-Origin", "*");
    res.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    if (req.method === "OPTIONS") {
      return res.sendStatus(200);
    }
    next();
});
  


// --- Mongoose Schema and Model ---
const listSchema = new mongoose.Schema({
    list: {
        type: String,
        required: true, // Make 'list' field mandatory
        trim: true,     // Remove leading/trailing whitespace
        maxlength: 200 // Limit string length (example validation)
    }
});

const List = mongoose.model('List', listSchema); // 'List' model for 'lists' collection


async function connectToMongo() {
    try {
        await mongoose.connect(mongoUrl);
        console.log('Connected to MongoDB with Mongoose');
    } catch (error) {
        console.error('Mongoose connection error:', error);
        process.exit(1); // Exit process if database connection fails
    }
}

// Error handling middleware (same as before)
app.use((err, req, res, next) => {
    console.error('ERROR HANDLER:', err);
    if (err instanceof SyntaxError && err.status === 400 && 'body' in err) {
        return res.status(400).send({ error: 'Invalid JSON format in request body' });
    }
    // Mongoose validation error handling
    if (err.name === 'ValidationError') {
        const errors = {};
        for (let key in err.errors) {
            errors[key] = err.errors[key].message;
        }
        return res.status(400).send({ error: 'Validation failed', details: errors });
    }
    res.status(500).send({ error: 'Something went wrong!', details: err.message });
});

// --- CRUD Operations for Lists (using Mongoose) ---

// 1. Create List
app.post('/lists', async (req, res, next) => {
    try {
        const { list } = req.body;
        if (!list) {
            return res.status(400).send({ error: 'List field is required in the request body' });
        }

        const newList = new List({ list }); // Create a new List document instance
        const savedList = await newList.save(); // Save the document to the database
        res.status(201).send({ message: 'List created successfully', data: savedList }); // Send back the saved list

    } catch (error) {
        next(error);
    }
});

// 2. Get All Lists
app.get('/lists', async (req, res, next) => {
    try {
        const lists = await List.find(); // Mongoose's find() to get all documents
        res.status(200).send(lists);
    } catch (error) {
        next(error);
    }
});

// 3. Get List by ID
app.get('/lists/:id', async (req, res, next) => {
    try {
        const id = req.params.id;

        if (!mongoose.Types.ObjectId.isValid(id)) { // Mongoose way to check for valid ObjectId
            return res.status(400).send({ error: 'Invalid list ID format' });
        }

        const list = await List.findById(id); // Mongoose's findById()

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

        if (!mongoose.Types.ObjectId.isValid(id)) {
            return res.status(400).send({ error: 'Invalid list ID format' });
        }
        if (!list) {
            return res.status(400).send({ error: 'List field is required in the request body for update' });
        }

        const updatedList = await List.findByIdAndUpdate(
            id,
            { list },
            { new: true, runValidators: true } // Options: new:true to get updated doc, runValidators to apply schema validations
        );

        if (!updatedList) {
            return res.status(404).send({ error: 'List not found for update' });
        }
        res.status(200).send({ message: 'List updated successfully', data: updatedList });

    } catch (error) {
        next(error);
    }
});

// 5. Delete List by ID
app.delete('/lists/:id', async (req, res, next) => {
    try {
        const id = req.params.id;

        if (!mongoose.Types.ObjectId.isValid(id)) {
            return res.status(400).send({ error: 'Invalid list ID format' });
        }

        const deletedList = await List.findByIdAndDelete(id);

        if (!deletedList) {
            return res.status(404).send({ error: 'List not found for deletion' });
        }
        res.status(200).send({ message: 'List deleted successfully' });

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