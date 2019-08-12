import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import TimePicker from '../../../components/form/TimePicker';

let component;

beforeAll(() => {
    component = renderer.create(
        <TimePicker
            question={ {
                id: 'q1',
                name: 'q1',
                title: 'q1',
                description: 'q1',
                type: 'INT',
                unit: '',
                default_value: '74745 ', // 08:45:45 pm
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
    
    const inputs = instance.findAll(node => node.type === 'input');
    expect(inputs.length).toEqual(2);
    const selects = instance.findAll(node => node.type === 'select');
    expect(selects.length).toEqual(1);
    
    // hours
    expect(inputs[0].props.min).toEqual(0);
    expect(inputs[0].props.max).toEqual(12);
    expect(inputs[0].props.value).toEqual(8);
    
    // minutes
    expect(inputs[1].props.min).toEqual(0);
    expect(inputs[1].props.max).toEqual(60);
    expect(inputs[1].props.value).toEqual(45);
    
    // am/pm
    const options = selects[0].findAllByType('option');
    const values = options.map(node => node.props.value);
    expect(values.toString()).toEqual('am,pm');
    expect(options.length).toEqual(2);
    expect(selects[0].props.value).toEqual('pm');
});