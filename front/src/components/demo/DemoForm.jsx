import React, { Component } from 'react';
import PropTypes from 'prop-types';

import { Link } from 'react-router-dom';
import Grid from '@material-ui/core/Grid';
import Box from '@material-ui/core/Box';
import Button from '@material-ui/core/Button';

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
import Question from '../survey/Question';

import Content from '../Content';
import Section from '../Section';


const styles = {
    grid: {
        fontSize: '.75rem',
        fontFamily: 'monospace',
    }
};

const Pre = function(props) {
    let { data } = props;

    if(props.data instanceof Error) {
        data = data.toString();
    }

    return(
        <React.Fragment>{ (typeof data === 'string') ? data : JSON.stringify(data) }</React.Fragment>
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
        const { selected, component } = this.props;

        if(selected && selected !== component.name) {
            return null;
        }

        const { title, description, type, defaultValue, choices } = this.props;
        const { values, unit } = this.state;

        const answers = (Object.keys(values).length > 0) ? Object.values(values) : null;
        const Component = component;

        const question =  {
            id: Component.name,
            name: Component.name,
            title: title || 'title',
            description: description || 'question description',
            type,

            defaultValue: defaultValue || 'default',
            choices: choices || [],
            unit,
        };

        return (
            <Section>
                <Question
                    { ...this.props }
                    question={ question }
                    handleChange={ this.handleChange.bind(this) }
                    className="card-body"
                />
                <Box bgcolor="text.hint" p={ 1 } mt={ 2 } style={ styles.grid }>
                    Answer: <Box display="inline" color="primary.main" component="pre">{ JSON.stringify(answers) }</Box> |
                    Type: { this.props.type }, Component: { component.name } |
                    <Button size="small" onClick={ e =>this.setState({ unit: (!this.state.unit) ? 'example unit': '' }) }> { (!this.state.unit) ? 'show unit' : 'hide unit' }</Button> |
                    { <Button component={ Link } size="small" to={ (!selected) ? `/demo/form/${Component.name}`: '/demo/form' }>{ (!selected) ? 'hide other': 'show all' }</Button> }
                </Box>
            </Section>
        );
    }
};

Row.propTypes = {
    component: PropTypes.func.isRequired,
    selected: PropTypes.string.isRequired,
};

const test = function(name) {
    const t = {};
    t[name] = name;
}

class Demo extends Component {
    constructor(props) {
        super(props);


        this.state = {
            unit: '',
        };
    }

    render() {
        const { params } = this.props.match;
        const selected = params.component || '';

        return (
            <Content title="Demo Form Elements">
                <Box component="h2">{ selected || 'All components' }{ selected && <Button color="primary" component={ Link } size="small" to={ '/demo/form' }>Show All</Button>}</Box>
                <Row type={ 'HIDDEN' }       selected={ selected } component={ HiddenInput }       unit={ this.state.unit } description="text with some <strong>markup</strong> html and an image: <img src='data:image/gif;base64,R0lGODlhEAAQAMQAAORHHOVSKudfOulrSOp3WOyDZu6QdvCchPGolfO0o/XBs/fNwfjZ0frl3/zy7////wAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACH5BAkAABAALAAAAAAQABAAAAVVICSOZGlCQAosJ6mu7fiyZeKqNKToQGDsM8hBADgUXoGAiqhSvp5QAnQKGIgUhwFUYLCVDFCrKUE1lBavAViFIDlTImbKC5Gm2hB0SlBCBMQiB0UjIQA7'>" defaultVlaue="visited"/>
                <Row type={ 'LATLON' }       selected={ selected } component={ GeoLocation }       unit={ this.state.unit } withButton={ true } />
                <Row type={ 'TIMERANGE' }    selected={ selected } component={ PeriodRangeSlider } unit={ this.state.unit }/>
                <Row type={ 'DAYTIME' }      selected={ selected } component={ DayTimeSlider }     unit={ this.state.unit }/>
                <Row type={ 'FIXEDPOINT' }   selected={ selected } component={ TimePicker }        unit={ this.state.unit }/>
                <Row type={ 'MULTICHOICE' }  selected={ selected } component={ CheckboxGroup }     unit={ this.state.unit } choices={ ['This', 'That', 'Another one' ] } defaultValue="Maybe"/>
                <Row type={ 'SINGLECHOICE' } selected={ selected } component={ RadioGroup }        unit={ this.state.unit } choices={ ['This', 'That', 'Another one' ] } defaultValue="Maybe"/>
                <Row type={ 'CHECKBOX' }     selected={ selected } component={ Checkbox }          unit={ this.state.unit } choices={ [ 'Unchecked!', 'Checked!'] }/>
                <Row type={ 'TEXT' }         selected={ selected } component={ RadioGroup }        unit={ this.state.unit } choices={ ['Yes', 'No', 'Maybe' ] } defaultValue="Maybe"/>
                <Row type={ 'SINGLESELECT' } selected={ selected } component={ Select }            unit={ this.state.unit } choices={ ['First', 'Second', 'Third' ] } defaultValue="Second"/>
                <Row type={ 'MULTISELECT' }  selected={ selected } component={ MultiSelect }       unit={ this.state.unit } choices={ ['First', 'Second', 'Third' ] } defaultValue="Second"/>
                <Row type={ 'TEXT' }         selected={ selected } component={ TextInput }         unit={ this.state.unit }/>
                <Row type={ 'INT' }          selected={ selected } component={ NumberInput }       unit={ this.state.unit }/>
                <Row type={ 'TEXT' }         selected={ selected } component={ Textarea }          unit={ this.state.unit }/>
                <Row type={ 'FIXEDPOINT' }   selected={ selected } component={ RadioMatrix }       unit={ this.state.unit } description="This is the <em>description</em> for this question group"
                    questions={ [
                        {
                            id: 'question1',
                            name: 'question1',
                            type: 'FIXEDPOINT',
                            title: 'Row 1',
                            description: 'Row 1 text',
                            choices: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 ],
                            unit: this.state.unit,
                        }, {
                            id: 'question2',
                            name: 'question2',
                            type: 'FIXEDPOINT',
                            title: 'Row 2',
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
                <Row type={ 'EMAIL' }        selected={ selected } component={ EmailInput }           unit={ this.state.unit }/>
                <Row type={ 'PASSWORD' }     selected={ selected } component={ PasswordInput }        unit={ this.state.unit }/>
                <Row type={ 'DAYTIME' }      selected={ selected } component={ DaytimeSequence }      unit={ this.state.unit }
                    questions={ [
                        {
                            id: 'question1',
                            name: 'question1',
                            type: 'DAYTIME',
                            title: 'breakfast time',
                            description: 'Row 1 text',
                            unit: 'seconds',
                        }, {
                            id: 'question2',
                            name: 'question2',
                            type: 'DAYTIME',
                            title: 'lunch time',
                            description: 'Row 2 text',
                            unit: 'seconds',
                        },{
                            id: 'question3',
                            name: 'question3',
                            type: 'DAYTIME',
                            title: 'afternoon tea time',
                            description: 'Row 3 text',
                            unit: 'seconds',
                        },{
                            id: 'question4',
                            name: 'question4',
                            type: 'DAYTIME',
                            title: 'late snack time',
                            description: 'Row 4 text',
                            unit: 'seconds',
                        }
                    ] }
                />
            </Content>
        );
    }
}

Demo.propTypes = {};

export default Demo;
