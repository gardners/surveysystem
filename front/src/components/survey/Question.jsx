import React from 'react';
import PropTypes from 'prop-types';

import QuestionModel from '../../Question';

import TextInput from '../form/TextInput';
import RadioGroup from '../form/RadioGroup';
import CheckboxGroup from '../form/CheckboxGroup';
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

/**
 * Render Previous/Next/Finish buttos
 * The component should not contain survey logic or handle complex data. It merely recieves a number of flags from the parent component
 */
const Question = function({ question, answer, handleChange, component, appearance, grouped, className, ...componentProps } ) {

    // fetch form control component and handle special cases
    const Component = (component && typeof component === 'function') ? component : getComponentByType(question.type);

    return (
        <QuestionRow className={ className } question={ question } appearance={ appearance } grouped={ grouped }>
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

};

Question.defaultProps = {
    answer: null,
    appearance: 'default',
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
    appearance: PropTypes.oneOf([
        'default',
        'horizontal',
        'inline',
        'matrix'
    ]),
    grouped: PropTypes.bool,
    // ...and component specific props
};

export { Question as default, getComponentByType };
