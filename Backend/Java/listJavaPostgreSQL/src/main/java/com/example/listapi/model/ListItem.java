package com.example.listapi.model;

import javax.persistence.*;

@Entity
@Table(name = "lists")
public class ListItem {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(name = "list", nullable = false, length = 200)
    private String list;

    public ListItem() {
    }

    public ListItem(String list) {
        this.list = list;
    }

    public Long getId() {
        return id;
    }

    public String getList() {
        return list;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public void setList(String list) {
        this.list = list;
    }
}
