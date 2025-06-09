// src/components/NavBar.js
import React from 'react';
import { Link, useLocation } from 'react-router-dom';

function NavBar() {
  const location = useLocation();
  return (
    <nav>
      <Link to="/" className={location.pathname === "/" ? "active" : ""}>
        Home
      </Link>
      <Link to="/add" className={location.pathname === "/add" ? "active" : ""}>
        Add List
      </Link>
    </nav>
  );
}

export default NavBar;
