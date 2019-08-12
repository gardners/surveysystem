import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import InputRange from 'react-input-range';
import DayTimeSlider from '../../../components/form/DayTimeSlider';

let component;

beforeAll(() => {
    component = renderer.create(
        <DayTimeSlider
            question={ {
                id: 'q1',
                name: 'q1',
                title: 'q1',
                description: 'q1',
                type: 'FIXEDPOINT',
                unit: '',
                default_value: '28800'
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
    const input = instance.findByType(InputRange);
    expect(input.props.value).toEqual(28800);
});