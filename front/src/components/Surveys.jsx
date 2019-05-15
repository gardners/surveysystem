import React from 'react';
import PropTypes from 'prop-types';

import { Link } from 'react-router-dom';

import LocalStorage from '../storage/LocalStorage';

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
        <React.Fragment>
            <h1>{ surveyProvider } <small>Surveys</small></h1>

            <div className="row card-deck">
                { surveys.map((survey, index) =>
                    <div className="card" key={ index }>
                        <div className="card-header">
                            <h2 className="card-title">{ survey }</h2>
                        </div>
                        <div className="card-body">
                            {
                                (cachedSurveyID === survey) ?
                                    <React.Fragment>
                                        <p className="card-text">
                                            <small>session ID: { cache.sessionID }</small><br/>
                                            <small>created: { formatDate(created) }</small><br/>
                                            <small>last access: { formatDate(modified) }</small>
                                        </p>
                                    </React.Fragment>
                                :
                                    <p className="card-text">
                                        <small>Start survey</small>
                                    </p>
                            }
                        </div>
                        <div className="card-footer">
                            {
                                (cachedSurveyID === survey) ?
                                    <React.Fragment>
                                        <Link to={ `/survey/${survey}/${cache.sessionID}` } className="btn btn-primary">Continue</Link>
                                        <Link to={ `/survey/${survey}/new` } onClick={ () => LocalStorage.delete(CACHE_KEY) } className="btn btn-s">Restart</Link>
                                    </React.Fragment>
                                :
                                    <Link to={ `/survey/${survey}` } className="btn btn-primary">Start</Link>
                            }
                        </div>
                    </div>
                ) }
            </div>
        </React.Fragment>
    );

};

Surveys.propTypes = {
    surveys: PropTypes.arrayOf(PropTypes.string).isRequired,
    surveyProvider: PropTypes.string.isRequired,
};

export default Surveys;
