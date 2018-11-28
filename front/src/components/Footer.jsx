import React from 'react';
import { Configuration } from '../conf/config';

const Footer = function() {
    return (
        <footer>
            <div className="text-center">
            { `© ${ Configuration.surveyProvider } ${new Date().getFullYear()}` }
            </div>
        </footer>
    );
}

Footer.propTypes = {};

export default Footer;
