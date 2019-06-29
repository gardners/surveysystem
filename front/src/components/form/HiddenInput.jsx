import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';


class HiddenInput extends Component {

    constructor(props) {
        super(props);

        this.state = {
            value: '',
        };
    }

    componentDidMount() {
        const { question } = this.props;
        const value = question.default_value || 'visited';
        this.setState({
            value,
        });

        // ! immediately invoke answer callback, no value and validation! handleChange is invoked immediately with defaultvalue
        this.props.handleChange(null, question, value);
    }

    render() {
        const { value  } = this.state;
        const { question, grouped, className } = this.props;

        // # 224, don't flag this qtype as required
        const required = false;

        return (
            <Field.Row className={ className } question={ question } grouped={ grouped } required={ required }>
                <Field.Title grouped={ grouped } question={ question } required={ required } />
                <Field.Description question={ question } grouped={ grouped } required={ required } />
                <input
                    id={ question.id }
                    name={ question.name }
                    type="hidden"
                    autoComplete="off"
                    value={ value }
                />
                { /* No Field.Unit, No Field.Error */ }
            </Field.Row>
        );
    }
};

HiddenInput.defaultProps = {
    grouped: false,
    required: false,
};

HiddenInput.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    value: QuestionModel.valuePropTypes(),
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    required: PropTypes.bool,

    className: PropTypes.string,
};

export default HiddenInput;
