import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';

class Textarea extends Component {
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
                    <textarea
                        id={ question.id }
                        name={ question.name }
                        className="form-control"
                        autoComplete="off"
                        required={ required }
                        value={ value }
                        onChange={ this.handleChange.bind(this) }
                    />
                <Field.Error error={ error } grouped={ grouped } />
            </Field.Row>
        );
    }
};

Textarea.defaultProps = {
    grouped: false,
    required: false,
};

Textarea.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    value: QuestionModel.valuePropTypes(),
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    required: PropTypes.bool,

    className: PropTypes.string,
};

export default Textarea;
