/**
 * Navbar Component
 */

import React from 'react';

function Navbar({ activePage, onPageChange }) {
  return (
    <nav className="navbar">
      <div className="navbar-brand">
        <h1>🎮 RootStream Dashboard</h1>
      </div>
      <div className="navbar-menu">
        <button
          className={activePage === 'dashboard' ? 'active' : ''}
          onClick={() => onPageChange('dashboard')}
        >
          Dashboard
        </button>
        <button
          className={activePage === 'performance' ? 'active' : ''}
          onClick={() => onPageChange('performance')}
        >
          Performance
        </button>
        <button
          className={activePage === 'settings' ? 'active' : ''}
          onClick={() => onPageChange('settings')}
        >
          Settings
        </button>
      </div>
    </nav>
  );
}

export default Navbar;
