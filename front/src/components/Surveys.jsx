import React, { Component } from 'react';
import PropTypes from 'prop-types';

import { Link } from 'react-router-dom';

import LocalStorage from '../storage/LocalStorage';
import SurveyList from '../SurveyList';
import ApiAlert from './ApiAlert';

const { REACT_APP_SURVEY_CACHEKEY } = process.env;

const formatDate = function(timestamp) {
    return (timestamp) ? new Date(timestamp).toLocaleString() : 'n/a';
};

const SurveyItemButtons = function({ survey, session, isCurrent }) {
    return (
        <React.Fragment>
        {
                (session) ?
                    <React.Fragment>
                        <Link to={ `/survey/${survey.id}/${session.sessionID}` } className="btn btn-lg btn-primary">Continue Survey</Link>
                        <Link to={ `/survey/${survey.id}/new` } onClick={ () => LocalStorage.delete(REACT_APP_SURVEY_CACHEKEY) } className="btn btn-sm btn-link">Restart Survey</Link>
                    </React.Fragment>
                :
                    <Link to={ `/survey/${survey.id}` } className="btn btn-lg btn-primary">Start Survey</Link>
        }
        </React.Fragment>
    );
}

SurveyItemButtons.defaultProps = {
    session: null,
    isCurrent: false,
};

SurveyItemButtons.propTypes = {
    survey: SurveyList.itemPropTypes().isRequired,
    session: PropTypes.object,
    isCurrent: PropTypes.bool,
};


const SurveyItem = function({ survey, session, isCurrent, withButtons }) {

    const created = (session && typeof session.created !== 'undefined') ? session.created : 0;
    const modified = (session && typeof session.modified !== 'undefined') ? session.modified : 0;
    const sessionID = (session && typeof session.sessionID !== 'undefined') ? session.sessionID : '';

    return (
        <React.Fragment>
            <h3>{ (survey.name) ?  survey.name : survey.id }</h3>
            { (survey.title) ?  <p><strong>{ survey.title }</strong></p> : null }
            { (survey.description) ?  <p>{ survey.description }</p> : null }
            <p>
                { (survey.organisation) ?  <React.Fragment>{ survey.organisation }<br/></React.Fragment> : null }
                { (survey.email) ?  <React.Fragment><i className="fas fa-envelope mr-4"></i><a href={ `mailto: ${survey.email}`} >{ survey.email }</a><br/></React.Fragment> : null }
                { (survey.phone) ?  <React.Fragment><i className="fas fa-phone mr-4"></i>{ survey.phone }</React.Fragment> : null }
            </p>
            <table className="mb-3" style={ { fontSize: '.8em' } }>
                <tbody>
                    { (sessionID) ?  <tr><td className="pr-3">sessionID:</td><td>{ sessionID }</td></tr> : null }
                    { (created) ?  <tr><td className="pr-3">started:</td><td>{ formatDate(created) }</td></tr> : null }
                    { (modified) ?  <tr><td className="pr-3">last access:</td><td>{ formatDate(modified) }</td></tr> : null }
                </tbody>
            </table>
            {
                (withButtons) ?
                    <div className="mb-3">
                        <SurveyItemButtons survey={ survey } session={ session } />
                    </div>
                : null
            }
        </React.Fragment>
    );
};

SurveyItem.defaultProps = {
    session: null,
    isCurrent: false,
    withButtons: true,
};

SurveyItem.propTypes = {
    survey: SurveyList.itemPropTypes().isRequired,
    session: PropTypes.object,
    isCurrent: PropTypes.bool,
    withButtons: PropTypes.bool,
};


class Surveys extends Component {

    constructor(props) {
        super(props);
        this.state = {
            surveys: [],
            error: null,
        }
    }

    componentDidMount() {
        const { surveyIds } = this.props;
        SurveyList.getAll(surveyIds)
        .then(surveys => this.setState({ surveys }))
        .catch(error => this.setState({ error }));
    }

    render () {
        const { surveys, error } = this.state;

        const currentSession = LocalStorage.get(REACT_APP_SURVEY_CACHEKEY);
        let currentSurvey = null;
        const availableSurveys = [];

        surveys.forEach(survey => {
            if (currentSession && currentSession.surveyID === survey.id) {
                currentSurvey = survey;
                return;
            }
            availableSurveys.push(survey);
        });

        return (
            <React.Fragment>
                <h1>Our Surveys</h1>
                { error && <ApiAlert error ={ error } /> }
                {
                    (currentSurvey) ?
                        <React.Fragment>
                            <div className="mb-3">Your current survey:</div>
                            <ul className="list-group mb-3">
                                <li className="list-group-item list-group-item-primary">
                                    <SurveyItem survey={ currentSurvey } session={ currentSession } isCurrent={ true } />
                                </li>
                            </ul>
                        </React.Fragment>
                    : null
                }
                {
                    (availableSurveys.length) ?
                        <React.Fragment>
                            <div className="mb-3">Available Surveys:</div>
                            <ul className="list-group mb-3">
                            {
                                availableSurveys.map((survey, index) =>
                                    <li key={ index } className="list-group-item">
                                        <SurveyItem survey={ survey } />
                                    </li>
                                )
                            }
                            </ul>
                        </React.Fragment>
                    : null
                }
            </React.Fragment>
        );
    }
};

Surveys.defaultProps = {
    surveyIds: [],
};

Surveys.propTypes = {
    surveyIds: PropTypes.arrayOf(
        PropTypes.string,
    ),
};

export { Surveys as default, SurveyItem };
