import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import RadioGroup from '../../../components/form/RadioGroup';

let component;

beforeAll(() => {
    component = renderer.create(
        <RadioGroup
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

it('renders inut[type=radio] components as children', () => {
    const instance = component.root;
    const radios = instance.findAll(node => node.type === 'input' &&  node.props.type === "radio");
    const values = radios.map(node => node.props.value);

    expect(values.toString()).toEqual('choice1,choice2');
    expect(radios.length).toEqual(2);
});
