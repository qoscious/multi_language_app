package com.example.listapi.model;

import org.springframework.data.annotation.Id;
import org.springframework.data.mongodb.core.mapping.Document;
import com.fasterxml.jackson.annotation.JsonProperty;

@Document(collection = "lists")
public class ListItem {

    @Id
    @JsonProperty("_id")
    private String id;

    private String list;

    public ListItem() {}

    public ListItem(String list) {
        this.list = list;
    }

    public String getId() {
        return id;
    }

    public String getList() {
        return list;
    }

    public void setId(String id) {
        this.id = id;
    }

    public void setList(String list) {
        this.list = list;
    }
}
