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

import { FormGroup, FormLabel, FormControl, FieldError } from '../FormHelpers';

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

const getError = function(answer) {
    return (answer && answer.serialized instanceof Error) ? answer.serialized : null;
};

const getApperanceClasses = function(appearance) {

    const classNames = {
        formGroup: 'row',
        label: 'col-sm-3 col-form-label',
        control: 'col-sm-9',
    };

    switch(appearance) {
        case 'matrix':
            classNames.formGroup += ' border';
        break;

        default:
            // nothing
    }

    return classNames;
};
/**
 * Render Previous/Next/Finish buttos
 * The component should not contain survey logic or handle complex data. It merely recieves a number of flags from the parent component
 */
const Question = function({ question, answer, handleChange, component, appearance, ...componentProps } ) {

    // appearance classes
    // @see https://getbootstrap.com/docs/4.2/components/forms/#layout
    const classes = getApperanceClasses(appearance);

    // fetch form control component and handle special cases
    const Component = (component && typeof component === 'function') ? component : getComponentByType(question.type);

    if(Component.name === 'HiddenInput') {
        return (
            <Component
                { ...componentProps }
                value={ (answer) ? answer.values[0] : null }
                question={ question }
                handleChange={ handleChange }
                defaultValue={ question.default_value || 'visited' /* TODO confirm with backend */ }
                required
            />
        );
    }

    return (
        <FormGroup className={ classes.group }>
            <FormLabel className={ classes.label }>{ question.title }</FormLabel>
            <FormControl className={ classes.control }>

                <Component
                    { ...componentProps }
                    value={ (answer) ? answer.values[0] : null }
                    question={ question }
                    handleChange={ handleChange }
                    required
                />

            </FormControl>
            <FieldError error={ getError(answer) } />
        </FormGroup>
    );

};

Question.defaultProps = {
    answer: null,
    appearance: 'default',
    component: null,
    classNames: {},
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
    appearance: PropTypes.oneOf([
        'default',
        'inline',
        'matrix'
    ]),
    classNames: PropTypes.object,
    // ...and component specific props
};

export { Question as default, getComponentByType };
