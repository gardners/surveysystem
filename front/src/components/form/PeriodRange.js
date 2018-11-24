import React, { Component } from 'react';
import PropTypes from 'prop-types';

const max = 86400; // 24 hours

const computeStyles = function() {

};

/**
 * range sliders for defining period values (seconds) within 24 hours
 */
class PeriodRange extends Component {
    constructor(props) {
        super(props);
        this.state = {
            value: {
                time_begin: 0,
                time_end: 0,
            },
        };
    }

    handleChange(e) {
        const { name, value } = e.target;
        const { question } = this.props;

        const updated = this.state.value;
        updated[name] = value;

        this.setState({
            value: updated,
        });
        // TODO validate here?
        this.props.handleChange(updated, question);
    }

    render() {
        const { question, timeBeginLabel, timeEndLabel } = this.props;
        const { time_begin, time_end } = this.state.value;

        return (
            <div className="form-group">
                <div id={ question.id } className="range">
                    <label>
                        <input
                            type="range"
                            name="time_begin"
                            id="time_begin"
                            min={ 0 }
                            max={ (time_end)?  time_end : max }
                            onChange={ this.handleChange.bind(this)}
                            />
                        { timeBeginLabel }
                    </label>
                </div>

                <div className="range">
                    <label>
                        <input
                            type="range"
                            name="time_end"
                            id="time_end"
                            min={ time_begin }
                            max={ max }
                            onChange={ this.handleChange.bind(this)}
                            />
                        { timeEndLabel }
                    </label>
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
