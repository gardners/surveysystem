import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import TextInput from '../../../components/form/TextInput';

let component;

beforeAll(() => {
    component = renderer.create(
        <TextInput
            question={ {
                id: 'q1',
                name: 'q1',
                title: 'q1',
                description: 'q1',
                type: 'TEXT',
                unit: '',
            } }
            placeholder="teststring"
            handleChange= { () => {} } />,
    );
});

it('renders without crashing', () => {
    const tree = component.toJSON();
    expect(tree).toMatchSnapshot();
});

it('renders inut[type=radio] components as children', () => {
    const instance = component.root;
    const input = instance.findByType('input');
    const placeholder = input.props.placeholder;

    expect(input.type).toEqual('input');
    expect(placeholder).toEqual('teststring');
});
