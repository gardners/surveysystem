import React, { Component } from 'react';
import PropTypes from 'prop-types';

import InputRange from 'react-input-range';
import './PeriodRangeSlider.scss';

import Question from '../../Question';

const DaySec = 86400; // 24 hours

// bg gradient
const dark = '#212077';
const light = '#ffc107';
const gradient = `linear-gradient(to right, ${dark} 15%, ${light} 30%, ${light} 70%, ${dark} 85%)`;

// styles
const wrapperStyle = {
    background: gradient,
    padding: '1rem',
    borderRadius: '1rem',
};

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
        fontSize: '2em',
    };
};

const prettyHours = function(sec) {
    let hours   = Math.floor(sec / 3600);
    let minutes = Math.floor((sec - (hours * 3600)) / 60);
    let seconds = sec - (hours * 3600) - (minutes * 60);

    if (hours   < 10) {hours   = "0" + hours;}
    let t = 'am';
    if (hours > 12) {
        hours -= 12;
        t = 'pm';
    }
    if (minutes < 10) {minutes = "0" + minutes;}
    if (seconds < 10) {seconds = "0" + seconds;}
    return `${hours}:${minutes} ${t}`;
};


/**
 * range sliders for defining period secondss (seconds) within 24 hours
 */
class PeriodRangeSlider extends Component {
    constructor(props) {
        super(props);
        this.state = {
            value: {
                min:  9 * 3600,
                max: 20 * 3600
            }
        };
    }

    handleChange(value) {
        const { question } = this.props;

        this.setState({
            value
        });

        this.props.handleChange(null, question, value.min, value.max, value.max - value.min);
    }

    render() {
        const { value } = this.state;
        const { question } = this.props;

        return (
            <div style={ wrapperStyle }>
                <div className={ this.props.className } style={ tableStyle }>
                    <div style={ cellStyle(0)}><i className="fas fa-moon text-white"></i></div>
                    <div style={ cellStyle(50) }><i className="fas fa-sun text-white"></i></div>
                    <div style={ cellStyle(100) }><i className="fas fa-moon text-white"></i></div>
                </div>
                <label> { question.title } { question.unit && <em>({ question.unit })</em> }</label>
                <InputRange
                    minValue={ 0 }
                    maxValue={ DaySec }
                    value={ value }
                    onChange={ this.handleChange.bind(this) }
                    formatLabel={ val => prettyHours(val) }
                />
            </div>
        );
    }
}

PeriodRangeSlider.defaultProps = {
    required: true,
    timeBeginLabel: 'Start',
    timeEndLabel: 'Finished',
};

PeriodRangeSlider.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: Question.propTypes().isRequired,

    required: PropTypes.bool,
    timeBeginLabel: PropTypes.string,
    timeEndLabel: PropTypes.string,
};

export default PeriodRangeSlider;
