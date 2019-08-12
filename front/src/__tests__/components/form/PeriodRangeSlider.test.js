import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import InputRange from 'react-input-range';
import PeriodRangeSlider from '../../../components/form/PeriodRangeSlider';

let component;

beforeAll(() => {
    component = renderer.create(
        <PeriodRangeSlider
            question={ {
                id: 'q1',
                name: 'q1',
                title: 'q1',
                description: 'q1',
                type: 'FIXEDPOINT',
                unit: '',
                default_value:'74745,81945', // 08:45:45 pm to 22:45:45 pm
            } }
            placeholder="teststring"
            handleChange= { () => {} } />
    );
});

it('renders without crashing', () => {
    const tree = component.toJSON();
    expect(tree).toMatchSnapshot();
});

it('form element renders default value', () => {
    const instance = component.root;
    const slider = instance.findByType(InputRange); 
    expect(slider.props.minValue).toEqual(0); 
    expect(slider.props.maxValue).toEqual(86400); 
    expect(slider.props.value).toHaveProperty('min', 74745); // 08:45:45 pm
    expect(slider.props.value).toHaveProperty('max', 81945); // 22:45:45 pm
});
