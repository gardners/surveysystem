import React from "react";
import PropTypes from 'prop-types';

import Slide from "@material-ui/core/Slide";
import useScrollTrigger from '@material-ui/core/useScrollTrigger';

const HideOnScroll = function(props) {
    const { children, threshold, ...other } = props;
    const trigger = useScrollTrigger({ threshold });

    return (
        <Slide appear={ false } direction="down" in={ !trigger } { ...other }>
        { children }
        </Slide>
    );
}

HideOnScroll.defaultProps = {
    threshold: 150,
};

HideOnScroll.propTypes = {
    threshold: PropTypes.number
};

export default HideOnScroll;
