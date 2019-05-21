import React from 'react';
import PropTypes from 'prop-types';
import { withStyles } from '@material-ui/core/styles';
import Box from '@material-ui/core/Box';
import Paper from '@material-ui/core/Paper';

const styles = theme => ({
    paper: {
        padding: theme.spacing(2, 2),
    },
});

function Section(props) {
    const { classes, noPadding, noPaper, children } = props;
    return (
        <Box mb={ 2 }>
            {
                (!noPaper) ?
                    <Paper className={ (!noPadding) ? classes.paper : null }>
                        { children }
                    </Paper>
                :
                    children
            }
        </Box>
    );
}

Section.defaultProps = {
    noPadding: false,
    noPaper: false,
};

Section.propTypes = {
    noPadding: PropTypes.bool,
    noPaper: PropTypes.bool,
};

export default withStyles(styles)(Section);
