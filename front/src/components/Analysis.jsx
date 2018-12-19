import React, { Component } from 'react';
import { Link } from 'react-router-dom';

// apis
import api from '../api';
import Log from '../Log';
import { DirtyJson, camelToNormal } from '../Utils';

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
        }
    }

    componentDidMount() {
        const surveyID = this.props.match.params.id;

        this.setState({
            loading: 'Fetching evaluation...',
        });

        api.getAnalysis(surveyID)
        .then(evaluation => this.setState({
            evaluation,
            loading: '',
        }))
        .catch(err => Log.error(err)); //TODO
    }

    render() {
        const surveyID = this.props.match.params.id;
        const { evaluation, loading } = this.state;

        const report = DirtyJson.get(evaluation, 'report', {});
        const evaluations = DirtyJson.get(report, 'evaluations', {});

        if(!evaluation && !loading) {
            return (
                <section>
                    <h1>{ surveyID }: Analysis</h1>
                    <div className="text-danger">
                        <p><i className="fas fa-exclamation-circle"></i> This survey is not finished yet!</p>
                        <Link to={ `/survey/${surveyID}` } className="btn bn-lg btn-primary">Continue survey</Link>
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
                <h1>{ surveyID } <small>Analysis</small></h1>

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
