package main

import (
	"log"
	"net/http"
	"strconv"
	"strings"

	"github.com/gin-gonic/gin"
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
func CreateListItem(c *gin.Context) {
	var item ListItem
	if err := c.ShouldBindJSON(&item); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}
	if msg, ok := ValidateListField(item.List); !ok {
		c.JSON(http.StatusBadRequest, gin.H{"error": msg})
		return
	}
	if err := DB.Create(&item).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
		return
	}
	c.JSON(http.StatusCreated, gin.H{"message": "List created successfully", "data": item})
}

// GetListItems handles GET /lists.
func GetListItems(c *gin.Context) {
	var items []ListItem
	if err := DB.Find(&items).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
		return
	}
	c.JSON(http.StatusOK, items)
}

// GetListItem handles GET /lists/:id.
func GetListItem(c *gin.Context) {
	idParam := c.Param("id")
	id, err := strconv.ParseUint(idParam, 10, 32)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "Invalid ID"})
		return
	}
	var item ListItem
	if err := DB.First(&item, uint(id)).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "List not found"})
		return
	}
	c.JSON(http.StatusOK, item)
}

// UpdateListItem handles PUT /lists/:id.
func UpdateListItem(c *gin.Context) {
	idParam := c.Param("id")
	id, err := strconv.ParseUint(idParam, 10, 32)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "Invalid ID"})
		return
	}
	var item ListItem
	if err := DB.First(&item, uint(id)).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "List not found"})
		return
	}

	var updateData struct {
		List string `json:"list"`
	}
	if err := c.ShouldBindJSON(&updateData); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}
	if msg, ok := ValidateListField(updateData.List); !ok {
		c.JSON(http.StatusBadRequest, gin.H{"error": msg})
		return
	}

	item.List = updateData.List
	if err := DB.Save(&item).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
		return
	}
	c.JSON(http.StatusOK, gin.H{"message": "List updated successfully", "data": item})
}

// DeleteListItem handles DELETE /lists/:id.
func DeleteListItem(c *gin.Context) {
	idParam := c.Param("id")
	id, err := strconv.ParseUint(idParam, 10, 32)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "Invalid ID"})
		return
	}
	var item ListItem
	if err := DB.First(&item, uint(id)).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "List not found"})
		return
	}
	if err := DB.Delete(&item).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
		return
	}
	c.JSON(http.StatusOK, gin.H{"message": "List deleted successfully"})
}

func main() {
	// PostgreSQL connection string. Adjust credentials as needed.
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

	// Initialize Gin router.
	router := gin.Default()

	// Set up CORS middleware.
	router.Use(func(c *gin.Context) {
		c.Writer.Header().Set("Access-Control-Allow-Origin", "*")
		c.Writer.Header().Set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
		c.Writer.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization")
		if c.Request.Method == "OPTIONS" {
			c.AbortWithStatus(http.StatusOK)
			return
		}
		c.Next()
	})

	// Define CRUD routes.
	router.POST("/lists", CreateListItem)
	router.GET("/lists", GetListItems)
	router.GET("/lists/:id", GetListItem)
	router.PUT("/lists/:id", UpdateListItem)
	router.DELETE("/lists/:id", DeleteListItem)

	// Start the server on port 3000.
	if err := router.Run(":3000"); err != nil {
		log.Fatal("Server failed to start: ", err)
	}
}
