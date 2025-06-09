// src/pages/Home.js
import React, { useState, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';

const API_BASE = 'http://localhost:3000';

function Home() {
  const [lists, setLists] = useState([]);
  const [loading, setLoading] = useState(true);
  const navigate = useNavigate();

  const fetchLists = async () => {
    try {
      const response = await fetch(`${API_BASE}/lists`);
      if (response.ok) {
        const data = await response.json();
        setLists(data);
      } else {
        console.error("Error fetching lists");
      }
    } catch (error) {
      console.error("Error fetching lists:", error);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchLists();
  }, []);

  const handleDelete = async (id) => {
    if (window.confirm('Are you sure you want to delete this item?')) {
      try {
        const response = await fetch(`${API_BASE}/lists/${id}`, {
          method: 'DELETE',
        });
        if (response.ok) {
          alert('List deleted successfully');
          fetchLists(); // Refresh the list
        } else {
          alert('Failed to delete the list');
        }
      } catch (error) {
        console.error("Error deleting list:", error);
        alert('Error deleting list');
      }
    }
  };

  return (
    <div>
      <h1>List Manager</h1>
      <p>Below is the list of items:</p>
      {loading ? (
        <p>Loading...</p>
      ) : (
        <table>
          <thead>
            <tr>
              <th>#</th>
              <th>List Item</th>
              <th>Actions</th>
            </tr>
          </thead>
          <tbody>
            {lists.length === 0 ? (
              <tr>
                <td colSpan="3">No lists available.</td>
              </tr>
            ) : (
              lists.map((item, index) => (
                <tr key={item._id}>
                  <td>{index + 1}</td>
                  <td>{item.list}</td>
                  <td>
                    <button
                      className="btn"
                      onClick={() => navigate(`/update/${item._id}`)}
                    >
                      Update
                    </button>
                    <button
                      className="btn btn-danger"
                      onClick={() => handleDelete(item._id)}
                    >
                      Delete
                    </button>
                  </td>
                </tr>
              ))
            )}
          </tbody>
        </table>
      )}
    </div>
  );
}

export default Home;
