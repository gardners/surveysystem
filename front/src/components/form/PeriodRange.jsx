import React, { Component } from 'react';
import PropTypes from 'prop-types';

import 'rc-slider/assets/index.css';
import Tooltip from 'rc-tooltip';
import Slider from 'rc-slider';

const d2sec = 86400; // 24 hours
const h2sec = 3600;

const dark = '#212077';
const light = '#ffc107';
const gradient = `linear-gradient(to right, ${dark} 15%, ${light} 30%, ${light} 70%, ${dark} 85%)`;

const tableStyle = {
    display: 'table',
    width: '100%',
    padding: '.5em',
    tableLayout: 'fixed',    /* For cells of equal size */
    background: gradient,
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

const Range = Slider.Range;

const Handle = (props) => {
  const { value, dragging, index, ...restProps } = props;
  return (
    <Tooltip
      prefixCls="rc-slider-tooltip"
      overlay={ prettyHours(value) }
      placement="top"
      key={ index }
      trigger={ [] } /* overwrite */
      visible={ true } /* overwrite */
      defaultVisible={ true }
    >
      <Slider.Handle value={value} {...restProps} />
    </Tooltip>
  );
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

    handleChange(e) {
        const { name, value } = e.target;
        const { question } = this.props;

        const updated = this.state;
        updated[name] = value;

        this.setState({
            seconds: updated,
            //step: (this.state.step < this.state.vals - 1) ? this.state.vals + 1 : this.state.step,
        });
        // TODO validate here?
        this.props.handleChange(updated, question);
    }

    render() {
        const { question, timeBeginLabel, timeEndLabel } = this.props;
        const { time_begin, time_end } = this.state;

        const domain = [0, d2sec];
        return (
            <div className="form-group">
                <div className={ this.props.className } style={ tableStyle }>
                    <div style={ cellStyle(0)}><i className="fas fa-moon text-white"></i></div>
                    <div style={ cellStyle(50) }><i className="fas fa-sun text-white"></i></div>
                    <div style={ cellStyle(100) }><i className="fas fa-moon text-white"></i></div>
                </div>
                <Range
                    handle= { Handle }
                    min={ 0 }
                    max={ d2sec }
                    defaultValue={ [1000, 3000] }
                    allowCross={ false }
                    pushable={ true }

                    activeDotStyle={ {
                        backgroundColor: 'blue',
                    } }
                    //railStyle = {
                    //    railStyle
                    //}
                />
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
