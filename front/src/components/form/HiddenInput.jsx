import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Question from '../../Question';

// no Inputgroup

class HiddenInput extends Component {

    // ! immediately invoke answer callback
    // note: no value and validation! handleChange is invoked immediately with defaultvalue
    componentDidMount() {
        const { question, defaultValue } = this.props;
        this.props.handleChange(null, question, defaultValue || 'visited' );
    }

    render() {
        const { question, defaultValue } = this.props;
        return (
            <input
                id={ question.id }
                name={ question.name }
                type="hidden"
                autoComplete="off"
                defaultValue={ defaultValue }
            />
        );
    }
};

HiddenInput.defaultProps = {
    required: true,
    defaultValue: null,
};

HiddenInput.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: Question.propTypes().isRequired,
    // no "required"prop

    // custom
    defaultValue: PropTypes.string,
};

export default HiddenInput;
