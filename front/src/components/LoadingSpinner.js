import React from 'react';
import PropTypes from 'prop-types';

import '../style/spinners.css';

const LoadingSpinner = function (props) {
    const loading = props.loading || false;
    console.log('jb',props.message);
    if (!loading) {
        return (null);
    }

    return (
        <div className="loader">
            <div className="spinner">
                <div className="double-bounce1" style={{ backgroundColor: props.color, }}></div>
                <div className="double-bounce2" style={{ backgroundColor: props.color, }}></div>
                <div className="message" style={{ color: props.color, }}> { props.message || 'Processing your answer...' } </div>
            </div>
        </div>
    );
};

LoadingSpinner.defaultProps = {
    loading: false,
    message: 'Processing your answer...',
    color: '#337ab7', // bootstrap-primary
};

LoadingSpinner.propTypes = {
    loading: PropTypes.oneOfType([
        PropTypes.bool,
        PropTypes.string,
    ]),
    message: PropTypes.string,
    spinnerClass: PropTypes.string,
    messageClass: PropTypes.string,
};

export default LoadingSpinner;
