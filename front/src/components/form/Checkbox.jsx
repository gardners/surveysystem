import React from 'react';
import PropTypes from 'prop-types';

import { InputGroup } from '../FormHelpers';
import Question from '../../Question';

const Checkbox = function(props) {

    const { question, required } = props;
    const { choices } = question;

    return (
        <div className="form-check">
            <InputGroup prepend={ question.unit }>
                <input
                    id={ question.id }
                    name={ question.name }
                    type="checkbox"
                    className="form-check-input"
                    autoComplete="off"
                    required={ required }
                    onChange={ (e) => {
                        props.handleChange(e.target, question, (e.target.checked) ?  choices[1] : choices[0]);
                    } }
                />
            </InputGroup>
        </div>
    );
};

Checkbox.defaultProps = {
    required: false, //! different to other element defaults
};

Checkbox.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: Question.propTypes(true).isRequired,
    required: PropTypes.bool,
};

export default Checkbox;
