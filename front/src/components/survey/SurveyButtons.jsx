import React from 'react';
import PropTypes from 'prop-types';

import Box from '@material-ui/core/Box';
import Grid from '@material-ui/core/Grid';
import Fab from '@material-ui/core/Fab';
import CheckCircleIcon from '@material-ui/icons/CheckCircle';
import BackspaceIcon from '@material-ui/icons/Backspace';
import WarningIcon from '@material-ui/icons/Warning';

const HasErrors = function({ hasErrors, hasAllAnswers, hasAnswers }) {
    if (!hasAnswers) {
        return null;
    }

    return (
        <React.Fragment>
            { hasErrors && <Box color="red" p={ 1 }> <WarningIcon/> Please fix errors above</Box> }
            { !hasAllAnswers && <Box color="orange" p={ 1 }><WarningIcon/> Please answer all questions</Box> }
        </React.Fragment>
    );
};

HasErrors.propTypes = {
    hasErrors: PropTypes.bool.isRequired,
    hasAllAnswers: PropTypes.bool.isRequired,
    hasAnswers: PropTypes.bool.isRequired,
};

const canNext = function ({ hasErrors, hasAnswers, hasAllAnswers }) {
    return (!hasErrors && hasAnswers && hasAllAnswers);
};

/**
 * Render Previous/Next/Finish buttos
 * The component should not contain survey logic or handle complex data. It merely recieves a number of flags from the parent component
 */
const SurveyButtons = function(props) {

    return (
        <Box className={ props.className }>
            <Grid
                className={ props.className }
                justify="space-between" // Add it here :)
                container
                spacing={ 2 }
                direction="row"
                alignItems="center"
            >
                <Grid item xs={ 12 }>
                    <HasErrors
                        hasErrors={ props.hasErrors }
                        hasAllAnswers={ props.hasAllAnswers }
                        hasAnswers={ props.hasAnswers }
                    />
                </Grid>

                <Grid item xs={ 8 }>
                    <Fab
                        variant="extended"
                        color="primary"
                        aria-label="Next Question"
                        disabled={ !canNext(props) }
                        onClick={ props.handleNext }
                        size="large"
                    >
                        Next Question
                        <CheckCircleIcon style={ { marginLeft: '.5rem' } } />
                    </Fab>
                </Grid>

                <Grid item xs={ 3 }>
                    <Fab
                        variant="extended"
                        aria-label="Previous Question"
                        onClick={ props.handlePrev }
                        size="small"
                    >
                        <BackspaceIcon style={ { marginRight: '.5rem' } } />
                        Back
                    </Fab>
                </Grid>
            </Grid>
        </Box>
    );
};

SurveyButtons.defaultProps = {
};

SurveyButtons.propTypes = {
    handleNext: PropTypes.func.isRequired,
    handlePrev: PropTypes.func.isRequired,

    hasQuestions: PropTypes.bool.isRequired,
    hasErrors: PropTypes.bool.isRequired,
    hasAnswers: PropTypes.bool.isRequired,
    hasAllAnswers: PropTypes.bool.isRequired,
    didAnswerBefore: PropTypes.bool.isRequired,
};

export default SurveyButtons;
