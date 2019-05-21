import React from 'react';
import PropTypes from 'prop-types';

import Typography from '@material-ui/core/Typography';
import Box from '@material-ui/core/Box';

const Footer = function({ surveyProvider }) {
    return (
         <Box component="footer" p={ 2 }>
            <Typography align="center">
            { `© ${surveyProvider} ${new Date().getFullYear()}` }
            </Typography>
        </Box>
    );
}

Footer.propTypes = {
    surveyProvider: PropTypes.string.isRequired,
};

export default Footer;
