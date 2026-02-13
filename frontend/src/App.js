/**
 * Main App Component
 */

import React, { useState } from 'react';
import Dashboard from './components/Dashboard';
import PerformanceGraphs from './components/PerformanceGraphs';
import SettingsPanel from './components/SettingsPanel';
import Navbar from './components/Navbar';
import './styles/App.css';

function App() {
  const [activePage, setActivePage] = useState('dashboard');

  return (
    <div className="app">
      <Navbar activePage={activePage} onPageChange={setActivePage} />
      <main className="main-content">
        {activePage === 'dashboard' && <Dashboard />}
        {activePage === 'performance' && <PerformanceGraphs />}
        {activePage === 'settings' && <SettingsPanel />}
      </main>
    </div>
  );
}

export default App;
