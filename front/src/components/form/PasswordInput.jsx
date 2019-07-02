import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';

class PasswordInput extends Component {

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

    handleChange(e){
        const { question, handleChange } = this.props;
        const { value } = e.target;
        this.setState({
            value,
        });
        handleChange(e.target, question, value);
    }

    render() {
        const { value  } = this.state;
        const { question, error, required, grouped, className } = this.props;
        return (
            <Field.Row className={ className } question={ question } grouped={ grouped } required={ required }>
                <Field.Description question={ question } grouped={ grouped } required={ required } />
                <Field.Title element="label" grouped={ grouped } question={ question } required={ required }>
                    <Field.Unit className="badge badge-secondary ml-1" question={ question } grouped={ grouped } />
                </Field.Title>
                <div className="input-group">
                    <div className="input-group-prepend d-none d-sm-block">
                        <span className="input-group-text">password</span>
                    </div>
                    <input
                        id={ question.id }
                        name={ question.name }
                        type="password"
                        className="form-control"
                        autoComplete="off"
                        required={ required }
                        value={ value }
                        onChange={ this.handleChange.bind(this) }
                    />
                </div>
                <Field.Error error={ error } grouped={ grouped } />
            </Field.Row>
        );
    }
};

PasswordInput.defaultProps = {
    grouped: false,
    required: false,
};

PasswordInput.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    value: QuestionModel.valuePropTypes(),
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    required: PropTypes.bool,

    className: PropTypes.string,
};

export default PasswordInput;
