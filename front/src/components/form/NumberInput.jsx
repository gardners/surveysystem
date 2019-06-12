import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';

class NumberInput extends Component {

    constructor(props) {
        super(props);

        this.state = {
            value: 0,
        };
    }

    componentDidMount() {
        const { question } = this.props;
        const val = Number(question.default_value);
        const minval = Number(question.min_value);
        const maxval = Number(question.max_value);

        this.setState({
            value: (!isNaN(val)) ? val : 0,
            minValue: (!isNaN(minval)) ? minval : 0,
            maxValue: (!isNaN(maxval) && maxval > minval) ? maxval : Infinity,
        });
    }

    handleChange(e){
        const { question, handleChange } = this.props;
        const { value } = e.target;
        this.setState({
            value,
        });
        handleChange(e.target, question, value);
    }

    render() {
        const { value, minValue, maxValue } = this.state;
        const { question, error, required, grouped, className, step } = this.props;
        return (
            <Field.Row className={ className } question={ question } grouped={ grouped } required={ required }>
                <Field.Description question={ question } grouped={ grouped } required={ required } />
                <Field.Title element="label" grouped={ grouped } question={ question } required={ required }>
                    <Field.Unit className="badge badge-secondary ml-1" question={ question } grouped={ grouped } />
                </Field.Title>
                <input
                    id={ question.id }
                    name={ question.name }
                    type="number"
                    className="form-control"
                    autoComplete="off"
                    required={ required }
                    value={ value }
                    min={ minValue }
                    max={ maxValue }
                    step={ step }
                    onChange={ this.handleChange.bind(this) }
                />
                <Field.Error error={ error } grouped={ grouped } />
            </Field.Row>
        );
    }
};

NumberInput.defaultProps = {
    grouped: false,
    required: false,

    step: 1,
};

NumberInput.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    value: QuestionModel.valuePropTypes(),
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    required: PropTypes.bool,

    step: PropTypes.number,
    className: PropTypes.string,
};

export default NumberInput;
