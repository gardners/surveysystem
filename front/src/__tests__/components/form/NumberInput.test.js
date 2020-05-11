import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import NumberInput from '../../../components/form/NumberInput';

let component;

describe( 'default min/max', () => {
    beforeAll(() => {
        component = renderer.create(
            <NumberInput
                question={ {
                    id: 'q1',
                    name: 'q1',
                    title: 'q1',
                    description: 'q1',
                    type: 'INT',
                    unit: '',
                    default_value: '99',
                } }
                handleChange= { () => {} } />,
        );
    });

    it('renders without crashing', () => {
        const tree = component.toJSON();
        expect(tree).toMatchSnapshot();
    });


    it('form element renders default value', () => {
        const instance = component.root;
        const input = instance.find(node => node.type === 'input');
        expect(input.props.value).toEqual(99);

        expect(input.props.min).toEqual(0);
        expect(input.props.max).toEqual(Infinity);
    });
});

describe( 'custom min/max', () => {
    beforeAll(() => {
        component = renderer.create(
            <NumberInput
                question={ {
                    id: 'q1',
                    name: 'q1',
                    title: 'q1',
                    description: 'q1',
                    type: 'INT',
                    default_value: '',
                    min_value: -1,
                    max_value: 99,
                    unit: '',
                } }
                handleChange= { () => {} } />,
        );
    });

    it('form element renders default value', () => {
        const instance = component.root;
        const input = instance.find(node => node.type === 'input');
        expect(input.props.min).toEqual(-1);
        expect(input.props.max).toEqual(99);
    });
});

describe( 'custom min/max ignore', () => {
    beforeAll(() => {
        component = renderer.create(
            <NumberInput
                question={ {
                    id: 'q1',
                    name: 'q1',
                    title: 'q1',
                    description: 'q1',
                    type: 'INT',
                    default_value: '',
                    min_value: 0,
                    max_value: 0,
                    unit: '',
                } }
                handleChange= { () => {} } />,
        );
    });

    it('form element renders default value', () => {
        const instance = component.root;
        const input = instance.find(node => node.type === 'input');
        expect(input.props.min).toEqual(0);
        expect(input.props.max).toEqual(Infinity);
    });
});

describe( 'custom min/max ignore', () => {
    beforeAll(() => {
        component = renderer.create(
            <NumberInput
                question={ {
                    id: 'q1',
                    name: 'q1',
                    title: 'q1',
                    description: 'q1',
                    type: 'INT',
                    default_value: '',
                    min_value: 99,
                    max_value: 99,
                    unit: '',
                } }
                handleChange= { () => {} } />,
        );
    });

    it('form element renders default value', () => {
        const instance = component.root;
        const input = instance.find(node => node.type === 'input');
        expect(input.props.min).toEqual(0);
        expect(input.props.max).toEqual(Infinity);
    });
});
