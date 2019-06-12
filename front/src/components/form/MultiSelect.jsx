import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';

import { isArray } from '../../Utils';

class MultiSelect extends Component {
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
        const { options } = e.target;
        const { question, handleChange } = this.props;

        const values = [];

        const { length } = options;
        for (let i = 0; i < length; i += 1) {
            if (options[i].selected) {
                values.push(options[i].value);
            }
        }

        this.setState({
            values,
        });

        handleChange(e.target, question, values);
    }

    render() {
        const { question, error, required, grouped, className } = this.props;
        const { choices } = question;
        const { values } = this.state

        return (
            <Field.Row className={ className } question={ question } grouped={ grouped } required={ required }>
                <Field.Description question={ question } grouped={ grouped } required={ required } />
                <Field.Title element="label" grouped={ grouped } question={ question } required={ required }>
                    <Field.Unit className="badge badge-secondary ml-1" question={ question } grouped={ grouped } />
                </Field.Title>
                <select
                    multiple={ true }
                    className="form-control"
                    autoComplete="off"
                    name={ question.name }
                    id={ question.id }
                    value={ values }
                    onChange={ this.handleChange.bind(this)}
                >
                    { choices.map((choice, index) => <option key={ index } value={ choice }> { choice }</option>) }
                </select>
                <Field.Error error={ error } grouped={ grouped } />
            </Field.Row>
        );
    }
}

MultiSelect.defaultProps = {
    grouped: false,
    required: false,
};

MultiSelect.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    required: PropTypes.bool,
};

export default MultiSelect;
