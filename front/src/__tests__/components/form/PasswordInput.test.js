import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import PasswordInput from '../../../components/form/PasswordInput';

let component;

beforeAll(() => {
    component = renderer.create(
        <PasswordInput
            question={ {
                id: 'q1',
                name: 'q1',
                title: 'q1',
                description: 'q1',
                type: 'FIXEDPOINT',
                unit: '',
                default_value: 'teststring',
            } }
            handleChange= { () => {} } />
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
