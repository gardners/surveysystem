import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import GeoLocation from '../../../components/form/GeoLocation';

let component;

beforeAll(() => {
    component = renderer.create(
        <GeoLocation
            question={ {
                id: 'q1',
                name: 'q1',
                title: 'q1',
                description: 'q1',
                type: 'FIXEDPOINT',
                unit: '',
            } }
            handleChange= { () => {} } />
    );
});

it('renders without crashing', () => {
    const tree = component.toJSON();
    expect(tree).toMatchSnapshot();
});

it('defaults to no button: withButton = false', () => {
    const instance = component.root;
    expect(instance.props.withButton).toEqual(false);

    const btns = instance.findAllByType('button');
    expect(btns.length).toEqual(0);
});

it('renders button: withButton = true', () => {
   const comp = renderer.create(
        <GeoLocation
            question={ {
                id: 'q1',
                name: 'q1',
                title: 'q1',
                description: 'q1',
                type: 'FIXEDPOINT',
                unit: '',
            } }
            withButton={ true }
            handleChange= { () => {} } />
    );
    const instance = comp.root;
    expect(instance.props.withButton).toEqual(true);

    const btns = instance.findAllByType('button');
    expect(btns.length).toEqual(1);
});
