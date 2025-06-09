// js/pages/home.js
const API_BASE = 'http://localhost:3000';

export async function init(container) {
  container.innerHTML = /* html */ `
    <h1>List Manager</h1>
    <p>Below is the list of items:</p>
    <table>
      <thead>
        <tr>
          <th>#</th>
          <th>List Item</th>
          <th>Actions</th>
        </tr>
      </thead>
      <tbody id="listTableBody">
        <tr><td colspan="3">Loading...</td></tr>
      </tbody>
    </table>
  `;

  const tableBody = container.querySelector('#listTableBody');
  try {
    const response = await fetch(`${API_BASE}/lists`);
    if (!response.ok) {
      tableBody.innerHTML = `<tr><td colspan="3">Error fetching lists</td></tr>`;
      return;
    }
    const lists = await response.json();
    if (lists.length === 0) {
      tableBody.innerHTML = `<tr><td colspan="3">No lists available.</td></tr>`;
    } else {
      tableBody.innerHTML = '';
      lists.forEach((item, index) => {
        const row = document.createElement('tr');
        row.innerHTML = `
          <td>${index + 1}</td>
          <td>${item.list}</td>
          <td>
            <button class="btn" data-update-id="${item._id}">Update</button>
            <button class="btn btn-danger" data-delete-id="${item._id}">Delete</button>
          </td>
        `;
        tableBody.appendChild(row);
      });
    }
  } catch (error) {
    console.error(error);
    tableBody.innerHTML = `<tr><td colspan="3">Error fetching lists</td></tr>`;
  }

  // Attach event listeners for update buttons.
  container.querySelectorAll('button[data-update-id]').forEach(button => {
    button.addEventListener('click', () => {
      const listId = button.getAttribute('data-update-id');
      history.pushState(null, '', `/update?listId=${listId}`);
      import('./update.js').then(module => {
        container.innerHTML = '';
        module.init(container);
      });
    });
  });

  // Attach event listeners for delete buttons.
  container.querySelectorAll('button[data-delete-id]').forEach(button => {
    button.addEventListener('click', async () => {
      const listId = button.getAttribute('data-delete-id');
      if (confirm('Are you sure you want to delete this item?')) {
        try {
          const response = await fetch(`${API_BASE}/lists/${listId}`, {
            method: 'DELETE'
          });
          if (response.ok) {
            alert('List deleted successfully');
            history.pushState(null, '', '/');
            import('./home.js').then(module => {
              container.innerHTML = '';
              module.init(container);
            });
          } else {
            alert('Failed to delete the list');
          }
        } catch (error) {
          console.error(error);
          alert('Error deleting list');
        }
      }
    });
  });
}
