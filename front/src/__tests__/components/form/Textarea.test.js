import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import Textarea from '../../../components/form/Textarea';

let component;

beforeAll(() => {
    component = renderer.create(
        <Textarea
            question={ {
                id: 'q1',
                name: 'q1',
                title: 'q1',
                description: 'q1',
                type: 'TEXT',
                unit: '',
            } }
            placeholder="teststring"
            handleChange= { () => {} } />,
    );
});

it('renders without crashing', () => {
    const tree = component.toJSON();
    expect(tree).toMatchSnapshot();
});

it('renders inut[type=radio] components as children', () => {
    const instance = component.root;
    const textarea = instance.findByType('textarea');
    const placeholder = textarea.props.placeholder;

    expect(textarea.type).toEqual('textarea');
    expect(placeholder).toEqual('teststring');
});
