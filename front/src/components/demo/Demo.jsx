import React, { Component } from 'react';
import PropTypes from 'prop-types';

import { Link } from 'react-router-dom';

// form elements
import DeviceLocation from '../form/DeviceLocation';
import PeriodRangeSlider from '../form/PeriodRangeSlider';
import DayTimeSlider from '../form/DayTimeSlider';
import CheckboxGroup from '../form/CheckboxGroup';
import Checkbox from '../form/Checkbox';
import RadioGroup from '../form/RadioGroup';
import TextInput from '../form/TextInput';
import NumberInput from '../form/NumberInput';
import Textarea from '../form/Textarea';
import Select from '../form/Select';
import MultiSelect from '../form/MultiSelect';
import TimePicker from '../form/TimePicker';
import RadioMatrix from '../form/RadioMatrix';
import HiddenInput from '../form/HiddenInput';
import EmailInput from '../form/EmailInput';
import PasswordInput from '../form/PasswordInput';
import DaytimeSequence from '../form/DaytimeSequence';
import DaytimeQuestionGroup from '../form/DaytimeQuestionGroup';

import AnswerModel from '../../Answer';
import QuestionModel from '../../Question';
import Question from '../survey/Question';
import QuestionGroup from '../survey/QuestionGroup';

const getDefaultAnswers = function(coercedQuestions) {
    const answers = {};
    coercedQuestions.forEach(question => {
        if (question.default_value) {
            const answerObj = AnswerModel.create(question);
            const serialized = AnswerModel.serialize(answerObj);
            if(serialized instanceof Error) {
                return;
            }
            answers[question.id] = serialized;
        }
    });
    return answers;
};

/**
 * select component
 */

const components = [
    DeviceLocation,
    PeriodRangeSlider,
    DayTimeSlider,
    CheckboxGroup,
    Checkbox,
    RadioGroup,
    TextInput,
    NumberInput,
    Textarea,
    Select,
    MultiSelect,
    TimePicker,
    RadioMatrix,
    HiddenInput,
    EmailInput,
    PasswordInput,
    DaytimeSequence,
    DaytimeQuestionGroup,
];

const SelectComponent = function({ history, selected }) {
    return (
        <select value={ selected } onChange={ e => history.push(`/demo/form/${e.target.value}`) }>
            <option value="">All components..</option>
            { components.map(component => <option key={ component.name } value={ component.name }>{ component.name }</option>) }
        </select>
    );
};

SelectComponent.propTypes = {
    history: PropTypes.object.isRequired,
    selected: PropTypes.string.isRequired,
};

/**
 * Display serialized answer
 */

const Answers = function({ questions, answers, className }) {
    if (!Object.keys(answers).length) {
        return (<React.Fragment>Answer: <i className={ className }>none</i></React.Fragment>);
    }
    return (
        <React.Fragment>
            {
                Object.keys(answers).map((id) => <div key={ id }>Answer: <span className={ className }>{ answers[id] }</span></div>)
            }
        </React.Fragment>
    );
};

Answers.defaultProps = {
    answers: {},
};

Answers.propTypes = {
    answers: PropTypes.object,
    questions: PropTypes.arrayOf(
        QuestionModel.propTypes(),
    ).isRequired,

    className: PropTypes.string,
};

/**
 * Question Component
 */

class Row extends Component {
    constructor(props) {
        super(props);

        this.mockError = new Error('This is a validation error!');

        this.state = {
            questions: [],
            answers: {},
            errors: {},
        }
    }

    componentDidMount() {
        const { props } = this;
        const Component = props.component;

        const id = Component.name;
        const title = 'Question title for ' + id

        let questions;

        if(props.questions.length) {
            questions = props.questions.map(q => QuestionModel.normalize(q));
        } else{
            questions = [
                QuestionModel.normalize({
                    id,
                    name: id,
                    title,
                    description: props.description || `This is the question description for question <span style="color: green">${id}</span>, it can contain HTML.`,
                    type: props.type,

                    default_value: props.default_value,
                    choices: props.choices || [],
                    unit: props.unit,
                })
            ];
        }

        const answers = getDefaultAnswers(questions);

        this.setState({
            questions,
            answers,
            errors: {},
        });
    }

    handleChange(element, question, value) {
        const { answers, errors } = this.state;
        const { id } = question;

        // @see https://developer.mozilla.org/en-US/docs/Web/API/ValidityState
        if(element && typeof element.validity !== 'undefined') {
            if (!element.validity.valid) {
                errors[id] = new Error (element.validationMessage);
                this.setState({
                    errors,
                });
                return;
            }
        }

        const answer = AnswerModel.setValue(question, value);
        if (answer instanceof Error) {
            errors[id] = answer;
            this.setState({
                errors,
            });
            return;
        }

        const serialized = AnswerModel.serialize(answer);// can be instanceof Error
        if (serialized instanceof Error) {
            errors[id] = answer;
            this.setState({
                errors,
            });
            return;
        }

        errors[id] = null;
        answers[id] = serialized;

        this.setState({
            errors,
            answers,
        });
    }

    render() {
        const { props } = this;
        const { questions, answers, errors } = this.state;

        const Component = props.component;

        if (!questions.length) {
            return (null);
        }

        if (props.selected && props.selected !== Component.name) {
            return (null);
        }

        return (
            <div className="list-group mb-3">
                {
                    (questions.length > 1) ?
                        <QuestionGroup
                            { ...props }
                            handleChange={ this.handleChange.bind(this) }
                            questions={ questions }
                            errors={ errors }
                            required={ false}
                            className="list-group-item"
                        />
                        :
                        <Question
                            { ...props }
                            handleChange={ this.handleChange.bind(this) }
                            question={ questions[0] }
                            error={ errors[questions[0].id] || null }
                            required={ false}
                            grouped={ false }
                            className="list-group-item"
                        />
                }

                <div className="list-group-item bg-light container text-monospace" style={ { fontSize: '0.8rem' } }>
                    <div className="row">
                        <div className="col-md"><Answers className="text-info" questions={ questions } answers={ answers } /></div>
                    </div>
                </div>

                <div className="list-group-item bg-light container text-monospace" style={ { fontSize: '0.8rem' } }>
                    <div className="row">
                        <div className="col-md">
                            Type: { questions.map(q => <span key={ q.id } className="text-info mr-2">{ q.type }</span>) }<br />
                            Default: { questions.map(q => <span key={ q.id } className="text-info mr-2">{ q.default_value }</span>) }
                        </div>

                        <div className="col-md">
                            <input type="checkbox" onChange={
                                (e) => {
                                    const { checked } = e.target;
                                    if (errors.length && errors[0] !== this.mockError) { // validation error
                                        return;
                                    }

                                    const newErrors = {};
                                    if (checked) {
                                        questions.forEach(q => {
                                            newErrors[q.id] = this.mockError;
                                        });
                                    }

                                    this.setState({
                                        errors: newErrors,
                                    });
                                }
                            } /> mock error
                            <br />
                            <input type="checkbox" onChange={
                                (e) => {
                                    questions.map(q => q.unit = (e.target.checked) ? 'units': '');
                                    this.setState({
                                        questions,
                                    });
                                }
                            } /> mock unit
                        </div>
                        <div className="col-md text-right">
                            {
                                (props.selected !== Component.name) ?
                                    <Link to={ `/demo/form/${Component.name}` }><i className="fas fa-compress-arrows-alt"></i> hide others</Link>
                                    :
                                    <Link to="/demo/form/"><i className="fas fa-expand-arrows-alt"></i> show all</Link>
                            }
                        </div>
                    </div>
                </div>

            </div>
        );
    }
};

Row.defaultProps = {
    unit: '',
    default_value: '',
    questions: [],
};

Row.propTypes = {
    selected: PropTypes.string.isRequired, //component.name
    type: PropTypes.string.isRequired,
    component: PropTypes.func.isRequired,
};

/**
 * Demo
 */

const Demo = function(props){
    const selected = props.match.params.component || '';

    return (
        <section>

            <div className="card bg-secondary text-white sticky-top mb-1">
                <div className="card-body d-flex flex-row-reverse bd-highlight">
                    <SelectComponent history={ props.history } selected={ selected } />
                </div>
            </div>

            <div>
                <Row selected={ selected } type={ 'HIDDEN' }       component={ HiddenInput }       description="text with some <strong>markup</strong> html and an image: <img src='data:image/gif;base64,R0lGODlhEAAQAMQAAORHHOVSKudfOulrSOp3WOyDZu6QdvCchPGolfO0o/XBs/fNwfjZ0frl3/zy7////wAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACH5BAkAABAALAAAAAAQABAAAAVVICSOZGlCQAosJ6mu7fiyZeKqNKToQGDsM8hBADgUXoGAiqhSvp5QAnQKGIgUhwFUYLCVDFCrKUE1lBavAViFIDlTImbKC5Gm2hB0SlBCBMQiB0UjIQA7'>" />
                <Row selected={ selected } type={ 'LATLON' }       component={ DeviceLocation }       withButton={ true } />
                <Row selected={ selected } type={ 'TIMERANGE' }    component={ PeriodRangeSlider } default_value="35145,53145" />
                <Row selected={ selected } type={ 'DAYTIME' }      component={ DayTimeSlider }     default_value="35145" />
                <Row selected={ selected } type={ 'DAYTIME' }      component={ TimePicker }        default_value="35145" />
                <Row selected={ selected } type={ 'MULTICHOICE' }  component={ CheckboxGroup }     choices={ ['This', 'That', 'Another one' ] } default_value="That" />
                <Row selected={ selected } type={ 'SINGLECHOICE' } component={ RadioGroup }        choices={ ['This', 'That', 'Another one' ] } default_value="Another one" />
                <Row selected={ selected } type={ 'CHECKBOX' }     component={ Checkbox }          choices={ [ 'Unchecked!', 'Checked!'] }  default_value="Checked!" />
                <Row selected={ selected } type={ 'TEXT' }         component={ RadioGroup }        choices={ ['Yes', 'No', 'Maybe' ] }  />
                <Row selected={ selected } type={ 'SINGLESELECT' } component={ Select }            choices={ ['First', 'Second', 'Third' ] } />
                <Row selected={ selected } type={ 'MULTISELECT' }  component={ MultiSelect }       choices={ ['First', 'Second', 'Third' ] } />
                <Row selected={ selected } type={ 'TEXT' }         component={ TextInput }         />
                <Row selected={ selected } type={ 'INT' }          component={ NumberInput }       />
                <Row selected={ selected } type={ 'TEXT' }         component={ Textarea }          />
                <Row selected={ selected } type={ 'FIXEDPOINT' }   component={ RadioMatrix }       description="This is the <em>description</em> for this question group"
                    questions={ [
                        {
                            id: 'question1',
                            name: 'question1',
                            type: 'FIXEDPOINT',
                            title: 'Row 1',
                            description: 'Row 1 text',
                            choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                        }, {
                            id: 'question2',
                            name: 'question2',
                            type: 'FIXEDPOINT',
                            title: 'Row 2',
                            description: 'Row 2 text',
                            choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                            default_value: '4',
                        },{
                            id: 'question3',
                            name: 'question3',
                            type: 'FIXEDPOINT',
                            title: 'Row 3',
                            description: 'Row 3 text',
                            choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                            default_value: '4',
                        },{
                            id: 'question4',
                            name: 'question4',
                            type: 'FIXEDPOINT',
                            title: 'Row 4',
                            description: 'Row 4 text',
                            choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                        }
                    ] }
                />
                <Row selected={ selected } type={ 'EMAIL' }             component={ EmailInput }        />
                <Row selected={ selected } type={ 'PASSWORD' }          component={ PasswordInput }     />
                <Row selected={ selected } type={ 'DAYTIME_SEQUENCE' }   component={ DaytimeSequence }
                    choices={ ['Breakfast', 'Lunch', 'Afternoon Tea' , ' Late Snack'] }
                    default_value={ [
                        '27000', /* 07:30:00 */
                        '43200', /* 12:00:00 */
                        '55800', /* 15:30:00 */
                        '73800', /* 20:30:00 */
                    ].join(',') }
                />
                <Row selected={ selected } type={ 'DAYTIME' }      component={ DaytimeQuestionGroup }
                    questions={ [
                        {
                            id: 'question1',
                            name: 'question1',
                            type: 'DAYTIME',
                            title: 'breakfast time',
                            description: 'This is the <em>description</em> for this <span class="text-primary">DaytimeQuestionGroup</span><br />. It displays like the <em>DaytimeSequence</em> component, except it handles a group of questions of type <span class="text-primary">DAYTIME</span>',
                            default_value: '27000', /* 07:30:00 */
                        }, {
                            id: 'question2',
                            name: 'question2',
                            type: 'DAYTIME',
                            title: 'lunch time',
                            description: 'Row 2 text',
                            default_value: '43200', /* 12:00:00 */
                        },{
                            id: 'question3',
                            name: 'question3',
                            type: 'DAYTIME',
                            title: 'afternoon tea time',
                            description: 'Row 3 text',
                            default_value: '55800', /* 15:30:00 */
                        },{
                            id: 'question4',
                            name: 'question4',
                            type: 'DAYTIME',
                            title: 'late snack time',
                            description: 'Row 4 text',
                            default_value: '73800', /* 20:30:00 */
                        }
                    ] }
                />
            </div>
        </section>
    );
};

Demo.propTypes = {};

export default Demo;
