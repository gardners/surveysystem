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
    choices: [ 'unchecked', 'checked' ],
    unit: '',
    default_value: 'checked',
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

it('form element renders default value', () => { 
    const instance = component.root;
    const input = instance.find(node => node.type === 'button'); //
    const fa = input.find(node => node.type === 'i');
    expect(fa.props.className).toContain('fa-check-square');
});