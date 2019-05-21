import React from 'react';
import PropTypes from 'prop-types';

import Button from '@material-ui/core/Button';
import Typography from '@material-ui/core/Typography';
import Card from '@material-ui/core/Card';
import CardHeader from '@material-ui/core/CardHeader';
import CardActions from '@material-ui/core/CardActions';
import CardContent from '@material-ui/core/CardContent';

import { Link } from 'react-router-dom';

const SurveyCompleted = function({ surveyID, sessionID }) {
    return (
        <Card>
            <CardHeader
                title={ surveyID }
            />
            <CardContent>
                <Typography variant="h2" gutterBottom>Survey completed.</Typography>
                <Typography className="card-text">Thank you for your time!</Typography>
            </CardContent>
            <CardActions>
                <Button variant="contained" color="primary" component={ Link }  to={ `/analyse/${surveyID}/${sessionID}` }>Finish Survey</Button>
            </CardActions>
        </Card>

    );
};

SurveyCompleted.propTypes = {
    surveyID: PropTypes.string.isRequired,
    sessionID: PropTypes.string.isRequired,
};

export default SurveyCompleted;
