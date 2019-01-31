import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import NumberInput from '../../../components/form/NumberInput';

let component;

beforeAll(() => {
    component = renderer.create(
        <NumberInput
            question={ {
                id: 'q1',
                name: 'q1',
                title: 'q1',
                description: 'q1',
                type: 'INT',
                unit: '',
            } }
            handleChange= { () => {} } />,
    );
});

it('renders without crashing', () => {
    const tree = component.toJSON();
    expect(tree).toMatchSnapshot();
});
