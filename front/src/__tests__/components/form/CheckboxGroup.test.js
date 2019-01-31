import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import CheckboxGroup from '../../../components/form/CheckboxGroup';

let component;

beforeAll(() => {
    component = renderer.create(
        <CheckboxGroup
            question={ {
                id: 'q1',
                name: 'q1',
                title: 'q1',
                description: 'q1',
                type: 'TEXT',
                choices: [ 'choice1', 'choice2' ],
                unit: '',
            } }
            handleChange= { () => {} } />,
    );
});

it('renders without crashing', () => {
    const tree = component.toJSON();
    expect(tree).toMatchSnapshot();
});

it('renders inut[type=checkbox] components as children', () => {
    const instance = component.root;
    const boxes = instance.findAll(node => node.type === 'input' &&  node.props.type === "checkbox");
    const values = boxes.map(node => node.props.value);

    expect(values.toString()).toEqual('choice1,choice2');
    expect(boxes.length).toEqual(2);
});
