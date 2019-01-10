import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import MultiSelect from '../../../components/form/Select';

let component;

beforeAll(() => {
    component = renderer.create(
        <MultiSelect
            question={ {
                id: 'q1',
                name: 'q1',
                title: 'q1',
                title_text: 'q1',
                type: 'TEXT',
                choices: [ 'choice1', 'choice2' ],
                unit: '',
            } }
            handleChange= { () => {} } />,
    );
});

it('renders without crashing', () => {
    const tree = component.toJSON();
    expect(tree).toMatchSnapshot();
});

it('renders options from question.choices', () => {
    const instance = component.root;
    const options = instance.findAllByType('option');
    const values = options.map(node => node.props.value);

    expect(values.toString()).toEqual('choice1,choice2');
    expect(options.length).toEqual(2);
});
