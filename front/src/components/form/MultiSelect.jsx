import React from 'react';
import PropTypes from 'prop-types';

import { InputGroup } from '../FormHelpers';
import Question from '../../Question';

const getValues = function(element) {
    const { options } = element;
    const values = [];
    for (let i = 0; i < options.length; i += 1) {
        if (options[i].selected) {
            values.push(options[i].value);
        }
    }
    return values;
};

const MultiSelect = function(props) {
    const { question } = props;
    const { choices } = question;

    return (
        <InputGroup prepend={ question.unit }>
            <select
                multiple
                id={ question.id }
                name={ question.name }
                className="form-control"
                onChange={ (e) => {
                    const element = e.target;
                    const values = getValues(element);
                    props.handleChange(element, question, values);
                } }>

                { choices.map((value, index) => {
                    return <option key={ index } value={ value }>{ value }</option>
                }) }

            </select>
        </InputGroup>
    );
}

MultiSelect.defaultProps = {
    required: true,
};

MultiSelect.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: Question.propTypes(true).isRequired,
    required: PropTypes.bool,
};

export default MultiSelect;
