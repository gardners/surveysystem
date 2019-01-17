import React from 'react';
import PropTypes from 'prop-types';

import QuestionModel from '../../Question';

import TextInput from '../form/TextInput';
import RadioGroup from '../form/RadioGroup';
import RadioMatrixRow from '../form/RadioMatrixRow';
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

/**
 * Render Previous/Next/Finish buttos
 * The component should not contain survey logic or handle complex data. It merely recieves a number of flags from the parent component
 */
const Question = function({ question, answer, handleChange, matrixState }) {

    switch (question.type) {

        case 'INT':
        case 'FIXEDPOINT':

            return (
                <NumberInput
                    value={ (answer) ? answer.values[0] : null }
                    question={ question }
                    handleChange={ handleChange }
                    required
                />
            );

        case 'MULTICHOICE':

            return (
                <CheckboxGroup
                    question={ question }
                    handleChange={ handleChange }
                    required
                />
            );

        case 'MULTISELECT':

            return (
                <MultiSelect
                    question={ question }
                    handleChange={ handleChange }
                    required
                />
            );

        case 'LATLON':

            return (
                <GeoLocation
                    value={ (answer) ? answer.values : '' }
                    question={ question }
                    handleChange={ handleChange }
                    required
                />
            );

        // TODO DATETIME
        // TODO DAYTIME slider/select

        case 'TIMERANGE':

            return (
                <PeriodRangeSlider
                    value={ (answer) ? answer.values : '' }
                    question={ question }
                    handleChange={ handleChange }
                    required
                />
            );

        case 'TEXTAREA':

            return (
                <Textarea
                    value={ (answer) ? answer.values[0] : null }
                    question={ question }
                    handleChange={ handleChange }
                    required
                />
            );

        // case TEXT > default

        // not required!
        case 'CHECKBOX':

            return (
                <Checkbox
                    question={ question }
                    handleChange={ handleChange }
                />
            );

        // html slide
        // no value!
        // no validation!
        // not required!
        case 'HIDDEN':

            return (
                <HiddenInput
                    question={ question }
                    defaultValue={ question.default_value || 'visited' /* TODO confirm with backend */ }
                    handleChange={ handleChange }
                />
            );

        case 'EMAIL':

            return (
                <EmailInput
                    value={ (answer) ? answer.values[0] : null }
                    question={ question }
                    handleChange={ handleChange }
                    required
                />
            );

        case 'PASSWORD':

            return (
                <PasswordInput
                    value={ (answer) ? answer.values[0] : null }
                    question={ question }
                    handleChange={ handleChange }
                    required
                />
            );

        // TODO SINGLECHOICE
        case 'SINGLECHOICE':

            if(matrixState[0] || matrixState[1]) {
                return (
                    <RadioMatrixRow
                        isFirst={ matrixState[0] }
                        isLast={ matrixState[1] }
                        question={ question }
                        handleChange={ handleChange }
                        required
                    />
                );
            }

            return (
                <RadioGroup
                    question={ question }
                    handleChange={ handleChange }
                    required
                />
            );

        case 'SINGLESELECT':

            return (
                <Select
                    value={ (answer) ? answer.values[0] : null }
                    question={ question }
                    handleChange={ handleChange }
                    required
                />
            );

        default:

            return  (
                <TextInput
                    value={ (answer) ? answer.values[0] : null }
                    question={ question }
                    handleChange={ handleChange }
                    required
                />
            );
    }
};

Question.defaultProps = {
    answer: null,
    matrixState: [false, false],
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
    matrixState: PropTypes.arrayOf([
        PropTypes.bool.isRequired,
        PropTypes.bool.isRequired,
    ]),
};

export default Question;
