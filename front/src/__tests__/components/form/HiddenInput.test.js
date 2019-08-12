import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import HiddenInput from '../../../components/form/HiddenInput';

let component;

const question = {
    id: 'q1',
    name: 'q1',
    title: 'q1',
    description: 'q1',
    type: 'TEXT',
    unit: '',
    default_value: 'teststring',
};

const mockCallback = jest.fn((element, question, value) => {
    return value;
});

beforeAll(() => {
    component = renderer.create(
        <HiddenInput
            question={ question }
            handleChange={ mockCallback } />,
    );
});

it('renders without crashing', () => {
    const tree = component.toJSON();
    expect(tree).toMatchSnapshot();
});

it('handleChange callback is invoked on mount', () => {
    // is called once
    expect(mockCallback.mock.calls.length).toBe(1);
    // first argument is the question
    expect(mockCallback.mock.calls[0][1]).toEqual(expect.objectContaining(question));
    // second argument is the defaultValue as value
    expect(mockCallback.mock.calls[0][2]).toBe("teststring");
    // returns defaultValue as value
    expect(mockCallback.mock.results[0].value).toBe("teststring");
});

it('form element renders default value', () => {
    const instance = component.root;
    const input = instance.find(node => node.type === 'input');
    expect(input.props.value).toEqual('teststring');
});