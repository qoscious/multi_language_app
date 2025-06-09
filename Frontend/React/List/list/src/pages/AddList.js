// src/pages/AddList.js
import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';

const API_BASE = 'http://localhost:3000';

function AddList() {
  const [listItem, setListItem] = useState('');
  const navigate = useNavigate();

  const handleSubmit = async (e) => {
    e.preventDefault();
    try {
      const response = await fetch(`${API_BASE}/lists`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({ list: listItem })
      });
      if (response.ok) {
        alert('List added successfully');
        navigate('/');
      } else {
        const errorData = await response.json();
        alert(`Error: ${errorData.error || 'Failed to add list'}`);
      }
    } catch (error) {
      console.error("Error adding list:", error);
      alert('Error adding list');
    }
  };

  return (
    <div>
      <h1>Add New List</h1>
      <form onSubmit={handleSubmit}>
        <div className="form-group">
          <label htmlFor="list">List Item:</label>
          <input
            type="text"
            id="list"
            name="list"
            value={listItem}
            onChange={(e) => setListItem(e.target.value)}
            required
          />
        </div>
        <button type="submit" className="btn">Add List</button>
      </form>
    </div>
  );
}

export default AddList;
