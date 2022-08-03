import React from 'react';
import ReactDOM from 'react-dom';
import Survey from '../../components/Survey';

// disable warnings from Session.js
const log = console.log;
console.warn = () => {};

const createMockXHR = (responseJSON) => {
    const mockXHR = {
        open: jest.fn(),
        send: jest.fn(),
        readyState: 4,
        responseText: JSON.stringify(
            responseJSON || {}
        ),
        setRequestHeader: jest.fn(),
    };
    return mockXHR;
}

const SurveyID = 'foo';

const match = {
    params: {
        id: SurveyID,
    },
};

/**
 * Workaround for: react-modal: No elements were found for selector #root.
 */
const createRoot = function() {
    const div = document.createElement('div');
    div.id = 'root';
    return div;
};

describe('withXHR mock', () => {
    const oldXMLHttpRequest = window.XMLHttpRequest;
    let mockXHR = null;

    beforeEach(() => {
        mockXHR = createMockXHR();
        window.XMLHttpRequest = jest.fn(() => mockXHR);
    });

    afterEach(() => {
        window.XMLHttpRequest = oldXMLHttpRequest;
    });

    it('renders without crashing', () => {
        const root = createRoot();
        ReactDOM.render(<Survey match={ match }/>, root);
        ReactDOM.unmountComponentAtNode(root);
    });
});
xit('should be implemented', () => {});

// restore logging
console.log = log;
