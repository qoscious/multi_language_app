package main

import (
	"log"
	"net/http"
	"strconv"
	"strings"

	"github.com/gofiber/fiber/v2"
	"github.com/gofiber/fiber/v2/middleware/cors"
	"gorm.io/driver/postgres"
	"gorm.io/gorm"
)

var DB *gorm.DB

// ListItem represents a record in the "lists" table.
type ListItem struct {
	ID   uint   `json:"id" gorm:"primaryKey;autoIncrement"`
	List string `json:"list" gorm:"size:200;not null"`
}

// TableName sets the table name for ListItem.
func (ListItem) TableName() string {
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
// It validates the input and creates a new record.
func CreateListItem(c *fiber.Ctx) error {
	var item ListItem
	if err := c.BodyParser(&item); err != nil {
		return c.Status(http.StatusBadRequest).JSON(fiber.Map{"error": err.Error()})
	}
	if msg, ok := ValidateListField(item.List); !ok {
		return c.Status(http.StatusBadRequest).JSON(fiber.Map{"error": msg})
	}
	if err := DB.Create(&item).Error; err != nil {
		return c.Status(http.StatusInternalServerError).JSON(fiber.Map{"error": err.Error()})
	}
	return c.Status(http.StatusCreated).JSON(fiber.Map{"message": "List created successfully", "data": item})
}

// GetListItems handles GET /lists.
func GetListItems(c *fiber.Ctx) error {
	var items []ListItem
	if err := DB.Find(&items).Error; err != nil {
		return c.Status(http.StatusInternalServerError).JSON(fiber.Map{"error": err.Error()})
	}
	return c.JSON(items)
}

// GetListItem handles GET /lists/:id.
func GetListItem(c *fiber.Ctx) error {
	idParam := c.Params("id")
	id, err := strconv.ParseUint(idParam, 10, 32)
	if err != nil {
		return c.Status(http.StatusBadRequest).JSON(fiber.Map{"error": "Invalid ID"})
	}
	var item ListItem
	if err := DB.First(&item, uint(id)).Error; err != nil {
		return c.Status(http.StatusNotFound).JSON(fiber.Map{"error": "List not found"})
	}
	return c.JSON(item)
}

// UpdateListItem handles PUT /lists/:id.
func UpdateListItem(c *fiber.Ctx) error {
	idParam := c.Params("id")
	id, err := strconv.ParseUint(idParam, 10, 32)
	if err != nil {
		return c.Status(http.StatusBadRequest).JSON(fiber.Map{"error": "Invalid ID"})
	}
	var item ListItem
	if err := DB.First(&item, uint(id)).Error; err != nil {
		return c.Status(http.StatusNotFound).JSON(fiber.Map{"error": "List not found"})
	}

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
	if err := DB.Save(&item).Error; err != nil {
		return c.Status(http.StatusInternalServerError).JSON(fiber.Map{"error": err.Error()})
	}
	return c.JSON(fiber.Map{"message": "List updated successfully", "data": item})
}

// DeleteListItem handles DELETE /lists/:id.
func DeleteListItem(c *fiber.Ctx) error {
	idParam := c.Params("id")
	id, err := strconv.ParseUint(idParam, 10, 32)
	if err != nil {
		return c.Status(http.StatusBadRequest).JSON(fiber.Map{"error": "Invalid ID"})
	}
	var item ListItem
	if err := DB.First(&item, uint(id)).Error; err != nil {
		return c.Status(http.StatusNotFound).JSON(fiber.Map{"error": "List not found"})
	}
	if err := DB.Delete(&item).Error; err != nil {
		return c.Status(http.StatusInternalServerError).JSON(fiber.Map{"error": err.Error()})
	}
	return c.JSON(fiber.Map{"message": "List deleted successfully"})
}

func main() {
	// PostgreSQL connection string. Adjust the credentials as needed.
	dsn := "host=localhost user=listuser password=listpassword dbname=listdb port=5432 sslmode=disable TimeZone=UTC"

	var err error
	DB, err = gorm.Open(postgres.Open(dsn), &gorm.Config{})
	if err != nil {
		log.Fatal("Failed to connect to database: ", err)
	}
	// Auto-migrate to create/update the "lists" table.
	if err := DB.AutoMigrate(&ListItem{}); err != nil {
		log.Fatal("Failed to migrate database: ", err)
	}

	// Create a new Fiber app.
	app := fiber.New()

	// Use Fiber's CORS middleware.
	app.Use(cors.New(cors.Config{
		AllowOrigins: "*",
		AllowMethods: "GET,POST,PUT,DELETE,OPTIONS",
		AllowHeaders: "Content-Type, Authorization",
	}))

	// Define CRUD routes.
	app.Post("/lists", CreateListItem)
	app.Get("/lists", GetListItems)
	app.Get("/lists/:id", GetListItem)
	app.Put("/lists/:id", UpdateListItem)
	app.Delete("/lists/:id", DeleteListItem)

	// Start the server on port 3000.
	log.Fatal(app.Listen(":3000"))
}
