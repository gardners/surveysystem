import React, { Component } from 'react';
import { Link } from 'react-router-dom';

// apis
import api from '../api';
import Log from '../Log';
import { DirtyJson, camelToNormal } from '../Utils';
import Alert from './Alert';

// components
import Feedback from './analysis/Feedback';
import Evaluation from './analysis/Evaluation';

import LoadingSpinner from './LoadingSpinner';
// TODO Alert

// devel
import Dev from './Dev';

class Analysis extends Component {

    constructor(props) {
        super(props);
        this.state = {
            evaluation: null,
            loading: '',
            alerts: [],
        }
    }

    componentDidMount() {
        const sessionID = this.props.match.params.sessionID;

        this.setState({
            loading: 'Fetching evaluation...',
        });

        api.getAnalysis(sessionID)
        .then(evaluation => this.setState({
            evaluation,
            loading: '',
            alerts: [],
        }))
        .catch(err => this.alert(err)); //TODO
    }

    /**
     * Sends a message or an Error to this.state.alerts
     * This method ist tpically used by the request api in case of response or network errors
     * @param {Error} error
     * @returns {void}
     */
    alert(message, severity) {
        const { alerts } = this.state;

        if(severity !== 'error' && message instanceof Error) {
            severity = 'error';
        }

        const entry = Log.add(message, severity);
        alerts.push(entry);

        this.setState({
            loading: '',
            alerts,
        });
    }

    render() {
        const surveyID = this.props.match.params.id;
        const sessionID = this.props.match.params.sessionID;
        const { evaluation, loading, alerts } = this.state;

        const report = DirtyJson.get(evaluation, 'report', {});
        const evaluations = DirtyJson.get(report, 'evaluations', {});

        if(alerts.length) {
            return (
                <section>
                     <h1>Analysis</h1>
                     Unfortunately we encountered an error retrieving your data.
                    { this.state.alerts.map((entry, index) => <Alert key={ index } severity={ entry.severity } message={ entry.message } />) }
                     <button onClick={ () => window.location.reload() } className="btn btn-secondary">Reload</button>&nbsp;
                     <Link to={ `/survey/${surveyID}` } className="btn bn-lg btn-secondary">Back to survey</Link>
                </section>
            );
        }

        if(!evaluation && !loading) {
            return (
                <section>
                    <h1>{ sessionID }: Analysis</h1>
                    <div className="text-danger">
                        <p><i className="fas fa-exclamation-circle"></i> This survey is not finished yet!</p>
                        <Link to={ `/survey/${surveyID}` } className="btn btn-lg btn-primary">Continue survey</Link>
                    </div>
                </section>
            );
        }

        if(loading) {
            return (
                <section>
                    <LoadingSpinner loading={ loading } message={ loading }/>
                </section>
            );
        }

        return (
            <section>
                <LoadingSpinner loading={ loading } message={ loading }/>
                <h1>Analysis <small>{ sessionID } </small></h1>

                <div className="jumbotron jumbotron-fluid">
                    <div className="container">
                        <h2 className="display-4">Feedback</h2>
                        <Feedback feedback={ evaluation.feedback } />
                    </div>
                </div>

                <h2>Evaluation</h2>
                { Object.keys(evaluations).map((key) => <Evaluation key={ key } name={ camelToNormal(key) /* TODO redundant */} evaluation={ evaluations[key] } />) }
                <hr />

                <Dev.Pretty label="raw analyis" data={ evaluation } open={ false }/>
            </section>
        );
    }
}

Analysis.propTypes = {
};

export default Analysis;
