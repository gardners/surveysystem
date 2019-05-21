import React from 'react';
import PropTypes from 'prop-types';

import FormControlLabel from '@material-ui/core/FormControlLabel';
import MuiCheckbox from '@material-ui/core/Checkbox';

import Question from '../../Question';

/**
 * requires two defined choices in the following order: [OFF-value, ON-value]
 */
const Checkbox = function({ question, required, handleChange }) {
    const { id, name, choices } = question;

    return (
        <FormControlLabel
            control={
                <MuiCheckbox
                    color="primary"
                    id={ id }
                    name={ name }
                    required={ required }
                    onChange={ (e) => {
                        handleChange(e.target, question, (e.target.checked) ?  choices[1] : choices[0]);
                    } }
                />
            }
            label={ choices[1] }
        />
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
