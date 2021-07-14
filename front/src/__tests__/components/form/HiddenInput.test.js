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

let _callbacks = 0;
let _args = [];

const mockCallback = (element, question, value) => {
    _args = [element, question, value];
    _callbacks++;
    return question.default_value;
};

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
    expect(_callbacks).toBe(1);
    // args
    expect(_args.length).toBe(3);

    // first argument is the question
    expect(_args[1]).toEqual(expect.objectContaining(question));
    // second argument is the defaultValue as value
    expect(_args[2]).toBe("teststring");
});

it('form element renders default value', () => {
    const instance = component.root;
    const input = instance.find(node => node.type === 'input');
    expect(input.props.value).toEqual('teststring');
});
