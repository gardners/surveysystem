import React, { Component } from 'react';
import PropTypes from 'prop-types';

import { Link } from 'react-router-dom';

import LocalStorage from '../storage/LocalStorage';
import SurveyList from '../SurveyList';

import ApiAlert from './ApiAlert';
import Modal from './Modal';
import EmbedHtmlFile from './EmbedHtmlFile';

const {
    REACT_APP_SURVEY_CACHEKEY,
    PUBLIC_URL
} = process.env;

const formatDate = function(timestamp) {
    return (timestamp) ? new Date(timestamp).toLocaleString() : 'n/a';
};

/**
 * Renders session action buttons for a survey
 */
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

/**
 * Renders a single survey item
 */

const SurveyItem = function({ survey, session, isCurrent, withButtons }) {

    if (survey.error) {
        return (
            <React.Fragment>
                <h3>{ (survey.name) ?  survey.name : survey.id }</h3>
                <div className="alert alert-danger" role="alert">{ survey.error.toString() }</div>
            </React.Fragment>
        );
    }

    const created = (session && typeof session.created !== 'undefined') ? session.created : 0;
    const modified = (session && typeof session.modified !== 'undefined') ? session.modified : 0;
    const sessionID = (session && typeof session.sessionID !== 'undefined') ? session.sessionID : '';

    return (
        <React.Fragment>
            <h3>{ (survey.name) ?  survey.name : survey.id }</h3>
            { (survey.title) ?  <div className="mb-3"><strong>{ survey.title }</strong></div> : null }
            { (survey.description) ?  <div className="mb-3">{ survey.description }</div> : null }
            { (survey.organisation) ?  <div className="mb-3">Organisation: { survey.organisation }</div> : null }
            <div className="mb-3">
                <ul className="list-unstyled pl-2">

                    { (survey.email) ?  <li><i className="fas fa-envelope ml-2 mr-2"></i><a href={ `mailto: ${survey.email}`} >{ survey.email }</a></li> : null }
                    { (survey.phone) ?  <li><i className="fas fa-phone ml-2 mr-2"></i>{ survey.phone }</li> : null }
                    {
                        survey.pages.map(
                            (page, index) =>
                                <li key={ index }>
                                    <i className="fas fa-external-link-alt ml-2 mr-2"></i>
                                    <Modal
                                        key={ index }
                                        title={ page.title }
                                        buttonClassName="btn btn-link btn-sm p-0"
                                        buttonText={ () => page.title || '' }
                                    >
                                        <EmbedHtmlFile
                                            title={ page.title || '' }
                                            src={ (page.src) ? `${PUBLIC_URL}/surveys/${survey.id}/${page.src}` :  '' }
                                            showTitle={ false }
                                            />
                                    </Modal>
                                </li>
                        )
                    }
                </ul>
            </div>
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

/**
 * Renders a display of all available survey items
 */
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

Surveys.defaultProps = {};

Surveys.propTypes = {};

export { Surveys as default, SurveyItem };
