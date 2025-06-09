package com.example.listapi.controller;

import com.example.listapi.model.ListItem;
import com.example.listapi.repository.ListItemRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.*;

@RestController
@RequestMapping("/lists")
@CrossOrigin(origins = "*")  // Allow CORS from any origin
public class ListController {

    private final ListItemRepository repository;

    @Autowired
    public ListController(ListItemRepository repository) {
        this.repository = repository;
    }

    // Create a new list item
    @PostMapping
    public ResponseEntity<?> createListItem(@RequestBody Map<String, String> payload) {
        String listValue = payload.get("list");
        if (listValue == null || listValue.trim().isEmpty()) {
            return ResponseEntity.badRequest().body(Collections.singletonMap("error", "List field is required"));
        }
        if (listValue.trim().length() > 200) {
            return ResponseEntity.badRequest().body(Collections.singletonMap("error", "List field exceeds maximum length of 200"));
        }
        ListItem newItem = new ListItem(listValue.trim());
        ListItem savedItem = repository.save(newItem);
        Map<String, Object> response = new HashMap<>();
        response.put("message", "List created successfully");
        response.put("data", savedItem);
        return new ResponseEntity<>(response, HttpStatus.CREATED);
    }

    // Get all list items
    @GetMapping
    public List<ListItem> getAllListItems() {
        return repository.findAll();
    }

    // Get a single list item by ID
    @GetMapping("/{id}")
    public ResponseEntity<?> getListItem(@PathVariable Long id) {
        Optional<ListItem> item = repository.findById(id);
        if (item.isPresent()) {
            return ResponseEntity.ok(item.get());
        } else {
            return ResponseEntity.status(HttpStatus.NOT_FOUND)
                                 .body(Collections.singletonMap("error", "List not found"));
        }
    }

    // Update a list item by ID
    @PutMapping("/{id}")
    public ResponseEntity<?> updateListItem(@PathVariable Long id, @RequestBody Map<String, String> payload) {
        Optional<ListItem> itemOpt = repository.findById(id);
        if (!itemOpt.isPresent()) {
            return ResponseEntity.status(HttpStatus.NOT_FOUND)
                                 .body(Collections.singletonMap("error", "List not found for update"));
        }
        String listValue = payload.get("list");
        if (listValue == null || listValue.trim().isEmpty()) {
            return ResponseEntity.badRequest()
                                 .body(Collections.singletonMap("error", "List field is required for update"));
        }
        if (listValue.trim().length() > 200) {
            return ResponseEntity.badRequest()
                                 .body(Collections.singletonMap("error", "List field exceeds maximum length of 200"));
        }
        ListItem item = itemOpt.get();
        item.setList(listValue.trim());
        ListItem updatedItem = repository.save(item);
        Map<String, Object> response = new HashMap<>();
        response.put("message", "List updated successfully");
        response.put("data", updatedItem);
        return ResponseEntity.ok(response);
    }

    // Delete a list item by ID
    @DeleteMapping("/{id}")
    public ResponseEntity<?> deleteListItem(@PathVariable Long id) {
        Optional<ListItem> itemOpt = repository.findById(id);
        if (!itemOpt.isPresent()) {
            return ResponseEntity.status(HttpStatus.NOT_FOUND)
                                 .body(Collections.singletonMap("error", "List not found for deletion"));
        }
        repository.delete(itemOpt.get());
        return ResponseEntity.ok(Collections.singletonMap("message", "List deleted successfully"));
    }
}
