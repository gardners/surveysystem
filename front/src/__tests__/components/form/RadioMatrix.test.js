import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import { AppContext } from '../../../Context';

import RadioMatrix from '../../../components/form/RadioMatrix';

let component;

describe( 'expanded display', () => {

    beforeAll(() => {
        component = renderer.create(
            <AppContext.Provider value={ {
                    matchesBreakpointOrAbove: () => true,
            } }>
                <RadioMatrix
                    questions={ [{
                        id: 'q1',
                        name: 'q1',
                        title: 'q1',
                        description: 'q1',
                        type: 'INT',
                        choices: [ 'choice1', 'choice2', 'choice3' ],
                        unit: '',
                        default_value: 'choice2',
                    }, {
                        id: 'q2',
                        name: 'q2',
                        title: 'q1',
                        description: 'q2',
                        type: 'INT',
                        choices: [ 'choice1', 'choice2', 'choice3' ],
                        unit: '',
                        default_value: 'choice3',
                    }] }
                    handleChange= { () => {} }
                    expand={ true }
                    errors={ {} }
                />
            </AppContext.Provider>
        );
    });

    it('renders without crashing', () => {
        const tree = component.toJSON();
        expect(tree).toMatchSnapshot();
    });

    it('renders input[type=radio] components as children', () => {
        const instance = component.root;
        const tbody = instance.findByType('tbody');

        const rows = tbody.findAllByType('tr');
        expect(rows.length).toEqual(2);

        rows.forEach((row) => {
            const radios = row.findAllByType('button');
            const values = radios.map(node => node.props.value);

            expect(values.toString()).toEqual('choice1,choice2,choice3');
            expect(radios.length).toEqual(3);
        });

        let fa;
        // row 1 default_value
        fa = rows[0].findAllByType('i');
        expect(fa[0].props.className).toContain('fa-circle');
        expect(fa[1].props.className).toContain('fa-check-circle');
        expect(fa[2].props.className).toContain('fa-circle');

        // row 2 default_value
        fa = rows[1].findAllByType('i');
        expect(fa[0].props.className).toContain('fa-circle');
        expect(fa[1].props.className).toContain('fa-circle');
        expect(fa[2].props.className).toContain('fa-check-circle');
    });

});

describe( 'contracted display', () => {

    beforeAll(() => {
        component = renderer.create(
            <AppContext.Provider value={ {
                    matchesBreakpointOrAbove: () => false,
            } }>
                <RadioMatrix
                    questions={ [{
                        id: 'q1',
                        name: 'q1',
                        title: 'q1',
                        description: 'q1',
                        type: 'INT',
                        choices: [ 'choice1', 'choice2', 'choice3' ],
                        unit: '',
                        default_value: 'choice2',
                    }, {
                        id: 'q2',
                        name: 'q2',
                        title: 'q1',
                        description: 'q2',
                        type: 'INT',
                        choices: [ 'choice1', 'choice2', 'choice3' ],
                        unit: '',
                        default_value: 'choice3',
                    }] }
                    handleChange= { () => {} }
                    expand={ true }
                    errors={ {} }
                />
            </AppContext.Provider>
        );
    });

    it('renders without crashing', () => {
        const tree = component.toJSON();
        expect(tree).toMatchSnapshot();
    });

    it('renders input[type=radio] components as children', () => {
        const instance = component.root;
        const tbody = instance.findByType('tbody');

        const rows = tbody.findAllByType('select');
        expect(rows.length).toEqual(2);

        let options;
        let values;

        // row 1 values
        options = rows[0].findAllByType('option');
        values = options.map(node => node.props.value);

        expect(values.toString()).toEqual('choice1,choice2,choice3');
        expect(options.length).toEqual(3);
        expect(rows[0].props.value).toEqual('choice2'); // default_value

        // row 2 values
        options = rows[1].findAllByType('option');
        values = options.map(node => node.props.value);

        expect(values.toString()).toEqual('choice1,choice2,choice3');
        expect(options.length).toEqual(3);
        expect(rows[1].props.value).toEqual('choice3'); // default_value
    });

});
