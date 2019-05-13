import React from 'react';
import PropTypes from 'prop-types';

import Question from '../../Question';
import { matchesBreakpoint } from '../../Media';

import Select from './Select';

import './RadioMatrix.scss';
//TODO, required, error

/**
 * @var {string} MEDIA_BREAKPOINT bootstrap media query breaKpoint who triggers a d=single <Select> column instead separate columns of radio buttons
 */
const MEDIA_BREAKPOINT = 'md';

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
    question: Question.propTypes(true).isRequired,
    expanded: PropTypes.bool.isRequired,
};

const Row = function({ question, handleChange, expanded, required }) {

    if(!expanded) {
        return (
            <tr>
                <td className="radiomatrix--firstcol">{ question.title }</td>
                <td>
                    <Select
                        key={ question.id }
                        question={ question }
                        handleChange={ handleChange }
                    />
                </td>
            </tr>
        );
    }

    const choices = question.choices || [];

    return (
        <tr>
            <td className="radiomatrix--firstcol">{ question.title }</td>
            {
                choices.map((choice, index) => {
                    return (
                        <td key={ index }>
                            <input
                                type="radio"
                                id={ question.id }
                                name={ question.name }
                                value={ choice }
                                onChange={ (e) => handleChange(e.target, question, choice) }
                                required={ required }
                            />
                        </td>
                    );
                })
            }
        </tr>
    );
};

Row.defaultProps = {
    required: false,
};

Row.propTypes = {
    question: Question.propTypes(true).isRequired,
    handleChange: PropTypes.func.isRequired,
    expanded: PropTypes.bool.isRequired,
    required: PropTypes.bool,
};

const RadioMatrix = function({ questions, handleChange, required, expand }) {
    //TODO
    if(!questions.length) {
        return (null);
    }

    const expanded = (expand === null) ? matchesBreakpoint(MEDIA_BREAKPOINT) : expand;
    const first = questions[0];

    return (
        <div className="table-responsive">
            <table className="table table-sm table-hover radiomatrix--table">
                <thead>
                    <TheadRow
                        question={ first }
                        expanded={ expanded }
                    />
                </thead>
                <tbody>
                    {
                        questions.map((question, index) => <Row
                            key={ index }
                            question={ question }
                            handleChange={ handleChange }
                            required={ required }
                            expanded={ expanded }
                        />)
                    }
                </tbody>
            </table>
        </div>
    );
};

RadioMatrix.defaultProps = {
    required: true,
    expand: null,
};

RadioMatrix.propTypes = {
    handleChange: PropTypes.func.isRequired,
    questions: PropTypes.arrayOf(
        Question.propTypes(true)
    ),
    required: PropTypes.bool,
    expand: PropTypes.bool, // force contracted or expanded display
};

export default RadioMatrix;
