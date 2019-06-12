import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';

import { isArray } from '../../Utils';

class CheckboxGroup extends Component {
    constructor(props) {
        super(props);
        this.state = {
            values: [],
        };
    }

    componentDidMount() {
        const { question } = this.props;
        const values = (isArray(question.default_value)) ? question.default_value : question.default_value.split(',');
        this.setState({
            values,
        });
    }

    handleChange(e) {
        const { value, checked } = e.target;
        const { question, handleChange } = this.props;

        let { values } = this.state;

        values = values.filter(v => v !== value);
        if(checked) {
            values.push(value);
        }

        this.setState({
            values,
        });

        handleChange(e.target, question, values);
    }

    render() {
        const { question, error, required, grouped, className } = this.props;
        const { choices } = question;
        const { values } = this.state;

        return (
            <Field.Row className={ className } question={ question } grouped={ grouped } required={ required }>
                <Field.Description question={ question } grouped={ grouped } required={ required } />
                <Field.Title element="label" grouped={ grouped } question={ question } required={ required }>
                    <Field.Unit className="badge badge-secondary ml-1" question={ question } grouped={ grouped } />
                </Field.Title>
                {
                    choices.map((choice, index) => (
                        <div key={index} className="radio form-check form-check-inline">
                            <input
                                type="checkbox"
                                className="form-check-input"
                                autoComplete="off"
                                name={ question.name }
                                id={ `${question.name}[${index}]` }
                                value={ choice }
                                onChange={ this.handleChange.bind(this)}
                                checked={  values.indexOf(choice) > -1 }
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

CheckboxGroup.defaultProps = {
    grouped: false,
    required: false,
};

CheckboxGroup.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    required: PropTypes.bool,
};

export default CheckboxGroup;
