import React from 'react';
import PropTypes from 'prop-types';

import Table from '@material-ui/core/Table';
import TableBody from '@material-ui/core/TableBody';
import TableCell from '@material-ui/core/TableCell';
import TableHead from '@material-ui/core/TableHead';
import TableRow from '@material-ui/core/TableRow';

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
        <TableRow>
            <TableCell className="radiomatrix--firstcol"></TableCell>
            {
                labels.map((label, index) => <TableCell key={ index }>{ label }</TableCell>)
            }
        </TableRow>
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
            <TableRow hover>
                <TableCell className="radiomatrix--firstcol">{ question.title }</TableCell>
                <TableCell>
                    <Select
                        key={ question.id }
                        question={ question }
                        handleChange={ handleChange }
                    />
                </TableCell>
            </TableRow>
        );
    }

    const choices = question.choices || [];

    return (
        <TableRow hover>
            <TableCell className="radiomatrix--firstcol">{ question.title }</TableCell>
            {
                choices.map((choice, index) => {
                    return (
                        <TableCell key={ index }>
                            <input
                                type="radio"
                                id={ question.id }
                                name={ question.name }
                                value={ choice }
                                onChange={ (e) => handleChange(e.target, question, choice) }
                                required={ required }
                            />
                        </TableCell>
                    );
                })
            }
        </TableRow>
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
        <Table>
            <TableHead>
                <TheadRow
                    question={ first }
                    expanded={ expanded }
                />
            </TableHead>
            <TableBody>
                {
                    questions.map((question, index) => <Row
                        key={ index }
                        question={ question }
                        handleChange={ handleChange }
                        required={ required }
                        expanded={ expanded }
                    />)
                }
            </TableBody>
        </Table>
    );
};

RadioMatrix.defaultProps = {
    required: true,
    expand: null, // indicates that bool was not set yet (testing), TODO
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
