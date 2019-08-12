import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import MultiSelect from '../../../components/form/MultiSelect';

let component;

beforeAll(() => {
    component = renderer.create(
        <MultiSelect
            question={ {
                id: 'q1',
                name: 'q1',
                title: 'q1',
                description: 'q1',
                type: 'TEXT',
                choices: [ 'choice1', 'choice2', 'choice3' ],
                unit: '',
                default_value: 'choice1,choice3',
            } }
            handleChange= { () => {} } />,
    );
});

it('renders without crashing', () => {
    const tree = component.toJSON();
    expect(tree).toMatchSnapshot();
});

it('renders options from question.choices', () => {
    const instance = component.root;
    const options = instance.findAllByType('option');
    const values = options.map(node => node.props.value);

    expect(values.toString()).toEqual('choice1,choice2,choice3');
    expect(options.length).toEqual(3);
});

it('form element renders default value', () => {
    const instance = component.root;
    const input = instance.find(node => node.type === 'select'); 
    expect(input.props.value.toString()).toEqual('choice1,choice3');
});
