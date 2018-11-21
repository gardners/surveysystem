import React from 'react';
import ReactDOM from 'react-dom';
import { MemoryRouter } from 'react-router-dom';

import Navbar from '../../components/Navbar';

it('renders without crashing', () => {
    const div = document.createElement('div');
    ReactDOM.render(
        <MemoryRouter>
            <Navbar />
        </MemoryRouter>, div);
    ReactDOM.unmountComponentAtNode(div);
});
