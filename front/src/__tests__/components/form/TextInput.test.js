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
                default_value: 'teststring'
            } }
            handleChange= { () => {} } />, 
    );
});

it('renders without crashing', () => {
    const tree = component.toJSON();
    expect(tree).toMatchSnapshot();
});

it('form element renders default value', () => {
    const instance = component.root;
    const input = instance.find(node => node.type === 'input');
    expect(input.props.value).toEqual('teststring');
});