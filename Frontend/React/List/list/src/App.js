// src/App.js
import React from 'react';
import { BrowserRouter as Router, Routes, Route } from 'react-router-dom';
import NavBar from './components/NavBar';
import Home from './pages/Home';
import AddList from './pages/AddList';
import UpdateList from './pages/UpdateList';
import './App.css';

function App() {
  return (
    <Router>
      <NavBar />
      <div className="container">
        <Routes>
          <Route path="/" element={<Home />} />
          <Route path="/add" element={<AddList />} />
          <Route path="/update/:id" element={<UpdateList />} />
        </Routes>
      </div>
    </Router>
  );
}

export default App;
