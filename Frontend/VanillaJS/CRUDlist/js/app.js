// js/app.js

// Route definitions: each path maps to a function that dynamically imports the page module.
const routes = {
  '/': () => import('./pages/home.js'),
  '/add': () => import('./pages/add.js'),
  '/update': () => import('./pages/update.js'),
};

const contentDiv = document.getElementById('content');

/**
* Loads the page for the given path.
* Dynamically imports the module and calls its init() method.
*/
async function loadPage(path) {
  // Use the route or fallback to home if not found.
  const route = routes[path] || routes['/'];
  try {
      const module = await route();
      // Clear any previous content
      contentDiv.innerHTML = '';
      // Call the page's init function, passing the container for rendering.
      if (module && typeof module.init === 'function') {
      module.init(contentDiv);
      }
      setActiveLink(path);
  } catch (error) {
      console.error('Error loading page:', error);
  }
}

/**
* Sets the "active" class on the current navigation link.
*/
function setActiveLink(path) {
  document.querySelectorAll('nav a[data-link]').forEach(link => {
      link.classList.toggle('active', link.getAttribute('href') === path);
  });
}

/**
* Setup Navigation API
*/

window.navigation.addEventListener('navigate', (event) => {
  event.intercept({
      async handler() {
          const destinationURL = new URL(event.destination.url);
          await loadPage(destinationURL.pathname);
      }
  });
});


// Load the initial page based on the URL.
loadPage(window.location.pathname);
