import React from 'react';
import PropTypes from 'prop-types';

const version = process.env.REACT_APP_VERSION  || '';

const Footer = function({ surveyProvider }) {
    return (
        <footer className="row">
            <div className="col text-center p-3">
            { `© ${surveyProvider} ${new Date().getFullYear()}` }{ version && <small className="text-muted ml-2">v{ version }</small> }
            </div>
        </footer>
    );
}

Footer.propTypes = {
    surveyProvider: PropTypes.string.isRequired,
};

export default Footer;
