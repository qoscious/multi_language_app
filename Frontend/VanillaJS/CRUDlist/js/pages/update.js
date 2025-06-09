// js/pages/update.js
const API_BASE = 'http://localhost:3000';

export async function init(container) {
  // Retrieve listId from query parameters.
  const params = new URLSearchParams(window.location.search);
  const listId = params.get('listId');
  if (!listId) {
    container.innerHTML = '<p>Error: No list ID provided for update.</p>';
    return;
  }
  
  container.innerHTML = /* html */ `
    <h1>Update List</h1>
    <form id="updateForm">
      <div class="form-group">
        <label for="list">List Item:</label>
        <input type="text" name="list" id="list" required />
      </div>
      <button type="submit" class="btn">Update List</button>
    </form>
  `;
  
  const form = container.querySelector('#updateForm');
  try {
    const response = await fetch(`${API_BASE}/lists/${listId}`);
    if (!response.ok) {
      container.innerHTML = '<p>Error fetching list data.</p>';
      return;
    }
    const listData = await response.json();
    form.elements['list'].value = listData.list;
  } catch (error) {
    console.error(error);
    container.innerHTML = `<p>Error fetching list data.</p>`;
    return;
  }
  
  form.addEventListener('submit', async (event) => {
    event.preventDefault();
    const formData = new FormData(form);
    const data = Object.fromEntries(formData);
    try {
      const updateResponse = await fetch(`${API_BASE}/lists/${listId}`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
      });
      if (updateResponse.ok) {
        alert('List updated successfully');
        history.pushState(null, '', '/');
        import('./home.js').then(module => {
          container.innerHTML = '';
          module.init(container);
        });
      } else {
        const errorData = await updateResponse.json();
        alert(`Error: ${errorData.error || 'Failed to update list'}`);
      }
    } catch (error) {
      console.error(error);
      alert('Error updating list');
    }
  });
}
