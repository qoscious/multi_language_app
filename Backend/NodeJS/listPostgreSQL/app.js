const express = require('express');
const { Sequelize, DataTypes } = require('sequelize');

const app = express();
const port = 3000;

// --- Sequelize Setup ---
// Adjust the DSN parameters (database name, user, password, host) as needed.
const sequelize = new Sequelize('listdb', 'listuser', 'listpassword', {
  host: 'localhost',
  dialect: 'postgres',
  logging: false,
});

// --- Define the List Model ---
// This model corresponds to the "lists" table.
// The "list" field is required and is limited to 200 characters.
const List = sequelize.define('List', {
  // Sequelize will auto-add an "id" primary key field if not defined explicitly.
  list: {
    type: DataTypes.STRING(200),
    allowNull: false,
    validate: {
      notEmpty: {
        msg: 'List field is required',
      },
      len: {
        args: [1, 200],
        msg: 'List field must be at most 200 characters',
      },
    },
  },
}, {
  tableName: 'lists',
  timestamps: false,
});

// --- Middleware ---
// Parse JSON request bodies.
app.use(express.json());

// Set CORS headers.
app.use((req, res, next) => {
  res.setHeader("Access-Control-Allow-Origin", "*");
  res.setHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  res.setHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  if (req.method === "OPTIONS") {
    return res.sendStatus(200);
  }
  next();
});

// --- Error Handling Middleware ---
// This middleware catches errors and sends a JSON response.
app.use((err, req, res, next) => {
  console.error('ERROR HANDLER:', err);
  if (err instanceof SyntaxError && err.status === 400 && 'body' in err) {
    return res.status(400).json({ error: 'Invalid JSON format in request body' });
  }
  // Handle Sequelize validation errors.
  if (err.name === 'SequelizeValidationError') {
    const errors = {};
    err.errors.forEach((error) => {
      errors[error.path] = error.message;
    });
    return res.status(400).json({ error: 'Validation failed', details: errors });
  }
  res.status(500).json({ error: 'Something went wrong!', details: err.message });
});

// --- CRUD Endpoints ---

// 1. Create a new list item
app.post('/lists', async (req, res, next) => {
  try {
    const { list } = req.body;
    if (!list) {
      return res.status(400).json({ error: 'List field is required in the request body' });
    }
    const newList = await List.create({ list });
    res.status(201).json({ message: 'List created successfully', data: newList });
  } catch (error) {
    next(error);
  }
});

// 2. Get all list items
app.get('/lists', async (req, res, next) => {
  try {
    const lists = await List.findAll();
    res.status(200).json(lists);
  } catch (error) {
    next(error);
  }
});

// 3. Get a single list item by ID
app.get('/lists/:id', async (req, res, next) => {
  try {
    const id = req.params.id;
    if (isNaN(id)) {
      return res.status(400).json({ error: 'Invalid list ID format' });
    }
    const listItem = await List.findByPk(id);
    if (!listItem) {
      return res.status(404).json({ error: 'List not found' });
    }
    res.status(200).json(listItem);
  } catch (error) {
    next(error);
  }
});

// 4. Update a list item by ID
app.put('/lists/:id', async (req, res, next) => {
  try {
    const id = req.params.id;
    if (isNaN(id)) {
      return res.status(400).json({ error: 'Invalid list ID format' });
    }
    const { list } = req.body;
    if (!list) {
      return res.status(400).json({ error: 'List field is required in the request body for update' });
    }
    const listItem = await List.findByPk(id);
    if (!listItem) {
      return res.status(404).json({ error: 'List not found for update' });
    }
    listItem.list = list;
    await listItem.save();
    res.status(200).json({ message: 'List updated successfully', data: listItem });
  } catch (error) {
    next(error);
  }
});

// 5. Delete a list item by ID
app.delete('/lists/:id', async (req, res, next) => {
  try {
    const id = req.params.id;
    if (isNaN(id)) {
      return res.status(400).json({ error: 'Invalid list ID format' });
    }
    const listItem = await List.findByPk(id);
    if (!listItem) {
      return res.status(404).json({ error: 'List not found for deletion' });
    }
    await listItem.destroy();
    res.status(200).json({ message: 'List deleted successfully' });
  } catch (error) {
    next(error);
  }
});

// --- Start the Server ---
// Authenticate with the database, sync the models (creating tables if needed),
// and then start listening for incoming requests.
sequelize.authenticate()
  .then(() => {
    console.log('Connected to PostgreSQL with Sequelize');
    return sequelize.sync(); // Auto-create tables if they don't exist.
  })
  .then(() => {
    app.listen(port, () => {
      console.log(`Server listening on port http://localhost:${port}`);
    });
  })
  .catch((error) => {
    console.error('Unable to connect to the database:', error);
    process.exit(1);
  });
