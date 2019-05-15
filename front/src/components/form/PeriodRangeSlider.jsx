import React, { Component } from 'react';
import PropTypes from 'prop-types';

import InputRange from 'react-input-range';
import './PeriodRangeSlider.scss';

import Question from '../../Question';
import { prettyHours, DaySeconds } from '../../Utils';

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

        return (
            <div className="periodrangeslider" style={ wrapperStyle }>
                <div className={ this.props.className } style={ tableStyle }>
                    <div style={ cellStyle(0)}><i className="fas fa-moon text-white"></i></div>
                    <div style={ cellStyle(50) }><i className="fas fa-sun text-white"></i></div>
                    <div style={ cellStyle(100) }><i className="fas fa-moon text-white"></i></div>
                </div>
                <InputRange
                    minValue={ 0 }
                    maxValue={ DaySeconds }
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
};

PeriodRangeSlider.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: Question.propTypes().isRequired,

    required: PropTypes.bool,
};

export default PeriodRangeSlider;
