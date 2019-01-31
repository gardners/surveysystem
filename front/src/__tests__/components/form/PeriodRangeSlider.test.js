import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

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
            } }
            placeholder="teststring"
            handleChange= { () => {} } />
    );
});

it('renders without crashing', () => {
    const tree = component.toJSON();
    expect(tree).toMatchSnapshot();
});

