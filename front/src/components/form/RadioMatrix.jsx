import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Field from './Field';
import QuestionModel from '../../Question';
import { AppContext } from '../../Context';

import './RadioMatrix.scss';

const TheadRow = function({ question, expanded }) {

    if(!expanded) {
        return(null);
    }

    const labels = question.choices || [];

    return (
        <tr>
            <th className="radiomatrix--firstcol"></th>
            {
                labels.map((label, index) => <th key={ index }>{ label }</th>)
            }
        </tr>
    );
};

TheadRow.defaultProps = {
};

TheadRow.propTypes = {
    question: QuestionModel.propTypes(true).isRequired,
    expanded: PropTypes.bool.isRequired,
};

const Row = function({ question, index, value, handleChange, expanded, required, className }) {

    const choices = question.choices || [];

    if(!expanded) {
        return (
            <tr className={ className }>
                <td className="radiomatrix--firstcol">{ question.title }</td>
                <td className="align-middle">
                    <select
                        id={ question.id }
                        name={ question.name }
                        autoComplete="off"
                        key={ question.id }
                        question={ question }
                        value={ value }
                        onChange={ (e) => handleChange(question, e.target.value) }
                    >
                    {
                        choices.map((choice, key) => {
                            return (
                                <option
                                    key= { key }
                                    value={ choice }
                                >
                                    { choice }
                                </option>
                            );
                        })
                    }
                    </select>
                </td>
            </tr>
        );
    }

    /* eslint-disable eqeqeq */
    return (
        <tr className={ className }>
            <td className="radiomatrix--firstcol">{ question.title }</td>
            {
                choices.map((choice, index) => {
                    const checked= (choice == value); /* intentionally using non-typesafe operator (number) */
                    return (
                        <td key={ index } className="align-middle">
                            <button
                                key={ index }
                                id={ `${question.id}[${index}]` }
                                name={ question.name }
                                className={ (checked) ? 'btn btn-sm text-primary' : 'btn btn-sm' }
                                value={ choice }
                                onClick={
                                    (e) => {
                                        e.preventDefault();
                                        handleChange(question, choice);
                                    }
                                }
                            >
                                { (checked) ? <i className="fas fa-check-circle text-primary" /> : <i className="far fa-circle text-muted" /> }
                            </button>
                        </td>
                    );
                })
            }
        </tr>
    );
    /* eslint-enable eqeqeq */
};

Row.defaultProps = {
    required: false,
};

Row.propTypes = {
    question: QuestionModel.propTypes(true).isRequired,
    index: PropTypes.number.isRequired,
    handleChange: PropTypes.func.isRequired,
    expanded: PropTypes.bool.isRequired,
    value: PropTypes.any,
    required: PropTypes.bool,

    className: PropTypes.string,
};

class RadioMatrix extends Component {

    constructor(props) {
        super(props);

        this.state = {
            values: {},
        };
    }

    componentDidMount() {
        const { questions } = this.props;
        const values = {};

        questions.forEach((q) => {
            values[q.id] = q.default_value;
        });

        this.setState({
            values,
        });
    }

    handleChange(question, value) {
        const { values } = this.state;
        values[question.id] = value;

        this.setState({
            values,
        });

        this.props.handleChange(null, question, value);
    }

    render() {
        const { values } = this.state;
        const { questions, errors, required, grouped, className } = this.props;

        if(!questions.length) {
            return (null);
        }

        const first = questions[0];

        return (
            <Field.Row className={ className } question={ first } grouped={ grouped } required={ required }>
                { /* No Field.title */ }
                <Field.Description question={ first } grouped={ grouped } required={ required }>
                    <Field.Unit className="badge badge-secondary ml-1" question={ first } grouped={ grouped } />
                </Field.Description>

                <AppContext.Consumer>
                {
                    ({ breakpoint, matchesBreakpointOrAbove }) => (
                        <div className="table-responsive">
                            <table className="table table-sm table-hover table-borderless table-striped radiomatrix--table">
                                <thead>
                                    <TheadRow
                                        question={ first }
                                        expanded={ matchesBreakpointOrAbove('md') }
                                    />
                                </thead>
                                <tbody>
                                    {
                                        questions.map((question, index) => <Row
                                            key={ index }
                                            index={ index }
                                            question={ question }
                                            value={ values[question.id] }
                                            handleChange={ this.handleChange.bind(this) }
                                            required={ required }
                                            expanded={ matchesBreakpointOrAbove('md') }
                                        />)
                                    }
                                </tbody>
                            </table>
                        </div>
                    )
                }
                </AppContext.Consumer>

                { Object.keys(errors).map(id => <Field.Error key= { id } error={ errors[id] } grouped={ grouped } />) }
            </Field.Row>
        );
    }
};

RadioMatrix.defaultProps = {
    grouped: false,
    required: false,
};

RadioMatrix.propTypes = {
    handleChange: PropTypes.func.isRequired,
    questions: PropTypes.arrayOf(
        QuestionModel.propTypes(),
    ).isRequired,

    errors: PropTypes.object.isRequired,
    grouped: PropTypes.bool,
    required: PropTypes.bool,

    className: PropTypes.string,
};

export default RadioMatrix;
