import React from 'react';
import PropTypes from 'prop-types';

import { InputGroup } from '../FormHelpers';

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
    question: PropTypes.shape({
        id: PropTypes.string.isRequired,
        name: PropTypes.string.isRequired,
        title: PropTypes.string.isRequired,
        title_text: PropTypes.string.isRequired,
        type: PropTypes.string.isRequired,
        unit: PropTypes.string.isRequired,
        // eunum
        choices: PropTypes.array.isRequired,
    }).isRequired,
    required: PropTypes.bool,
};

export default Select;
