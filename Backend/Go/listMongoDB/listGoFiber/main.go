package main

import (
	"log"
	"net/http"
	"strings"

	"github.com/gofiber/fiber/v2"
	"github.com/gofiber/fiber/v2/middleware/cors"
	"github.com/kamva/mgm/v3"
	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo/options"
)

// ListItem represents a document with a single "list" field.
// mgm.DefaultModel provides the default MongoDB _id field.
type ListItem struct {
	mgm.DefaultModel `bson:",inline"`
	List             string `json:"list" bson:"list"`
}

// CollectionName returns the collection name for this model.
func (l *ListItem) CollectionName() string {
	return "lists"
}

// ValidateListField checks that the list field is non-empty and within 200 characters.
func ValidateListField(list string) (string, bool) {
	trimmed := strings.TrimSpace(list)
	if trimmed == "" {
		return "List field is required", false
	}
	if len(trimmed) > 200 {
		return "List field exceeds maximum length of 200", false
	}
	return "", true
}

// CreateListItem handles POST /lists.
// It validates the input and creates a new document.
func CreateListItem(c *fiber.Ctx) error {
	var item ListItem
	if err := c.BodyParser(&item); err != nil {
		return c.Status(http.StatusBadRequest).JSON(fiber.Map{"error": err.Error()})
	}
	if msg, ok := ValidateListField(item.List); !ok {
		return c.Status(http.StatusBadRequest).JSON(fiber.Map{"error": msg})
	}
	if err := mgm.Coll(&item).Create(&item); err != nil {
		return c.Status(http.StatusInternalServerError).JSON(fiber.Map{"error": err.Error()})
	}
	return c.Status(http.StatusCreated).JSON(fiber.Map{"message": "List created successfully", "data": item})
}

// GetListItems handles GET /lists and returns all list documents.
func GetListItems(c *fiber.Ctx) error {
	var items []ListItem
	if err := mgm.Coll(&ListItem{}).SimpleFind(&items, bson.M{}); err != nil {
		return c.Status(http.StatusInternalServerError).JSON(fiber.Map{"error": err.Error()})
	}

	// Convert each item to a map with _id as a hex string.
	var responseItems []fiber.Map
	for _, item := range items {
		responseItems = append(responseItems, fiber.Map{
			"_id":  item.ID.Hex(),
			"list": item.List,
		})
	}
	return c.JSON(responseItems)
}

// GetListItem handles GET /lists/:id and returns a single document by ID.
func GetListItem(c *fiber.Ctx) error {
	id := c.Params("id")
	var item ListItem
	if err := mgm.Coll(&ListItem{}).FindByID(id, &item); err != nil {
		return c.Status(http.StatusNotFound).JSON(fiber.Map{"error": "List not found"})
	}
	return c.JSON(fiber.Map{
		"_id":  item.ID.Hex(),
		"list": item.List,
	})
}

// UpdateListItem handles PUT /lists/:id.
// It validates input and updates the list field.
func UpdateListItem(c *fiber.Ctx) error {
	id := c.Params("id")
	var item ListItem
	if err := mgm.Coll(&ListItem{}).FindByID(id, &item); err != nil {
		return c.Status(http.StatusNotFound).JSON(fiber.Map{"error": "List not found"})
	}

	// Bind only the update data.
	var updateData struct {
		List string `json:"list"`
	}
	if err := c.BodyParser(&updateData); err != nil {
		return c.Status(http.StatusBadRequest).JSON(fiber.Map{"error": err.Error()})
	}
	if msg, ok := ValidateListField(updateData.List); !ok {
		return c.Status(http.StatusBadRequest).JSON(fiber.Map{"error": msg})
	}

	item.List = updateData.List
	if err := mgm.Coll(&item).Update(&item); err != nil {
		return c.Status(http.StatusInternalServerError).JSON(fiber.Map{"error": err.Error()})
	}
	return c.JSON(fiber.Map{"message": "List updated successfully", "data": item})
}

// DeleteListItem handles DELETE /lists/:id and removes a document.
func DeleteListItem(c *fiber.Ctx) error {
	id := c.Params("id")
	var item ListItem
	if err := mgm.Coll(&ListItem{}).FindByID(id, &item); err != nil {
		return c.Status(http.StatusNotFound).JSON(fiber.Map{"error": "List not found"})
	}
	if err := mgm.Coll(&item).Delete(&item); err != nil {
		return c.Status(http.StatusInternalServerError).JSON(fiber.Map{"error": err.Error()})
	}
	return c.JSON(fiber.Map{"message": "List deleted successfully"})
}

func main() {
	// Initialize mgm with the MongoDB connection.
	// Database name: listdb; MongoDB URI: mongodb://localhost:27017
	if err := mgm.SetDefaultConfig(nil, "listdb", options.Client().ApplyURI("mongodb://localhost:27017")); err != nil {
		log.Fatal("Error connecting to MongoDB: ", err)
	}

	// Create a new Fiber app.
	app := fiber.New()

	// Use Fiber's CORS middleware to set CORS headers.
	app.Use(cors.New(cors.Config{
		AllowOrigins: "*",
		AllowMethods: "GET,POST,PUT,DELETE,OPTIONS",
		AllowHeaders: "Content-Type, Authorization",
	}))

	// Define CRUD routes for /lists.
	app.Post("/lists", CreateListItem)
	app.Get("/lists", GetListItems)
	app.Get("/lists/:id", GetListItem)
	app.Put("/lists/:id", UpdateListItem)
	app.Delete("/lists/:id", DeleteListItem)

	// Start the server on port 3000.
	log.Fatal(app.Listen(":3000"))
}
