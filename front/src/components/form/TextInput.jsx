import React from 'react';
import PropTypes from 'prop-types';

import { InputGroup } from '../FormHelpers';
import Question from '../../Question';

const TextInput = function(props) {
    const { question, placeholder } = props;

    return (
        <InputGroup prepend={ question.unit }>

            <input
                id={ question.id }
                name={ question.name }
                type="text"
                className="form-control"
                placeholder={ placeholder }
                autoComplete="off"
                onChange={ (e) => {
                    const { value } = e.target;
                    props.handleChange(e.target, question, value);
                } }
            />
        </InputGroup>
    );
};

TextInput.defaultProps = {
    required: true,
    placeholder: null,
};

TextInput.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: Question.propTypes().isRequired,
    required: PropTypes.bool,
    placeholder: PropTypes.string,
};

export default TextInput;
