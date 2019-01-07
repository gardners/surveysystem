import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import RadioMatrix from '../../../components/form/RadioMatrix';

let component;

beforeAll(() => {
    component = renderer.create(
        <RadioMatrix
            questionGroup={ [{
                id: 'q1',
                name: 'q1',
                title: 'q1',
                title_text: 'q1',
                type: 'INT',
                choices: [ 'choice1', 'choice2' ],
            }, {
                id: 'q2',
                name: 'q2',
                title: 'q1',
                title_text: 'q2',
                type: 'INT',
                choices: [ 'choice1', 'choice2' ],
            }] }
            handleChange= { () => {} } />,
    );
});

it('renders without crashing', () => {
    const tree = component.toJSON();
    expect(tree).toMatchSnapshot();
});

it('renders inut[type=radio] components as children', () => {
    const instance = component.root;
    const body = instance.findByType('tbody');

    const rows = body.findAllByType('tr');
    expect(rows.length).toEqual(2);

    rows.forEach((row) => {
        const radios = row.findAll(node => node.type === 'input' &&  node.props.type === "radio");
        expect(radios.length).toEqual(2);
    });
});
