import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

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
            } }
            handleChange= { () => {} } />,
    );
});

it('renders without crashing', () => {
    const tree = component.toJSON();
    expect(tree).toMatchSnapshot();
});
