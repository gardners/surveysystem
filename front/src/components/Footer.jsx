import React from 'react';
import PropTypes from 'prop-types';

const Footer = function({ surveyProvider }) {
    return (
        <footer>
            <div className="text-center">
            { `© ${surveyProvider} ${new Date().getFullYear()}` }
            </div>
        </footer>
    );
}

Footer.propTypes = {
    surveyProvider: PropTypes.string.isRequired,
};

export default Footer;
