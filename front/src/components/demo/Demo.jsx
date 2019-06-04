import React, { Component } from 'react';
import PropTypes from 'prop-types';

// form elements
import GeoLocation from '../form/GeoLocation';
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

import { serializeQuestionAnswer } from '../../serializer';
import { normalizeQuestion, normalizeQuestions } from '../../Question';
import Question from '../survey/Question';

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
        const { unit, default_value } = props;
        this.state = {
            unit,
            default_value,
            values: {},
        }
    }

    componentDidMount() {
        const { unit, default_value } = this.props;
        this.setState({
            unit,
            default_value,
        });
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
        const { values, unit, default_value } = this.state;

        const hasAnswers = Object.keys(values).length > 0;

        const Component = props.component;

        const question =  normalizeQuestion({
            id: Component.name,
            name: Component.name,
            title: Component.name,
            description: props.description || 'question description',
            type: props.type,

            default_value,
            choices: props.choices || [],
            unit,
        });

        return (
            <div className="card mb-1" >
                <Question
                    { ...props }
                    question={ question }
                    handleChange={ this.handleChange.bind(this) }
                    className="card-body"
                />

                <div className="card-footer text-muted">
                    { hasAnswers && <small className="mr-2 float-left text-info" style={ { fontFamily: 'monospace' } }>{ JSON.stringify(Object.values(values)) }</small> }
                    <small className="mr-2 float-left">Type: { this.props.type }</small>
                    <small className="mr-2 float-right"><input type="checkbox" onChange={(e) => this.setState({ unit: (e.target.checked) ? 'example unit': '' })} /> toggle unit</small>
                </div>
            </div>
        );
    }
};

Row.defaultProps = {
    unit: '',
    default_value: '',
};

Row.propTypes = {
    component: PropTypes.func.isRequired,
};

const Demo = function(props){

    return (
        <section>
            <div>
                <Row type={ 'HIDDEN' }       component={ HiddenInput }       description="text with some <strong>markup</strong> html and an image: <img src='data:image/gif;base64,R0lGODlhEAAQAMQAAORHHOVSKudfOulrSOp3WOyDZu6QdvCchPGolfO0o/XBs/fNwfjZ0frl3/zy7////wAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACH5BAkAABAALAAAAAAQABAAAAVVICSOZGlCQAosJ6mu7fiyZeKqNKToQGDsM8hBADgUXoGAiqhSvp5QAnQKGIgUhwFUYLCVDFCrKUE1lBavAViFIDlTImbKC5Gm2hB0SlBCBMQiB0UjIQA7'>" />
                <Row type={ 'LATLON' }       component={ GeoLocation }       withButton={ true } />
                <Row type={ 'TIMERANGE' }    component={ PeriodRangeSlider } />
                <Row type={ 'DAYTIME' }      component={ DayTimeSlider }     />
                <Row type={ 'FIXEDPOINT' }   component={ TimePicker }        />
                <Row type={ 'MULTICHOICE' }  component={ CheckboxGroup }     choices={ ['This', 'That', 'Another one' ] } default_value="That" />
                <Row type={ 'SINGLECHOICE' } component={ RadioGroup }        choices={ ['This', 'That', 'Another one' ] } />
                <Row type={ 'CHECKBOX' }     component={ Checkbox }          choices={ [ 'Unchecked!', 'Checked!'] }  default_value="Checked!" />
                <Row type={ 'TEXT' }         component={ RadioGroup }        choices={ ['Yes', 'No', 'Maybe' ] }  />
                <Row type={ 'SINGLESELECT' } component={ Select }            choices={ ['First', 'Second', 'Third' ] } />
                <Row type={ 'MULTISELECT' }  component={ MultiSelect }       choices={ ['First', 'Second', 'Third' ] } />
                <Row type={ 'TEXT' }         component={ TextInput }         />
                <Row type={ 'INT' }          component={ NumberInput }       />
                <Row type={ 'TEXT' }         component={ Textarea }          />
                <Row type={ 'FIXEDPOINT' }   component={ RadioMatrix }       description="This is the <em>description</em> for this question group"
                    questions={ normalizeQuestions([
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
                        },{
                            id: 'question3',
                            name: 'question3',
                            type: 'FIXEDPOINT',
                            title: 'Row 3',
                            description: 'Row 3 text',
                            choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                        },{
                            id: 'question4',
                            name: 'question4',
                            type: 'FIXEDPOINT',
                            title: 'Row 4',
                            description: 'Row 4 text',
                            choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                        }
                    ]) }
                />
                <Row type={ 'EMAIL' }        component={ EmailInput }        />
                <Row type={ 'PASSWORD' }     component={ PasswordInput }     />
                <Row type={ 'DAYTIME' }      component={ DaytimeSequence }
                    questions={ normalizeQuestions([
                        {
                            id: 'question1',
                            name: 'question1',
                            type: 'DAYTIME',
                            title: 'breakfast time',
                            description: 'Row 1 text',
                            default_value: 27000, /* 07:30:00 */
                        }, {
                            id: 'question2',
                            name: 'question2',
                            type: 'DAYTIME',
                            title: 'lunch time',
                            description: 'Row 2 text',
                            default_value: 43200, /* 12:00:00 */
                        },{
                            id: 'question3',
                            name: 'question3',
                            type: 'DAYTIME',
                            title: 'afternoon tea time',
                            description: 'Row 3 text',
                            default_value: 55800, /* 15:30:00 */
                        },{
                            id: 'question4',
                            name: 'question4',
                            type: 'DAYTIME',
                            title: 'late snack time',
                            description: 'Row 4 text',
                            default_value: 73800, /* 20:30:00 */
                        }
                    ]) }
                />
            </div>
        </section>
    );
};


Demo.propTypes = {};

export default Demo;
