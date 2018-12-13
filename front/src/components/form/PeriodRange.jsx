import React, { Component } from 'react';
import PropTypes from 'prop-types';

import InputRange from 'react-input-range';
import './PeriodRange.scss';

const d2sec = 86400; // 24 hours
const h2sec = 3600;

const dark = '#212077';
const light = '#ffc107';
const gradient = `linear-gradient(to right, ${dark} 15%, ${light} 30%, ${light} 70%, ${dark} 85%)`;

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
class PeriodRange extends Component {
    constructor(props) {
        super(props);
        this.state = {
            time_begin: 0,
            time_end: 0,
        };
    }

    handleChange(value) {
        const { question } = this.props;
        const updated = this.state;
        updated['time_begin'] = value.min;
        updated['time_end'] = value.max;

        this.setState(updated);

        // TODO validate here?
        this.props.handleChange(updated, question);
    }

    render() {
        const { question, timeBeginLabel, timeEndLabel } = this.props;
        const { time_begin, time_end } = this.state;

        const domain = [0, d2sec];
        return (
            <div className="form-group">
            <div style={ wrapperStyle }>
                <div className={ this.props.className } style={ tableStyle }>
                    <div style={ cellStyle(0)}><i className="fas fa-moon text-white"></i></div>
                    <div style={ cellStyle(50) }><i className="fas fa-sun text-white"></i></div>
                    <div style={ cellStyle(100) }><i className="fas fa-moon text-white"></i></div>
                </div>
                <InputRange
                    minValue={ 0 }
                    maxValue={ d2sec }
                    defaultValue={ [1000, 3000] }
                    value={ { min: time_begin, max: time_end } }
                    onChange={ this.handleChange.bind(this) }
                    formatLabel={ value => prettyHours(value) }
                />
            </div>
            </div>
        );
    }
}

PeriodRange.defaultProps = {
    placeholder: null,
    timeBeginLabel: 'Start',
    timeEndLabel: 'Finished',
};

PeriodRange.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: PropTypes.shape({
        id: PropTypes.string.isRequired,
        name: PropTypes.string.isRequired,
        title: PropTypes.string.isRequired,
        title_text: PropTypes.string.isRequired,
        type: PropTypes.string.isRequired,

        timeBeginLabel: PropTypes.string,
        timeEndLabel: PropTypes.string,
    }).isRequired,
    placeholder: PropTypes.string,
};

export default PeriodRange;
