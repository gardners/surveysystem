import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';


class Checkbox extends Component {

    constructor(props) {
        super(props);

        this.state = {
            checked: false,
        };
    }

    componentDidMount() {
        const { question } = this.props;
        this.setState({
            checked: question.default_value === question.choices[1],
        });
    }

    handleChange(e){
        const { question, handleChange } = this.props;
        const { choices } = question;
        const { checked } = e.target;
        this.setState({
            checked,
        });
        handleChange(e.target, question, (checked) ?  choices[1] : choices[0]);
    }

    render() {
        const { checked  } = this.state;
        const { question, error, required, grouped, className } = this.props;
        return (
            <Field.Row className={ className } question={ question } grouped={ grouped } required={ required }>
                <Field.Description question={ question } grouped={ grouped } required={ required } />
                <div className="form-check">
                    <input
                        id={ question.id }
                        name={ question.name }
                        type="checkbox"
                        className="form-check-input"
                        autoComplete="off"
                        checked={ checked }
                        required={ false /* single checkbox! */ }
                        onChange={ this.handleChange.bind(this) }
                    />
                    <Field.Title element="label" grouped={ grouped } question={ question } required={ required }>
                        <Field.Unit className="badge badge-secondary ml-1" question={ question } grouped={ grouped } />
                    </Field.Title>
                </div>
                <Field.Error error={ error } grouped={ grouped } />
            </Field.Row>
        );
    }
};

Checkbox.defaultProps = {
    grouped: false,
    required: false,
};

Checkbox.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: QuestionModel.propTypes().isRequired,
    value: QuestionModel.valuePropTypes(),
    error: PropTypes.instanceOf(Error),
    grouped: PropTypes.bool,
    required: PropTypes.bool,

    className: PropTypes.string,
};

export default Checkbox;
