import React from 'react';

const {
    REACT_APP_SURVEY_PROVIDER,
    REACT_APP_VERSION,
} = process.env;

const Footer = function() {
    const version = REACT_APP_VERSION || '';
    return (
        <footer className="container">
            <div className="row">
                <div className="col text-center p-3">
                { `© ${REACT_APP_SURVEY_PROVIDER} ${new Date().getFullYear()}` }
                <br />
                { version && <small className="text-muted">v{ version || '' }</small> }<small className="text-muted ml-2">({ process.env.NODE_ENV })</small>
                </div>
            </div>
        </footer>
    );
}

export default Footer;
