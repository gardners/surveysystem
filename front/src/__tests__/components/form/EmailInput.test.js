import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import EmailInput from '../../../components/form/EmailInput';

let component;

beforeAll(() => {
    component = renderer.create(
        <EmailInput
            question={ {
                id: 'q1',
                name: 'q1',
                title: 'q1',
                title_text: 'q1',
                type: 'FIXEDPOINT',
            } }
            placeholder="teststring"
            handleChange= { () => {} } />
    );
});

it('renders without crashing', () => {
    const tree = component.toJSON();
    expect(tree).toMatchSnapshot();
});

it('input child renders placeholder', () => {
    const instance = component.root;
    const input = instance.find(node => node.type === 'input');
    expect(input.props.placeholder).toEqual('teststring');
});
