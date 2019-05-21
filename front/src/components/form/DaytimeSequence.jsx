import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Table from '@material-ui/core/Table';
import TableBody from '@material-ui/core/TableBody';
import TableCell from '@material-ui/core/TableCell';
import TableHead from '@material-ui/core/TableHead';
import TableRow from '@material-ui/core/TableRow';

import Question from '../../Question';
import { prettyHours, DaySeconds } from '../../Utils';
import { matchesBreakpoint } from '../../Media';

import InputRange from 'react-input-range';
import TimePicker from './TimePicker';

import './DaytimeSequence.scss';

const tableStyle = {
    display: 'table',
    width: '100%',
    padding: '.5em',
    tableLayout: 'fixed',    /* For cells of equal size */
};

const cellStyle = function(percent) {
    const textAlign = ((percent === 50) ? 'center' : (percent < 50) ? 'left' : 'right');
    return {
        width: '25%',
        display: 'table-cell',
        textAlign,
    };
};

/**
 * @var {string} MEDIA_BREAKPOINT bootstrap media query breaKpoint who triggers a d=single <Select> column instead separate columns of radio buttons
 */
const MEDIA_BREAKPOINT = 'md';


const timePickerDisabled = function(seconds, scope) {
    if (!seconds) {
        return [];
    }

    const r = [];
    switch (scope) {
        case 'h':
            const h = Math.floor(seconds / 3600);
            for (let i = 0; i < h; i++) {
                r.push(i);
            }
        break;
        case 'm':
            const m = Math.floor(seconds % 3600 / 60);
            for (let i = 0; i < m; i++) {
                r.push(i);
            }
        break;
        case 's':
            const s = Math.floor(seconds % 3600 % 60);
            for (let i = 0; i < s; i++) {
                r.push(i);
            }
        break;
        default:
            // nothing
    }

    return r;
};


const TheadRow = function({ question, expanded }) {

    if(!expanded) {
        return(null);
    }

    return (
        <TableRow>
            <TableCell className="daytimeslider--firstcol"></TableCell>
            <TableCell className="daytimeslider--secondcol"></TableCell>
            <TableCell>
                <div style={ tableStyle }>
                    <div style={ cellStyle(0) }><i className="fas fa-moon"></i></div>
                    <div style={ cellStyle(50) }><i className="fas fa-sun"></i></div>
                    <div style={ cellStyle(100) }><i className="fas fa-moon"></i></div>
                </div>
            </TableCell>
        </TableRow>
    );
};

TheadRow.defaultProps = {
};

TheadRow.propTypes = {
    question: Question.propTypes().isRequired,
    expanded: PropTypes.bool.isRequired,
};

const Row = function({ question, handleChange, value, disabled, minValue, expanded, step }) {

        if(!expanded) {
            return (
                <TableRow hover>
                    <TableCell className="daytimeslider--firstcol">{ question.title }</TableCell>
                    <TableCell>
                        <TimePicker
                            key={ question.id }
                            question={ question }
                            handleChange={ handleChange }
                            disabledHours= { timePickerDisabled(value, 'h') }
                            disabledMinutes= { timePickerDisabled(value, 'm') }
                            disabledHSeconds= { timePickerDisabled(value, 's') }
                        />
                    </TableCell>
                </TableRow>
            );
        }

        return (
            <TableRow hover>
                <TableCell className="daytimeslider--firstcol">{ question.title }</TableCell>
                <TableCell className="daytimeslider--secondcol">{ prettyHours(value) }</TableCell>
                <TableCell>
                    <InputRange
                        minValue={ minValue }
                        maxValue={ DaySeconds }
                        value={ value }
                        onChange={ handleChange }
                        disabled= { disabled }
                        step= { step }
                        formatLabel={value => prettyHours(value)}
                    />
                </TableCell>
            </TableRow>
        );
};

Row.defaultProps = {
    required: false,
    step:  15 * 60,
    minValue: 0,
    disabled: false,
};

Row.propTypes = {
    question: Question.propTypes().isRequired,
    handleChange: PropTypes.func.isRequired,
    expanded: PropTypes.bool.isRequired,
    required: PropTypes.bool,

    // react-input-range props
    value: PropTypes.number.isRequired,
    minValue: PropTypes.number,
    disabled: PropTypes.bool,
    step: PropTypes.number,
};

const prevValue = function(index, values) {
    if(index === 0 ) {
        return null;
    }
    return values[index - 1];
};

const setValues = function(value, index, values) {
    const length = values.length;
    const prev = prevValue(index, values);
    // set current value
    values[index] = (prev && value < prev) ? prev: value;
    for (let i = index; i < length; i += 1) {
        if(values[i] < value) {
            values[i] = value;
        }
    }
    return values;
}

class DaytimeSequence extends Component {

    constructor(props) {
        super(props);

        this.state = {
            values: props.questions.map(() => 0),
        };
    }

    handleChange(value, question, index){
        const values = setValues(value, index, this.state.values);
        this.setState({
            values
        });

        this.props.handleChange(null, question, value);
    }

    render() {
        const { questions, required, expand } = this.props;

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
                        questions.map((question, index) => {
                            const prev = prevValue(index, this.state.values);
                            return (
                                <Row
                                    key={ index }
                                    question={ question }
                                    handleChange={ (value) => this.handleChange(value, question, index) }
                                    value={ this.state.values[index] }
                                    // minValue={ minValue }
                                    disabled={ index > 0 && !prev }
                                    required={ required }
                                    expanded={ expanded }
                                />
                            );
                        })
                    }
                </TableBody>
            </Table>
        );
    }
};

DaytimeSequence.defaultProps = {
    required: true,
    expand: null,
};

DaytimeSequence.propTypes = {
    handleChange: PropTypes.func.isRequired,
    questions: PropTypes.arrayOf(
        Question.propTypes()
    ),
    required: PropTypes.bool,
    expand: PropTypes.bool, // force contracted or expanded display
};

export default DaytimeSequence;
