import React from 'react';
import PropTypes from 'prop-types';

import QuestionModel from '../../Question';

import TextInput from '../form/TextInput';
import RadioGroup from '../form/RadioGroup';
import CheckboxGroup from '../form/CheckboxGroup';
import DayTimeSlider from '../form/DayTimeSlider';
import Checkbox from '../form/Checkbox';
import DeviceLocation from '../form/DeviceLocation';
import PeriodRangeSlider from '../form/PeriodRangeSlider';
import NumberInput from '../form/NumberInput';
import Textarea from '../form/Textarea';
import HiddenInput from '../form/HiddenInput';
import EmailInput from '../form/EmailInput';
import PasswordInput from '../form/PasswordInput';
import Select from '../form/Select';
import MultiSelect from '../form/MultiSelect';
import DaytimeSequence from '../form/DaytimeSequence';

import { addClassNames } from '../../Utils';

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
        return null;
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
            return DeviceLocation;

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

        case 'DAYTIME_SEQUENCE':
            return DaytimeSequence;

        default:
            return TextInput;
    }

};

/**
 * Question
 */

const Question = function({ handleChange, question, value, error, grouped, component, className, ...fallthroughProps }) {

    const Component = (typeof component === 'function') ? component : getComponentByType(question.type);
    const cls = addClassNames('question', className, (grouped) ? 'grouped' : '');
    return (
        <div className={ cls }>
            <Component
                { ...fallthroughProps }
                handleChange={ handleChange }
                question={ question }
                value={ value }
                error={ error }
                required
            />
        </div>
    );
};

Question.defaultProps = {
    component: null,
    className: '',
    grouped: false,
    value: null,
    error: null,
};

Question.propTypes = {
    className: PropTypes.string,
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    value: QuestionModel.valuePropTypes(),
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    component: PropTypes.func,
    // ...and component specific props
};

export { Question as default, getComponentByType, debugData };
