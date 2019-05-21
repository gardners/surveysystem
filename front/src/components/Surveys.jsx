import React from 'react';
import PropTypes from 'prop-types';

import { Link } from 'react-router-dom';
import Button from '@material-ui/core/Button';
import Typography from '@material-ui/core/Typography';
import Grid from '@material-ui/core/Grid';
import Card from '@material-ui/core/Card';
import CardHeader from '@material-ui/core/CardHeader';
import CardActions from '@material-ui/core/CardActions';
import CardContent from '@material-ui/core/CardContent';

import LocalStorage from '../storage/LocalStorage';

import Content from './Content';
import Section from './Section';

const CACHE_KEY = process.env.REACT_APP_SURVEY_CACHEKEY;

const formatDate = function(timestamp) {
    return (timestamp) ? new Date(timestamp).toLocaleString() : 'n/a';
};

const Surveys = function({ surveyProvider, surveys }) {

    const cache = LocalStorage.get(CACHE_KEY);
    const cachedSurveyID = (cache) ? cache.surveyID : '';
    let created = (cache && typeof cache.created !== 'undefined') ? cache.created : 0;
    let modified = (cache && typeof cache.modified !== 'undefined') ? cache.modified : 0;

    return (
        <Content title="Surveys">
            <Section noPaper>
                <Grid container spacing={4}>
                    { surveys.map((survey, index) =>
                        <Grid item key={ index } xs={ 12 } md={ 6 }>
                            <Card>
                                <CardHeader
                                    title={ survey }
                                />
                                <CardContent>
                                    {
                                        (cachedSurveyID === survey) ?

                                                <Typography variant="body2">
                                                    session ID: { cache.sessionID }<br/>
                                                    created: { formatDate(created) }<br/>
                                                    last access: { formatDate(modified) }
                                                </Typography>
                                            : null
                                    }
                                </CardContent>
                                <CardActions>
                                    {
                                        (cachedSurveyID === survey) ?
                                                <React.Fragment>
                                                    <Button variant="contained" color="primary" component={ Link } to={ `/survey/${survey}/${cache.sessionID}` }>Continue Survey</Button>
                                                    <Button variant="contained" component={ Link } to={ `/survey/${survey}/new` }>Start again</Button>
                                                </React.Fragment>
                                                :
                                                <Button variant="contained" color="primary" component={ Link } to={ `/survey/${survey}/new` }>Start Survey</Button>
                                    }
                                </CardActions>
                            </Card>
                        </Grid>
                    ) }
                </Grid>
            </Section>
        </Content>
    );

};

Surveys.propTypes = {
    surveys: PropTypes.arrayOf(PropTypes.string).isRequired,
    surveyProvider: PropTypes.string.isRequired,
};

export default Surveys;
