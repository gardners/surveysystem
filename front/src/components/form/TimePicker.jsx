import React from 'react';
import PropTypes from 'prop-types';

import moment from 'moment';

import RcTimePicker from 'rc-time-picker';
import 'rc-time-picker/assets/index.css';

const format = 'h:mm a';

const seconds = function(m) {
    const midnight = m.clone().startOf('day');
    return  m.diff(midnight, 'seconds');
};

const TimePicker = function(props) {

    const { question, handleChange } = props;
    return (
        <div className="form-group">
            <label htmlFor={ question.id }>{ question.title_text }</label>

            <RcTimePicker
                id={ question.id }
                name={ question.name }
                showSecond={ false }
                defaultValue={ moment(0) }
                className={ props.className }
                onChange={ (m) => {
                    const value = seconds(m);
                    handleChange(question, value);
                } }
                format={ format }
                use12Hours
                inputReadOnly
            />
        </div>
    );

};

TimePicker.defaultProps = {
    placeholder: null,
    // rc-time-picker props
    minuteStep: 10,
};

TimePicker.propTypes = {
    handleChange: PropTypes.func.isRequired,
    withButton: PropTypes.bool,
    question: PropTypes.shape({
        id: PropTypes.string.isRequired,
        name: PropTypes.string.isRequired,
        title: PropTypes.string.isRequired,
        title_text: PropTypes.string.isRequired,
        type: PropTypes.string.isRequired,
    }).isRequired,
    placeholder: PropTypes.string,
    className: PropTypes.string,

    // rc-time-picker props
    minuteStep: PropTypes.number,
};

export default TimePicker;
