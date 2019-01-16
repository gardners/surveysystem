import React from 'react';
import PropTypes from 'prop-types';

import { InputGroup } from '../FormHelpers';
import Question from '../../Question';

const Select = function(props) {
    const { question } = props;
    const { choices } = question;

    return (
        <div className="form-group">
            <label htmlFor={ question.id }>{ question.title }</label>
            <InputGroup prepend={ question.unit }>
                <select
                    id={ question.id }
                    name={ question.name }
                    className="form-control"
                    onChange={ (e) => {
                        const { value } = e.target;
                        props.handleChange(e.target, question, value);
                    } }>

                    { choices.map((value, index) => {
                        return <option key={ index } value={ value }>{ value }</option>
                    }) }

                </select>
            </InputGroup>
        </div>
    );
}

Select.defaultProps = {
    required: true,
};

Select.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: Question.propTypes(true).isRequired,
    required: PropTypes.bool,
};

export default Select;
