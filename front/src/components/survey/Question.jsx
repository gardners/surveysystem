import React, { Component } from 'react';
import PropTypes from 'prop-types';

import QuestionModel from '../../Question';

import TextInput from '../form/TextInput';
import RadioGroup from '../form/RadioGroup';
import CheckboxGroup from '../form/CheckboxGroup';
import DayTimeSlider from '../form/DayTimeSlider';
import Checkbox from '../form/Checkbox';
import GeoLocation from '../form/GeoLocation';
import PeriodRangeSlider from '../form/PeriodRangeSlider';
import NumberInput from '../form/NumberInput';
import Textarea from '../form/Textarea';
import HiddenInput from '../form/HiddenInput';
import EmailInput from '../form/EmailInput';
import PasswordInput from '../form/PasswordInput';
import Select from '../form/Select';
import MultiSelect from '../form/MultiSelect';

import { FieldError } from '../FormHelpers';
import QuestionRow from './QuestionRow';

/**
 * Provides data for "debug-data" HTML attribute
 * @param {object} question
 * @param {object} component
 * (Google Pupeteer)
 * @returns {string} a colon separated string with debug information
 */
const debugData = function(question, component) {
    // don't display in production mode
    if (process.env.NODE_ENV === 'production') {
        return '';
    }

    return `${question.id}:${question.type}:${component.name}`;
};

/**
 * Fetches form component for a given question type
 * @param {string} questionType
 *
 * @returns {function} React Component
 */
const getComponentByType = function(questionType = 'TEXT') {

    switch (questionType) {

        case 'INT':
        case 'FIXEDPOINT':
            return NumberInput;

        case 'MULTICHOICE':
            return CheckboxGroup;

        case 'MULTISELECT':
            return MultiSelect;

        case 'LATLON':
            return GeoLocation;

        // TODO DATETIME
        // TODO DAYTIME slider/select

        case 'DAYTIME':
            return DayTimeSlider;

        case 'TIMERANGE':
            return PeriodRangeSlider;

        case 'TEXTAREA':
            return Textarea;

        // case TEXT > default

        // not required!
        case 'CHECKBOX':
            return Checkbox;

        // html slide
        // no value!
        // no validation!
        // not required!
        case 'HIDDEN':
            return HiddenInput;

        case 'EMAIL':
            return EmailInput;

        case 'PASSWORD':
            return PasswordInput;

        // TODO SINGLECHOICE
        case 'SINGLECHOICE':
            return RadioGroup;

        case 'SINGLESELECT':
            return Select;

        default:
            return TextInput;
    }

};

/**
 * Extracts potential Error from answer object
 * @param {object} answer object, consisting of 'value' and 'serialized' properties
 *
 * @returns {null|Error}
 */
const getError = function(answer) {
    return (answer && answer.serialized instanceof Error) ? answer.serialized : null;
};

class Question extends Component {

    constructor(props) {
        super(props);

        this.state = {
            component: this.getComponent(),
        }
    }

    componentWillUnmount() {
        window.removeEventListener('resize', this.updateBreakpoint);
    }

    // fetch form control component and handle special cases
    getComponent() {
        const { question, component } = this.props;

        return (component && typeof component === 'function') ? component : getComponentByType(question.type);
    }

    render() {
        const { question, answer, handleChange, grouped, className, ...componentProps } = this.props;
        const Component = this.state.component;

        return (
            <QuestionRow
                className={ className }
                question={ question }
                componentName={ Component.name }
                grouped={ grouped }
                debugData= { debugData(question, Component ) }
            >
                <Component
                    { ...componentProps }
                    value={ (answer) ? answer.values[0] : null }
                    question={ question }
                    handleChange={ handleChange }
                    required
                />
                <FieldError error={ getError(answer) } />
            </QuestionRow>
        );
    }

};

Question.defaultProps = {
    answer: null,
    component: null,
    classNames: {},
    grouped: false,
};

Question.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    answer: PropTypes.shape({
        values: PropTypes.any,
        serialized: PropTypes.oneOfType([
            PropTypes.string,
            PropTypes.instanceOf(Error),
        ])
    }),

    component: PropTypes.func,
    className: PropTypes.string,
    grouped: PropTypes.bool,
    // ...and component specific props
};

export { Question as default, getComponentByType, debugData };
