import React, { Component } from 'react';
import PropTypes from 'prop-types';

// form elements
import GeoLocation from './form/GeoLocation';
import PeriodRangeSlider from './form/PeriodRangeSlider';
import DayTimeSlider from './form/DayTimeSlider';
import CheckboxGroup from './form/CheckboxGroup';
import Checkbox from './form/Checkbox';
import RadioGroup from './form/RadioGroup';
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

import Question from './survey/Question';

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

class Row extends Component {
    constructor(props) {
        super(props);
        this.state = {
            unit: '',
            appearance: 'default',
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
        const { values, unit, appearance } = this.state;

        const hasAnswers = Object.keys(values).length > 0;

        const Component = props.component;

        const question =  {
            id: Component.name,
            name: Component.name,
            title: props.title || 'title',
            description: props.description || 'title text',
            type: props.type,

            defaultValue: props.defaultValue || 'default',
            choices: props.choices || [],
            unit,
        };

        return (
            <div className="card mb-1" >
                <Question
                    { ...props }
                    appearance={ appearance }
                    question={ question }
                    handleChange={ this.handleChange.bind(this) }
                    className="card-body"
                />

                <div className="card-footer text-muted">
                    { hasAnswers && <small className="mr-2 float-left text-info" style={ { fontFamily: 'monospace' } }>{ JSON.stringify(Object.values(values)) }</small> }
                    <small className="mr-2 float-left">Type: { this.props.type }</small>
                    <small className="mr-2 float-right"><input type="checkbox" onChange={(e) => this.setState({ appearance: (e.target.checked) ? 'horizontal' : 'default' })} /> horizontal display</small>
                    <small className="mr-2 float-right"><input type="checkbox" onChange={(e) => this.setState({ unit: (e.target.checked) ? 'example unit': '' })} /> toggle unit</small>
                </div>
            </div>
        );
    }
};

Row.propTypes = {
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
                    <Row type={ 'HIDDEN' }       component={ HiddenInput }       unit={ this.state.unit } description="text with some <strong>markup</strong> html and an image: <img src='data:image/gif;base64,R0lGODlhEAAQAMQAAORHHOVSKudfOulrSOp3WOyDZu6QdvCchPGolfO0o/XBs/fNwfjZ0frl3/zy7////wAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACH5BAkAABAALAAAAAAQABAAAAVVICSOZGlCQAosJ6mu7fiyZeKqNKToQGDsM8hBADgUXoGAiqhSvp5QAnQKGIgUhwFUYLCVDFCrKUE1lBavAViFIDlTImbKC5Gm2hB0SlBCBMQiB0UjIQA7'>" defaultVlaue="visited"/>
                    <Row type={ 'LATLON' }       component={ GeoLocation }       unit={ this.state.unit } withButton={ true } />
                    <Row type={ 'TIMERANGE' }    component={ PeriodRangeSlider } unit={ this.state.unit }/>
                    <Row type={ 'FIXEDPOINT' }   component={ DayTimeSlider }     unit={ this.state.unit }/>
                    <Row type={ 'FIXEDPOINT' }   component={ TimePicker }        unit={ this.state.unit }/>
                    <Row type={ 'MULTICHOICE' }  component={ CheckboxGroup }     unit={ this.state.unit } choices={ ['This', 'That', 'Another one' ] } defaultValue="Maybe"/>
                    <Row type={ 'SINGLECHOICE' } component={ RadioGroup }        unit={ this.state.unit } choices={ ['This', 'That', 'Another one' ] } defaultValue="Maybe"/>
                    <Row type={ 'CHECKBOX' }     component={ Checkbox }          unit={ this.state.unit } choices={ [ 'Unchecked!', 'Checked!'] }/>
                    <Row type={ 'TEXT' }         component={ RadioGroup }        unit={ this.state.unit } choices={ ['Yes', 'No', 'Maybe' ] } defaultValue="Maybe"/>
                    <Row type={ 'SINGLESELECT' } component={ Select }            unit={ this.state.unit } choices={ ['First', 'Second', 'Third' ] } defaultValue="Second"/>
                    <Row type={ 'MULTISELECT' }  component={ MultiSelect }       unit={ this.state.unit } choices={ ['First', 'Second', 'Third' ] } defaultValue="Second"/>
                    <Row type={ 'TEXT' }         component={ TextInput }         unit={ this.state.unit }/>
                    <Row type={ 'INT' }          component={ NumberInput }       unit={ this.state.unit }/>
                    <Row type={ 'TEXT' }         component={ Textarea }          unit={ this.state.unit }/>
                    <Row type={ 'FIXEDPOINT' }   component={ RadioMatrix }       unit={ this.state.unit }
                        questions={ [
                            {
                                id: 'question1',
                                name: 'question1',
                                type: 'FIXEDPOINT',
                                title: '1',
                                description: 'Row 1 text',
                                choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                                unit: this.state.unit,
                            }, {
                                id: 'question2',
                                name: 'question2',
                                type: 'FIXEDPOINT',
                                title: 'Rowfghhd 2',
                                description: 'Row 2 text',
                                choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                                unit: this.state.unit,
                            },{
                                id: 'question3',
                                name: 'question3',
                                type: 'FIXEDPOINT',
                                title: 'Row 3',
                                description: 'Row 3 text',
                                choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                                unit: this.state.unit,
                            },{
                                id: 'question4',
                                name: 'question4',
                                type: 'FIXEDPOINT',
                                title: 'Row 4',
                                description: 'Row 4 text',
                                choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                                unit: this.state.unit,
                            }
                        ] }
                    />
                    <Row type={ 'EMAIL' }        component={ EmailInput }           unit={ this.state.unit }/>
                    <Row type={ 'PASSWORD' }     component={ PasswordInput }        unit={ this.state.unit }/>
                </div>
            </section>
        );
    }
}

Demo.propTypes = {};

export default Demo;
