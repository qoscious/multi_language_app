// src/pages/UpdateList.js
import React, { useState, useEffect } from 'react';
import { useNavigate, useParams } from 'react-router-dom';

const API_BASE = 'http://localhost:3000';

function UpdateList() {
  const { id } = useParams();
  const [listItem, setListItem] = useState('');
  const [loading, setLoading] = useState(true);
  const navigate = useNavigate();

  const fetchList = async () => {
    try {
      const response = await fetch(`${API_BASE}/lists/${id}`);
      if (response.ok) {
        const data = await response.json();
        setListItem(data.list);
      } else {
        alert('Error fetching list data');
      }
    } catch (error) {
      console.error("Error fetching list data:", error);
      alert('Error fetching list data');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchList();
  }, [id]);

  const handleSubmit = async (e) => {
    e.preventDefault();
    try {
      const response = await fetch(`${API_BASE}/lists/${id}`, {
        method: 'PUT',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({ list: listItem })
      });
      if (response.ok) {
        alert('List updated successfully');
        navigate('/');
      } else {
        const errorData = await response.json();
        alert(`Error: ${errorData.error || 'Failed to update list'}`);
      }
    } catch (error) {
      console.error("Error updating list:", error);
      alert('Error updating list');
    }
  };

  if (loading) {
    return <p>Loading...</p>;
  }

  return (
    <div>
      <h1>Update List</h1>
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
        <button type="submit" className="btn">Update List</button>
      </form>
    </div>
  );
}

export default UpdateList;
