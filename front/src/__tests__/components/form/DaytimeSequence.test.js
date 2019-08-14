import React from 'react';
import ReactDOM from 'react-dom';
import renderer from 'react-test-renderer';

import { AppContext } from '../../../Context';

import InputRange from 'react-input-range';
import { DaytimeInput } from '../../../components/form/TimePicker';
import DaytimeSequence from '../../../components/form/DaytimeSequence';

let component;

describe( 'expanded display', () => {

    beforeAll(() => {
        component = renderer.create(
            <AppContext.Provider value={ {
                    isBreakpointAbove: () => true, 
            } }>
                <DaytimeSequence
                    question={ {
                        id: 'q1',
                        name: 'q1',
                        title: 'q1',
                        description: 'q1',
                        type: 'DAYTIME_SEQUENCE',
                        choices: ['Start', 'End'],
                        unit: 'seconds',
                        default_value: '35145,74745' // 09:45:45 am, 08:45:45 pm
                    } }
                    expand={ true }
                    handleChange= { () => true }
                />
            </AppContext.Provider>
        );
    });

    it('renders without crashing', () => {
        const tree = component.toJSON();
        expect(tree).toMatchSnapshot();
    });

    it('mounts with default values in state', () => {
        const instance = component.root;
        const node = instance.findByType(DaytimeSequence);
        // access state
        const { state } = node.instance;
        expect(state.values.toString()).toEqual('35145,74745');
    });

    it('renders exact 1 (the first one) DaytimeInput component as children', () => {
        const instance = component.root;
        const input = instance.findByType(InputRange);
        expect(input.props.value).toEqual(35145);
        expect(input.props.minValue).toEqual(0);
        expect(input.props.maxValue).toEqual(86400); 
    });

});

describe( 'contracted display', () => {

    beforeAll(() => {
        component = renderer.create(
            <AppContext.Provider value={ {
                    isBreakpointAbove: () => false, 
            } }>
                <DaytimeSequence
                    question={ {
                        id: 'q1',
                        name: 'q1',
                        title: 'q1',
                        description: 'q1',
                        type: 'DAYTIME_SEQUENCE',
                        choices: ['Start', 'End'],
                        unit: 'seconds',
                        default_value: '35145,74745' // 09:45:45 am, 08:45:45 pm
                    } }
                    expand={ true }
                    handleChange= { () => true }
                />
            </AppContext.Provider>
        );
    });

    it('renders without crashing', () => {
        const tree = component.toJSON();
        expect(tree).toMatchSnapshot();
    });

    it('mounts with default values in state', () => {
        const instance = component.root;
        const node = instance.findByType(DaytimeSequence);
        // access state
        const { state } = node.instance;
        expect(state.values.toString()).toEqual('35145,74745');
    });

    it('renders exact 1 (the first one) DaytimeInput component as children', () => {
        const instance = component.root;
        const nodes = instance.findAllByType(DaytimeInput);
        expect(nodes.length).toEqual(1);
    
        const inputs = nodes[0].findAll(node => node.type === 'input');
        expect(inputs.length).toEqual(2);
        const selects = instance.findAll(node => node.type === 'select');
        expect(selects.length).toEqual(1);
        
        // hours
        expect(inputs[0].props.min).toEqual(0);
        expect(inputs[0].props.max).toEqual(12);
        expect(inputs[0].props.value).toEqual(9); 
        
        // minutes
        expect(inputs[1].props.min).toEqual(0);
        expect(inputs[1].props.max).toEqual(60);
        expect(inputs[1].props.value).toEqual(45);
        
        // am/pm
        const options = selects[0].findAllByType('option');
        const values = options.map(node => node.props.value);
        expect(values.toString()).toEqual('am,pm');
        expect(options.length).toEqual(2);
        expect(selects[0].props.value).toEqual('am');
    });

});