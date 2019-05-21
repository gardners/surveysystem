import React from 'react';
import { Link as RouterLink } from 'react-router-dom';
import MuiLink from '@material-ui/core/Link';

/**
 * Wraps React Fouter <Link> component into Mui <Link> component
 * @see https://next.material-ui.com/components/links/#third-party-routing-library
 */
const Link = function(props) {
    return (
        <MuiLink component={ RouterLink } { ...props } />
    );
};

Link.propTypes = {};
export default Link;
