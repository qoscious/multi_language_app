package main

import (
	"log"
	"net/http"
	"strings"

	"github.com/gin-gonic/gin"
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

// CORSMiddleware sets CORS headers similar to the Node.js example.
func CORSMiddleware() gin.HandlerFunc {
	return func(c *gin.Context) {
		c.Writer.Header().Set("Access-Control-Allow-Origin", "*")
		c.Writer.Header().Set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
		c.Writer.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization")
		if c.Request.Method == "OPTIONS" {
			c.AbortWithStatus(200)
			return
		}
		c.Next()
	}
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
func CreateListItem(c *gin.Context) {
	var item ListItem
	if err := c.ShouldBindJSON(&item); err != nil {
		c.AbortWithError(http.StatusBadRequest, err)
		return
	}
	if msg, ok := ValidateListField(item.List); !ok {
		c.AbortWithStatusJSON(http.StatusBadRequest, gin.H{"error": msg})
		return
	}
	if err := mgm.Coll(&item).Create(&item); err != nil {
		c.AbortWithError(http.StatusInternalServerError, err)
		return
	}
	c.JSON(http.StatusCreated, gin.H{"message": "List created successfully", "data": item})
}

// GetListItems handles GET /lists and returns all list documents.
func GetListItems(c *gin.Context) {
	var items []ListItem
	if err := mgm.Coll(&ListItem{}).SimpleFind(&items, bson.M{}); err != nil {
		c.AbortWithError(http.StatusInternalServerError, err)
		return
	}

	// Convert each item to a map with a string _id
	var responseItems []gin.H
	for _, item := range items {
		responseItems = append(responseItems, gin.H{
			"_id":  item.ID.Hex(),
			"list": item.List,
		})
	}
	c.JSON(http.StatusOK, responseItems)
}

// GetListItem handles GET /lists/:id and returns a single document by ID.
func GetListItem(c *gin.Context) {
	id := c.Param("id")
	var item ListItem
	if err := mgm.Coll(&ListItem{}).FindByID(id, &item); err != nil {
		c.AbortWithStatusJSON(http.StatusNotFound, gin.H{"error": "List not found"})
		return
	}
	c.JSON(http.StatusOK, gin.H{
		"_id":  item.ID.Hex(),
		"list": item.List,
	})
}

// UpdateListItem handles PUT /lists/:id.
// It validates input and updates the list field.
func UpdateListItem(c *gin.Context) {
	id := c.Param("id")
	var item ListItem
	if err := mgm.Coll(&ListItem{}).FindByID(id, &item); err != nil {
		c.AbortWithStatusJSON(http.StatusNotFound, gin.H{"error": "List not found"})
		return
	}

	// Bind only the update data.
	var updateData struct {
		List string `json:"list"`
	}
	if err := c.ShouldBindJSON(&updateData); err != nil {
		c.AbortWithError(http.StatusBadRequest, err)
		return
	}
	if msg, ok := ValidateListField(updateData.List); !ok {
		c.AbortWithStatusJSON(http.StatusBadRequest, gin.H{"error": msg})
		return
	}

	item.List = updateData.List
	if err := mgm.Coll(&item).Update(&item); err != nil {
		c.AbortWithError(http.StatusInternalServerError, err)
		return
	}
	c.JSON(http.StatusOK, gin.H{"message": "List updated successfully", "data": item})
}

// DeleteListItem handles DELETE /lists/:id and removes a document.
func DeleteListItem(c *gin.Context) {
	id := c.Param("id")
	var item ListItem
	if err := mgm.Coll(&ListItem{}).FindByID(id, &item); err != nil {
		c.AbortWithStatusJSON(http.StatusNotFound, gin.H{"error": "List not found"})
		return
	}
	if err := mgm.Coll(&item).Delete(&item); err != nil {
		c.AbortWithError(http.StatusInternalServerError, err)
		return
	}
	c.JSON(http.StatusOK, gin.H{"message": "List deleted successfully"})
}

func main() {
	// Initialize mgm with the MongoDB connection.
	// Database name: listdb; MongoDB URI: mongodb://localhost:27017
	if err := mgm.SetDefaultConfig(nil, "listdb", options.Client().ApplyURI("mongodb://localhost:27017")); err != nil {
		log.Fatal("Error connecting to MongoDB: ", err)
	}

	// Initialize Gin with default middleware.
	router := gin.Default()

	// Attach CORS
	router.Use(CORSMiddleware())

	// Define CRUD routes with endpoint /lists.
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
