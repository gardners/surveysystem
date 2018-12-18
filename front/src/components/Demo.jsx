import React, { Component } from 'react';
import PropTypes from 'prop-types';

import { FormRow } from './FormHelpers';

// form elements
import GeoLocation from './form/GeoLocation';
import PeriodRangeSlider from './form/PeriodRangeSlider';
import DayTimeSlider from './form/DayTimeSlider';
import CheckboxGroup from './form/CheckboxGroup';
import RadioGroup from './form/RadioGroup';
import TextInput from './form/TextInput';
import TextArea from './form/TextArea';
import Select from './form/Select';
import TimePicker from './form/TimePicker';

import { serializeAnswer, mapTypeToField } from '../payload-serializer';

const Pre = function(props) {

    let cls = null;
    let { data } = props;

    if(props.data instanceof Error) {
        cls = 'text-danger';
        data = data.toString();
    }

    return(
        <pre className={ cls }>{ (typeof data === 'string') ? data : JSON.stringify(data) }</pre>
    );
};

Pre.propTypes = {
    data: PropTypes.any
};

class Question extends Component {
    constructor(props) {
        super(props);
        this.state = {
            question: {
                id: props.component.name,
                name: props.component.name,
                title: 'title',
                title_text: 'title text',
                type: props.type,

                defaultValue: props.defaultValue || 'default',
                choices: props.choices || [],
            },
            value: '',
        }
    }

    handleChange(question, ...values) {
        const fn = mapTypeToField(question.type);
        let answer;

        if (fn instanceof Error) {
            answer = fn;
        } else {
            answer = fn(...values);
        }

        this.setState({
            value: serializeAnswer(question.id, answer),
        });
    }

    render() {
        const { question } = this.state;
        const Component = this.props.component;
        return (
            <FormRow className="list-group-item mb-1" legend={ question.name }>
            { this.state.value && <Pre data={ this.state.value } /> }
                { this.state.value && <Pre data={ this.state.value } /> }
                <Component
                    { ...this.props } question={ question } handleChange={ this.handleChange.bind(this) }
                />
                <div><span className="badge badge-secondary">question type: { this.props.type }</span></div>
            </FormRow>
        );
    }
};

Question.propTypes = {
    component: PropTypes.func.isRequired,
};

class Demo extends Component {
    render() {
        return (
            <section className="list-group">
                <Question type={ 'LATLON' } component={ GeoLocation } withButton={ true } />
                <Question type={ 'TIMERANGE' } component={ PeriodRangeSlider } />
                <Question type={ 'FIXEDPOINT' } component={ DayTimeSlider } />
                <Question type={ 'FIXEDPOINT' } component={ TimePicker }/>
                <Question type={ 'MULTICHOICE' } component={ CheckboxGroup } choices={ ['This', 'That', 'Another one' ] } defaultValue="Maybe"/>
                <Question type={ 'TEXT' } component={ RadioGroup } choices={ ['Yes', 'No', 'Maybe' ] } defaultValue="Maybe"/>
                <Question type={ 'TEXT' } component={ Select } choices={ ['First', 'Second', 'Third' ] } defaultValue="Second"/>
                <Question type={ 'TEXT' } component={ TextInput }/>
                <Question type={ 'TEXT' } component={ TextArea }/>
            </section>
        );
    }
}

Demo.propTypes = {};

export default Demo;
