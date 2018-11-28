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
            <section>
                <h1>{ surveyProvider }</h1>
                <strong>Our Surveys</strong>

                <div className="row">

                    { surveys.map(survey =>
                        <div key={survey} className="col col-sm-6">
                            <div className="card">
                                <div className="card-body">
                                    <h3 className="card-title">{ survey }</h3>
                                    { (cachedSurveyID === survey) ?
                                        <div>
                                            { (cachedTime) ? <p><small>last access: <i>{ new Date(cachedTime).toLocaleString() }</i>, session ID: { cache.sessionID }</small></p> : 'n/a' }
                                            <Link to={ `/survey/${survey}/${cache.sessionID}` } className="btn btn-primary">Continue</Link>
                                            <Link to={ `/survey/${survey}/new` } onClick={ () => LocalStorage.delete('sessionstate') } className="btn btn-default">Restart</Link>
                                        </div>
                                        :
                                    <Link to={ `/survey/${survey}` } className="btn btn-primary">Start</Link> }
                                </div>
                            </div>
                        </div>
                    ) }
                </div>

            </section>
        );
    }
}

Surveys.propTypes = {};

export default Surveys;
