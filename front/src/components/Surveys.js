import React, { Component } from 'react';
import { Link } from 'react-router-dom';

import { Configuration } from '../conf/config';
import LocalStorage from '../storage/LocalStorage';

class Surveys extends Component {

    render() {
        const { surveyProvider, surveys } = Configuration;
        const cache = LocalStorage.get('sessionstate');
        const cachedSurveyID = (cache) ? cache.surveyID : '';
        const cachedTime = (cache) ? cache.time : '';

        return (
            <div className="container-fluid">
            <h1>{ surveyProvider }</h1>
            <p>Survey front page. For tests, try /survey/:id with a valid id to display the survey</p>
            <h2>Our Surveys</h2>

            { surveys.map(survey =>
                <div key={survey} className="panel panel-default">
                    <div className="panel-body">
                        <h3>{ survey }</h3>
                        { (cachedSurveyID === survey) ?
                            <div>
                                { (cachedTime) ? <p><small>last access: <i>{ new Date(cachedTime).toLocaleString() }</i>, session ID: { cache.sessionID }</small></p> : 'n/a' }
                                <Link to={ `/survey/${survey}/${cache.sessionID}` } className="btn btn-primary">Continue</Link>
                                <Link to={ `/survey/${survey}/new` } className="btn btn-default">Restart</Link>
                            </div>
                            :
                        <Link to={ `/survey/${survey}` } className="btn btn-primary">Start</Link> }
                    </div>
                </div>
            ) }
            </div>
        );
    }
}

Surveys.propTypes = {};

export default Surveys;
