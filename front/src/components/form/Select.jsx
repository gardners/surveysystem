import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';

class Select extends Component {
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
        const { question, handleChange } = this.props;

        this.setState({
            value,
        });
        handleChange(e.target, question, value);
    }

    render() {
        const { question, error, required, grouped, className } = this.props;
        const { choices } = question;
        const { value } = this.state

        return (
            <Field.Row className={ className } question={ question } grouped={ grouped } required={ required }>
                <Field.Description question={ question } grouped={ grouped } required={ required } />
                <Field.Title element="label" grouped={ grouped } question={ question } required={ required }>
                    <Field.Unit className="badge badge-secondary ml-1" question={ question } grouped={ grouped } />
                </Field.Title>
                <div className="input-group">
                    <div className="input-group-prepend d-none d-sm-block">
                        <span className="input-group-text">Select...</span>
                    </div>
                    <select
                        multiple={ false }
                        className="form-control"
                        autoComplete="off"
                        name={ question.name }
                        id={ question.id }
                        value={ value }
                        onChange={ this.handleChange.bind(this)}
                    >
                        { choices.map((choice, index) => <option key={ index } value={ choice }> { choice }</option>) }
                    </select>
                    {
                        question.unit &&
                        <div className="input-group-append d-none d-sm-block">
                            <span className="input-group-text">{ question.unit }</span>
                        </div>
                    }
                </div>
                <Field.Error error={ error } grouped={ grouped } />
            </Field.Row>
        );
    }
}

Select.defaultProps = {
    grouped: false,
    required: false,
};

Select.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    required: PropTypes.bool,
};

export default Select;
