import React from 'react';
import ReactDOM from 'react-dom';
import { MemoryRouter } from 'react-router-dom';

import HeaderNav from '../../components/HeaderNav';

const location = {
    pathname: 'test',
};

it('renders without crashing', () => {
    const div = document.createElement('div');
    ReactDOM.render(
        <MemoryRouter>
            <HeaderNav location={ location }/>
        </MemoryRouter>, div);
    ReactDOM.unmountComponentAtNode(div);
});
