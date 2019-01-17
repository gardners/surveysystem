import React, { Component } from 'react';
import PropTypes from 'prop-types';

import { FormRow, FieldError } from './FormHelpers';

// form elements
import GeoLocation from './form/GeoLocation';
import PeriodRangeSlider from './form/PeriodRangeSlider';
import DayTimeSlider from './form/DayTimeSlider';
import CheckboxGroup from './form/CheckboxGroup';
import Checkbox from './form/Checkbox';
import RadioGroup from './form/RadioGroup';
import RadioMatrixRow from './form/RadioMatrixRow';
import TextInput from './form/TextInput';
import NumberInput from './form/NumberInput';
import Textarea from './form/Textarea';
import Select from './form/Select';
import MultiSelect from './form/MultiSelect';
import TimePicker from './form/TimePicker';
import RadioMatrix from './form/RadioMatrix';
import HiddenInput from './form/HiddenInput';
import EmailInput from './form/EmailInput';
import PasswordInput from './form/PasswordInput';

import { serializeQuestionAnswer } from '../serializer';

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
            unit: '',
            values: {},
        }
    }

    handleChange(element, question, ...values) {
        const updated = this.state.values;
        updated[question.id] = serializeQuestionAnswer(element, question, ...values);

        this.setState({
            values: updated,
        });
    }

    render() {
        const { props } = this;
        const { values, unit } = this.state;

        const hasAnswers = Object.keys(values).length > 0;

        const Component = props.component;
        const value = values[Component.name] || '';

        const question =  {
            id: Component.name,
            name: Component.name,
            title: props.title || 'title',
            title_text: props.title_text || 'title text',
            type: props.type,

            defaultValue: props.defaultValue || 'default',
            choices: props.choices || [],
            unit,
        };

        return (
            <div className="card mb-1" >
                <FormRow className="card-body" legend={ question.name } description={ question.title_text }>
                    <Component
                        { ...this.props } question={ question } handleChange={ this.handleChange.bind(this) }
                    />
                    <FieldError error={ (value instanceof Error) ? value : null }/>
                </FormRow>

                <div className="card-footer text-muted">
                    { hasAnswers && <small className="mr-2 float-left text-info" style={ { fontFamily: 'monospace' } }>{ JSON.stringify(Object.values(values)) }</small> }
                    <small className="mr-2 float-left">Type: { this.props.type }</small>
                    <small className="mr-2 float-right"><input type="checkbox" onChange={(e) => this.setState({ unit: (e.target.checked) ? 'example unit': '' })} /> toggle unit</small>
                </div>
            </div>
        );
    }
};

Question.propTypes = {
    component: PropTypes.func.isRequired,
};

class Demo extends Component {
    constructor(props) {
        super(props);
        this.state = {
            unit: '',
        };
    }

    render() {
        return (
            <section>
                <div>
                    <Question type={ 'HIDDEN' }       component={ HiddenInput }       unit={ this.state.unit } title_text="text with some <strong>markup</strong> html and an image: <img src='data:image/gif;base64,R0lGODlhEAAQAMQAAORHHOVSKudfOulrSOp3WOyDZu6QdvCchPGolfO0o/XBs/fNwfjZ0frl3/zy7////wAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACH5BAkAABAALAAAAAAQABAAAAVVICSOZGlCQAosJ6mu7fiyZeKqNKToQGDsM8hBADgUXoGAiqhSvp5QAnQKGIgUhwFUYLCVDFCrKUE1lBavAViFIDlTImbKC5Gm2hB0SlBCBMQiB0UjIQA7'>" defaultVlaue="visited"/>
                    <Question type={ 'LATLON' }       component={ GeoLocation }       unit={ this.state.unit } withButton={ true } />
                    <Question type={ 'TIMERANGE' }    component={ PeriodRangeSlider } unit={ this.state.unit }/>
                    <Question type={ 'FIXEDPOINT' }   component={ DayTimeSlider }     unit={ this.state.unit }/>
                    <Question type={ 'FIXEDPOINT' }   component={ TimePicker }        unit={ this.state.unit }/>
                    <Question type={ 'MULTICHOICE' }  component={ CheckboxGroup }     unit={ this.state.unit } choices={ ['This', 'That', 'Another one' ] } defaultValue="Maybe"/>
                    <Question type={ 'SINGLECHOICE' } component={ RadioGroup }        unit={ this.state.unit } choices={ ['This', 'That', 'Another one' ] } defaultValue="Maybe"/>
                    <Question type={ 'CHECKBOX' }     component={ Checkbox }          unit={ this.state.unit } choices={ [ 'unchecked value', 'checked value'] }/>
                    <Question type={ 'TEXT' }         component={ RadioGroup }        unit={ this.state.unit } choices={ ['Yes', 'No', 'Maybe' ] } defaultValue="Maybe"/>
                    <Question type={ 'SINGLESELECT' } component={ Select }            unit={ this.state.unit } choices={ ['First', 'Second', 'Third' ] } defaultValue="Second"/>
                    <Question type={ 'MULTISELECT' }  component={ MultiSelect }       unit={ this.state.unit } choices={ ['First', 'Second', 'Third' ] } defaultValue="Second"/>
                    <Question type={ 'TEXT' }         component={ TextInput }         unit={ this.state.unit }/>
                    <Question type={ 'INT' }          component={ NumberInput }       unit={ this.state.unit }/>
                    <Question type={ 'TEXT' }         component={ Textarea }          unit={ this.state.unit }/>
                    <Question type={ 'FIXEDPOINT' }   component={ RadioMatrix }       unit={ this.state.unit }
                        questionGroup={[
                            {
                                id: 'question1',
                                name: 'question1',
                                type: 'FIXEDPOINT',
                                title: 'Question 1',
                                title_text: 'Question 1 text',
                                choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                                unit: this.state.unit,
                            }, {
                                id: 'question2',
                                name: 'question2',
                                type: 'FIXEDPOINT',
                                title: 'Question 2',
                                title_text: 'Question 2 text',
                                choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                                unit: this.state.unit,
                            },{
                                id: 'question3',
                                name: 'question3',
                                type: 'FIXEDPOINT',
                                title: 'Question 3',
                                title_text: 'Question 3 text',
                                choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                                unit: this.state.unit,
                            },{
                                id: 'question4',
                                name: 'question4',
                                type: 'FIXEDPOINT',
                                title: 'Question 4',
                                title_text: 'Question 4 text',
                                choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                                unit: this.state.unit,
                            }
                        ]}
                    />
                    <Question type={ 'SINGLECHOICE' } id='Group1__1' component={ RadioMatrixRow } matrixState={ [false, true] } unit={ this.state.unit } choices={ '123456789'.split('') } defaultValue="3"/>
                    <Question type={ 'SINGLECHOICE' } id='Group2__1' component={ RadioMatrixRow } matrixState={ [true, true] } unit={ this.state.unit } choices={ '123456789'.split('') } defaultValue="3"/>
                    <Question type={ 'SINGLECHOICE' } id='Group3__1' component={ RadioMatrixRow } matrixState={ [true, false] } unit={ this.state.unit } choices={ '123456789'.split('') } defaultValue="3"/>

                    <Question type={ 'EMAIL' }        component={ EmailInput }           unit={ this.state.unit }/>
                    <Question type={ 'PASSWORD' }     component={ PasswordInput }        unit={ this.state.unit }/>
                </div>
            </section>
        );
    }
}

Demo.propTypes = {};

export default Demo;
