import React from 'react';
import PropTypes from 'prop-types';
import Box from '@material-ui/core/Box';
import Typography from '@material-ui/core/Typography';
import LinearProgress from '@material-ui/core/LinearProgress';

import './preloader.scss';

const Preloader = function ({ loading, message, color }) {
    if (!loading) {
        return (null);
    }

    return (
        <Box display="block">
            <LinearProgress />
            <Typography variant="caption" align="center" display="block" style={ { marginTop: '0.5em' } }>
                { message || 'Processing your answer...' }
            </Typography>
        </Box>
    );
};

Preloader.defaultProps = {
    loading: false,
    message: 'Processing your answer...',
};

Preloader.propTypes = {
    loading: PropTypes.oneOfType([
        PropTypes.bool,
        PropTypes.string,
    ]),
    message: PropTypes.string,
    color: PropTypes.string,
};

export default Preloader;
