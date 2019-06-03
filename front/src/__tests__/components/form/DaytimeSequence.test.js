import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import InputRange from 'react-input-range';
import TimePicker from '../../../components/form/TimePicker';
import DaytimeSequence from '../../../components/form/DaytimeSequence';

let component;

describe( 'expanded display', () => {

    beforeAll(() => {
        component = renderer.create(
            <DaytimeSequence
                questions={ [{
                    id: 'q1',
                    name: 'q1',
                    title: 'q1',
                    description: 'q1',
                    type: 'DAYTIME',
                    unit: 'seconds',
                }, {
                    id: 'q2',
                    name: 'q2',
                    title: 'q1',
                    description: 'q2',
                    type: 'DAYTIME',
                    unit: 'seconds',
                }] }
                expand={ true }
                handleChange= { () => true }
            />,
        );
    });

    it('renders without crashing', () => {
        const tree = component.toJSON();
        expect(tree).toMatchSnapshot();
    });

    it('renders Inputrange components as children', () => {
        const instance = component.root;
        const tbody = instance.findByType('tbody');

        const rows = tbody.findAllByType('tr');
        expect(rows.length).toEqual(2);

        rows.forEach((row) => {
            const children = row.findAllByType(InputRange);
            expect(children.length).toEqual(1);
        });

    });

});

describe( 'contracted display', () => {//

    beforeAll(() => {
        component = renderer.create(
            <DaytimeSequence
                questions={ [{
                    id: 'q1',
                    name: 'q1',
                    title: 'q1',
                    description: 'q1',
                    type: 'DAYTIME',
                    unit: 'seconds',
                }, {
                    id: 'q2',
                    name: 'q2',
                    title: 'q1',
                    description: 'q2',
                    type: 'DAYTIME',
                    unit: 'seconds',
                }] }
                expand={ false }
                handleChange= { () => true }
            />,
        );
    });

    it('renders without crashing', () => {
        const tree = component.toJSON();
        expect(tree).toMatchSnapshot();
    });

    it('renders TimePicker components as children', () => {
        const instance = component.root;
        const tbody = instance.findByType('tbody');

        const rows = tbody.findAllByType('tr');
        expect(rows.length).toEqual(2);

        rows.forEach((row) => {
            const children = row.findAllByType(TimePicker);
            expect(children.length).toEqual(1);
        });
    });

});
