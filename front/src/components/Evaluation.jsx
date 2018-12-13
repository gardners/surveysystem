import React, { Component } from 'react';
import { Link } from 'react-router-dom';

// apis
import api, { BaseUri } from '../api';
import Log from '../Log';

// components
import Feedback from './evaluation/Feedback';
import LoadingSpinner from './LoadingSpinner';
import Alert from './Alert';

// devel
import Dev from './Dev';

class Evaluation extends Component {

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

        api.getEvaluation(surveyID)
        .then(evaluation => this.setState({
            evaluation,
            loading: '',
        }))
    }

    render() {
        const surveyID = this.props.match.params.id;
        const { evaluation, loading } = this.state;


        if(!evaluation && !loading) {
            return (
                <section>
                    <h1>{ surveyID }: Evaluation</h1>
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
                <h1>{ surveyID } <small>Evaluation</small></h1>
                <div className="card">
                    <div className="card-body">
                        <h2 className="card-title">Feedback</h2>
                        <p className="card-text">Some quick example text to build on the card title and make up the bulk of the card's content.</p>
                    </div>
                    <Feedback feedback={ evaluation.feedback } />
                </div>
                <Dev label="survey" data={ evaluation } open={ true }/>
            </section>
        );
    }
}

Evaluation.propTypes = {
};

export default Evaluation;
