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

    handleChange(value) {
        const { question } = this.props;

        this.setState({
            value: value,
        });

        this.props.handleChange(null, question, value);
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
                <div className="list-group">
                {
                    choices.map((choice, index) => {
                        const checked = (choice === value);
                        return (
                            <button
                                key={ index }
                                id={ `${question.id}[${index}]` }
                                name={ question.name }
                                className={ (checked) ? 'text-left list-group-item list-group-item-primary' : 'text-left list-group-item' }
                                value={ choice }
                                onClick={
                                    //
                                    (e) => {
                                        e.preventDefault();
                                        this.handleChange(choice);
                                    }
                                }
                            >
                                { (checked) ? <i className="mr-2 fas fa-check-circle text-primary" /> : <i className="mr-2 far fa-circle text-muted" /> }
                                { choice }
                            </button>
                        );
                    })
                }
                </div>
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
