import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import RadioMatrix from '../../../components/form/RadioMatrix';

let component;

describe( 'expanded display', () => {

    beforeAll(() => {
        component = renderer.create(
            <RadioMatrix
                questions={ [{
                    id: 'q1',
                    name: 'q1',
                    title: 'q1',
                    description: 'q1',
                    type: 'INT',
                    choices: [ 'choice1', 'choice2' ],
                    unit: '',
                }, {
                    id: 'q2',
                    name: 'q2',
                    title: 'q1',
                    description: 'q2',
                    type: 'INT',
                    choices: [ 'choice1', 'choice2' ],
                    unit: '',
                }] }
                handleChange= { () => {} }
                expand={ true }
            />,
        );
    });

    it('renders without crashing', () => {
        const tree = component.toJSON();
        expect(tree).toMatchSnapshot();
    });

    it('renders inut[type=radio] components as children', () => {
        const instance = component.root;
        const tbody = instance.findByType('tbody');

        const rows = tbody.findAllByType('tr');
        expect(rows.length).toEqual(2);

        rows.forEach((row) => {
            const radios = row.findAll(node => node.type === 'input' &&  node.props.type === "radio");
            const values = radios.map(node => node.props.value);

            expect(values.toString()).toEqual('choice1,choice2');
            expect(radios.length).toEqual(2);
        });
    });

});

describe( 'contracted display', () => {

    beforeAll(() => {
        component = renderer.create(
            <RadioMatrix
                questions={ [{
                    id: 'q1',
                    name: 'q1',
                    title: 'q1',
                    description: 'q1',
                    type: 'INT',
                    choices: [ 'choice1', 'choice2' ],
                    unit: '',
                }, {
                    id: 'q2',
                    name: 'q2',
                    title: 'q1',
                    description: 'q2',
                    type: 'INT',
                    choices: [ 'choice1', 'choice2' ],
                    unit: '',
                }] }
                handleChange= { () => {} }
                expand={ false }
            />,
        );
    });

    it('renders without crashing', () => {
        const tree = component.toJSON();
        expect(tree).toMatchSnapshot();
    });

    it('renders inut[type=radio] components as children', () => {
        const instance = component.root;
        const tbody = instance.findByType('tbody');

        const rows = tbody.findAllByType('tr');
        expect(rows.length).toEqual(2);

        const selects = tbody.findAllByType('select');
        expect(selects.length).toEqual(2);

        selects.forEach((select) => {
            const options = select.findAllByType('option');
            const values = options.map(node => node.props.value);

            // #114, with empty start option
            expect(values.toString()).toEqual(',choice1,choice2');
            expect(options.length).toEqual(3);
        });
    });

});
