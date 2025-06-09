// js/pages/add.js
const API_BASE = 'http://localhost:3000';

export async function init(container) {
  container.innerHTML = /* html */ `
    <h1>Add New List</h1>
    <form id="addForm">
      <div class="form-group">
        <label for="list">List Item:</label>
        <input type="text" name="list" id="list" required />
      </div>
      <button type="submit" class="btn">Add List</button>
    </form>
  `;

  const form = container.querySelector('#addForm');
  form.addEventListener('submit', async (event) => {
    event.preventDefault();
    const formData = new FormData(form);
    const data = Object.fromEntries(formData);
    try {
      const response = await fetch(`${API_BASE}/lists`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
      });
      if (response.ok) {
        alert('List added successfully');
        history.pushState(null, '', '/');
        // Manually load the home page by importing home.js
        import('./home.js').then(module => {
          container.innerHTML = '';
          module.init(container);
        });
      } else {
        const errorData = await response.json();
        alert(`Error: ${errorData.error || 'Failed to add list'}`);
      }
    } catch (error) {
      console.error(error);
      alert('Error adding list');
    }
  });
}
