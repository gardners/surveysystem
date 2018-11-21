import React from 'react';
import ReactDOM from 'react-dom';
import { MemoryRouter } from 'react-router-dom';

import Surveys from '../../components/Surveys';

it('renders without crashing', () => {
    const div = document.createElement('div');
    ReactDOM.render(
        <MemoryRouter>
            <Surveys />
        </MemoryRouter>, div);
    ReactDOM.unmountComponentAtNode(div);
});
