import React from 'react';
import ReactDOM from 'react-dom';
import FlexibleSurvey from '../../components/FlexibleSurvey';

const createMockXHR = (responseJSON) => {
    const mockXHR = {
        open: jest.fn(),
        send: jest.fn(),
        readyState: 4,
        responseText: JSON.stringify(
            responseJSON || {}
        )
    };
    return mockXHR;
}

const SurveyID = 'foo';

const match = {
    params: {
        id: SurveyID,
    },
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
        const div = document.createElement('div');
        ReactDOM.render(<FlexibleSurvey match={ match }/>, div);
        ReactDOM.unmountComponentAtNode(div);
    });
});
xit('should be implemented', () => {});
