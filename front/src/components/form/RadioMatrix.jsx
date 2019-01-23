import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Question from '../../Question';
//TODO, required, error
const Thead = function(props) {

    const { question } = props;
    const { choices } = question;

    return(
        <tr>
            <th>{ question.unit && <em>({ question.unit })</em> }</th>
            {
                choices.map((value, index) => <th key={ index }>{ value }</th>)
            }
        </tr>
    );
};

Thead.propTypes = {
    question: Question.propTypes(true).isRequired,
};


class Row  extends Component {

   constructor(props) {
        super(props);

        this.state = {
            values: {},
        };
    }

    handleChange(e) {
        const { value } = e.target;
        const { question } = this.props;

        this.setState({
            value: value
        });

        this.props.handleChange(e.target, question, value);
    }

    render() {
        const { question } = this.props;
        const { choices } = question;

        return(
            <tr>
                <td>{ question.title }</td>
                {
                    choices.map((val, index) => <td key={ index }>
                        <input
                            type="radio"
                            name={ question.name }
                            id={ question.id }
                            value={ val }
                            onChange={ this.handleChange.bind(this) }
                        />
                    </td>)
                }
            </tr>
        );
    }
}

Row.defaultProps = {
    checked: false,
};


Row.propTypes = {
    question: Question.propTypes(true).isRequired,
    handleChange: PropTypes.func.isRequired,
    // no "required" prop
};


class RadioMatrix extends Component {

    constructor(props) {
        super(props);

        this.state = {
            values: {},
        };
    }

    handleChange(question, event) {
        event.preventDefault();
        const { values } = this.state;
        values[question.id] = event.target.value;

        this.setState({
            values
        });

        const args = [event.target].concat(Object.values(values));
        this.props.handleChange.apply(null, args);
    }

    render() {
        const { questions, handleChange } = this.props;
        const first = questions[0] || null;

        return (
            <div className="table-responsive">
                <table className="table table-sm table-hover">
                    <thead>
                        <Thead question={ first } />
                    </thead>
                    <tbody>
                        { questions.map(question => <Row
                                key={ question.id }
                                question={ question }
                                handleChange={ handleChange }
                        />) }
                    </tbody>
                </table>
            </div>
        );
    }
}

RadioMatrix.defaultProps = {
    required: true,
};

RadioMatrix.propTypes = {
    handleChange: PropTypes.func.isRequired,
    questions: PropTypes.arrayOf(
        Question.propTypes(true)
    ),
    required: PropTypes.bool,
};

export default RadioMatrix;
