import React from 'react';
import Loader from 'react-loader-spinner'
import PropTypes from 'prop-types';

const LoadingSpinner = function (props) {
    const loading = props.loading || false;

    if (!loading) {
        return (null);
    }

    return (
        <div className="mx-auto w-50 p-3 text-center">

            <span><Loader
                type="Puff"
                color="#00BFFF"
                height="100"
                width="100"
            /></span>
            <div> Processing your answer...</div>
        </div>
    );
};

LoadingSpinner.propTypes = {
    loading:  PropTypes.bool,
};

export default LoadingSpinner;
