import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import RadioGroup from '../../../components/form/RadioGroup';

let component;
let radios;

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
                default_value: 'choice2'
            } }
            handleChange= { () => {} } />,
    );
});

it('renders without crashing', () => {
    const tree = component.toJSON();
    expect(tree).toMatchSnapshot();
});

it('renders input[type=radio] components as children', () => { 
    const instance = component.root;
    const radios = instance.findAll(node => node.type === 'button');
    const values = radios.map(node => node.props.value);

    expect(values.toString()).toEqual('choice1,choice2');
    expect(radios.length).toEqual(2);
});

it('form element renders default value', () => { 
    const instance = component.root;
    const radios = instance.findAll(node => node.type === 'button');
    
    // selected button
    expect(radios[1].props.value).toEqual('choice2');
    const fa = radios[1].find(node => node.type === 'i');
    expect(fa.props.className).toContain('fa-check-circle');
});
