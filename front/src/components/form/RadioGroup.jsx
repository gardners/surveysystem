import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';

class RadioGroup extends Component {
    constructor(props) {
        super(props);
        this.state = {
            value: '',
        };
    }

    componentDidMount() {
        const { question } = this.props;
        this.setState({
            value: question.default_value,
        });
    }

    handleChange(e) {
        const { value } = e.target;
        const { question } = this.props;

        this.setState({
            value: value,
        });

        this.props.handleChange(e.target, question, value);
    }

    render() {
        const { question, error, required, grouped, className } = this.props;
        const { choices } = question;
        const { value } = this.state;

        return (
            <Field.Row className={ className } question={ question } grouped={ grouped } required={ required }>
                <Field.Description question={ question } grouped={ grouped } required={ required } />
                <Field.Title element="label" grouped={ grouped } question={ question } required={ required }>
                    <Field.Unit className="badge badge-secondary ml-1" question={ question } grouped={ grouped } />
                </Field.Title>
                {
                    choices.map((choice, index) => (
                        <div key={index} className="radio form-check">
                            <input
                                type="radio"
                                className="form-check-input"
                                autoComplete="off"
                                name={ question.name }
                                id={ `${question.name}[${index}]` }
                                value={ choice }
                                onChange={ this.handleChange.bind(this)}
                                checked={ choice === value }
                            />
                            <label htmlFor={ `${question.name}[${index}]` } className="form-check-label">{ choice }</label>
                        </div>
                    ))
                }
                <Field.Error error={ error } grouped={ grouped } />
            </Field.Row>
        );
    }
}

RadioGroup.defaultProps = {
    grouped: false,
    required: false,
};

RadioGroup.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    required: PropTypes.bool,
};

export default RadioGroup;
