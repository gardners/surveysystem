import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import DeviceLocation from '../../../components/form/DeviceLocation';

let component;

beforeAll(() => {
    component = renderer.create(
        <DeviceLocation
            question={ {
                id: 'q1',
                name: 'q1',
                title: 'q1',
                description: 'q1',
                type: 'FIXEDPOINT',
                unit: '',
                default_value: 'default_value is ignored for this component',
            } }
            handleChange= { () => {} } />
    );
});

it('renders without crashing', () => {
    const tree = component.toJSON();
    expect(tree).toMatchSnapshot();
});

it('defaults to withButton=true', () => {
    const instance = component.root;
    expect(instance.props.withButton).toEqual(true);

    const btns = instance.findAllByType('button');
    expect(btns.length).toEqual(1);
});

it('form element renders default value', () => {
    const instance = component.root;
    const input = instance.find(node => node.type === 'input');
    // default_value is ignored for this component
    expect(input.props.value).toEqual('');
});