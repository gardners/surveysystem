import React from 'react';
import PropTypes from 'prop-types';

import { InputGroup } from '../FormHelpers';

const Checkbox = function(props) {
    const { question, value, required } = props;

    return (
        <div className="form-group form-check">
            <InputGroup prepend={ question.unit }>
                <input
                    id={ question.id }
                    name={ question.name }
                    type="checkbox"
                    className="form-check-input"
                    value={ value }
                    autoComplete="off"
                    required={ required }
                    onChange={ (e) => {
                        const { value } = e.target;
                        props.handleChange(e.target, question, (e.target.checked) ?  props.choices[1] : props.choices[0]);
                    } }
                /><label className="form-check-label" htmlFor={ question.id }>{ question.title }</label>
            </InputGroup>
        </div>
    );
};

Checkbox.defaultProps = {
    required: false, //! different to other element defaults
};

Checkbox.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: PropTypes.shape({
        id: PropTypes.string.isRequired,
        name: PropTypes.string.isRequired,
        title: PropTypes.string.isRequired,
        title_text: PropTypes.string.isRequired,
        type: PropTypes.string.isRequired,
        unit: PropTypes.string.isRequired,

        // eunum
        choices: PropTypes.array.isRequired,// TODO custom proptypes, checking for length === 2
    }).isRequired,
    required: PropTypes.bool,
};

export default Checkbox;
