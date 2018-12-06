import React, { Component } from 'react';
import PropTypes from 'prop-types';

import { FormRow } from './FormHelpers';

// form elements
import GeoLocation from './form/GeoLocation';
import PeriodRange from './form/PeriodRange';
import RadioGroup from './form/RadioGroup';
import TextInput from './form/TextInput';
import Select from './form/Select';

import { serializeAnswer } from '../payload-serializer';

const Answer = function(props) {

    let cls = null;
    let { answer } = props;

    if(props.answer instanceof Error) {
        cls = 'text-danger';
        answer = answer.toString();
    }

    return(
        <pre className={ cls }>{ (typeof answer === 'string') ? answer : JSON.stringify(answer) }</pre>
    );
};

Answer.propTypes = {
    answer: PropTypes.any
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

    handleChange(answer, question) {
        this.setState({
            value: serializeAnswer(question.id, answer),
        });
    }

    render() {
        const { value, question } = this.state;
        const Component = this.props.component;
        return (
            <FormRow className="list-group-item" legend={ question.name }>
                { this.state.value && <Answer answer={ this.state.value } /> }
                <Component
                    { ...this.props } question={ question } handleChange={ this.handleChange.bind(this) }
                />
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
                <Question type={ 'TIMERANGE' } component={ PeriodRange } />
                <Question type={ 'TEXT' } component={ RadioGroup } choices={ ['Yes', 'No', 'Maybe' ] } defaultValue="Maybe"/>
                <Question type={ 'TEXT' } component={ Select } choices={ ['First', 'Second', 'Third' ] } defaultValue="Second"/>
                <Question type={ 'TEXT' } component={ TextInput }/>
            </section>
        );
    }
}

Demo.propTypes = {};

export default Demo;
