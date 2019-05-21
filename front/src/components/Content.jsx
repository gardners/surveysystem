import React from 'react';
import PropTypes from 'prop-types';

import Typography from '@material-ui/core/Typography';
import Box from '@material-ui/core/Box';

const renders = function(prop) {
    if(typeof prop === 'function') {
        return prop();
    }
    return prop;
}

const Content = function({ title, subTitle, prepend, append, children }) {
    return (
        <React.Fragment>
            <Typography variant="h1" gutterBottom>{ renders(title) }</Typography>
            { subTitle && <Typography variant="h2" gutterBottom>{ renders(subTitle) }</Typography> }
            { prepend && <Box mb={ 2 }>{ renders(prepend) }</Box> }
            {
                children
            }
            { append && <Box mb={ 2 }>{ renders(append) }</Box> }
        </React.Fragment>
    );
};

Content.defaultProps = {
    subTitle: '',
    prepend: '',
    append: '',
};

Content.propTypes = {
    title: PropTypes.oneOfType([
        PropTypes.string,
        PropTypes.func
    ]).isRequired,
    subTitle: PropTypes.oneOfType([
        PropTypes.string,
        PropTypes.func
    ]),
    prepend: PropTypes.oneOfType([
        PropTypes.string,
        PropTypes.func
    ]),
    append: PropTypes.oneOfType([
        PropTypes.string,
        PropTypes.func
    ]),
};

export default Content;
