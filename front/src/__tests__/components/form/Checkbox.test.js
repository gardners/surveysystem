import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import Checkbox from '../../../components/form/Checkbox';

let component;

const question = {
    id: 'q1',
    name: 'q1',
    title: 'q1',
    description: 'q1',
    type: 'CHECKBOX',
    choices: [ 'uncheckedVal', 'checkedVal' ],
    unit: '',
};

beforeAll(() => {
    component = renderer.create(
        <Checkbox
            question={ question }
            handleChange={ () => {} } />,
    );
});

it('renders without crashing', () => {
    const tree = component.toJSON();
    expect(tree).toMatchSnapshot();
});

// TODO test  handleChange supplying first choice
