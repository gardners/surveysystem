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
import RadioMatrix from './form/RadioMatrix';

import { serializeAnswer, mapTypeToField } from '../payload-serializer';

const Pre = function(props) {

    let cls = null;
    let { data } = props;

    if(props.data instanceof Error) {
        cls = 'text-danger';
        data = data.toString();
    }

    return(
        <pre style={ { marginBottom: 0 } }className={ cls }>{ (typeof data === 'string') ? data : JSON.stringify(data) }</pre>
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
                title: props.title || 'title',
                title_text: props.title_text || 'title text',
                type: props.type,

                defaultValue: props.defaultValue || 'default',
                choices: props.choices || [],
            },
            values: {},
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

        const updated = this.state.values;
        updated[question.id] = serializeAnswer(question.id, answer);

        this.setState({
            values: updated,
        });
    }

    render() {
        const { question, values } = this.state;
        const Component = this.props.component;

        return (
            <FormRow className="list-group-item mb-1" legend={ question.name }>
                { Object.keys(values).map(key => <Pre key={ values[key].id } data={ values[key] } />) }
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
                <Question type={ 'FIXEDPOINT' } component={ RadioMatrix }
                    questionGroup={[
                        {
                            id: 'question1',
                            name: 'question1',
                            type: 'FIXEDPOINT',
                            title: 'Question 1',
                            title_text: 'Question 1 text',
                            choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                        }, {
                            id: 'question2',
                            name: 'question2',
                            type: 'FIXEDPOINT',
                            title: 'Question 2',
                            title_text: 'Question 2 text',
                            choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                        },{
                            id: 'question3',
                            name: 'question3',
                            type: 'FIXEDPOINT',
                            title: 'Question 3',
                            title_text: 'Question 3 text',
                            choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                        },{
                            id: 'question4',
                            name: 'question4',
                            type: 'FIXEDPOINT',
                            title: 'Question 4',
                            title_text: 'Question 4 text',
                            choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                        }
                    ]}
                />
            </section>
        );
    }
}

Demo.propTypes = {};

export default Demo;
