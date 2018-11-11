import React from 'react';
import Loader from 'react-loader-spinner'

const LoadingSpinner = () => (

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

export default LoadingSpinner;