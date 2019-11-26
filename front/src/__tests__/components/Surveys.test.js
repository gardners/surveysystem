import React from 'react';
import ReactDOM from 'react-dom';
import { MemoryRouter } from 'react-router-dom';

import Surveys from '../../components/Surveys';


/**
 * Workaround for: react-modal: No elements were found for selector #root.
 */
const createRoot = function() {
    const div = document.createElement('div');
    div.id = 'root';
    return div;
};

it('renders without crashing', () => {
    const root = createRoot();
    ReactDOM.render(
        <MemoryRouter>
            <Surveys
                surveys={ ['test'] }
                surveyProvider={ 'test' }
            />
        </MemoryRouter>, root);
    ReactDOM.unmountComponentAtNode(root);
});
