import React from 'react';
import PropTypes from 'prop-types';

import { InputGroup } from '../FormHelpers';

const getValues = function(element) {
    const { options } = element;
    const values = [];
    for (let i = 0; i < options.length; i += 1) {
        if (options[i].selected) {
            values.push(options[i].value);
        }
    }
    return values.toString();
};

const MultiSelect = function(props) {
    const { question } = props;
    const { choices } = question;

    return (
        <div className="form-group">
            <label htmlFor={ question.id }>{ question.title }</label>
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
        </div>
    );
}

MultiSelect.defaultProps = {
    required: true,
};

MultiSelect.propTypes = {
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

export default MultiSelect;
