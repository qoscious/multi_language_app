package com.example.listapi.repository;

import com.example.listapi.model.ListItem;
import org.springframework.data.mongodb.repository.MongoRepository;
import org.springframework.stereotype.Repository;

@Repository
public interface ListItemRepository extends MongoRepository<ListItem, String> {
}
