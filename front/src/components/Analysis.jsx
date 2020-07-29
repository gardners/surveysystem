import React, { Component } from 'react';
import { Link } from 'react-router-dom';

// apis
import Api from '../Api';
import ApiAlert from './ApiAlert';
import { normalizeAnalysis } from '../Analysis';

// components
import AnalysisMeta from './analysis/AnalysisMeta';
import { EvaluationGroup } from './analysis/Evaluation';
import Preloader from './Preloader';

// devel
import Dev from './Dev';

class Analysis extends Component {

    constructor(props) {
        super(props);
        this.state = {
            analysis: null,
            loading: '',
            alerts: [],
        }
    }

    componentDidMount() {
        const sessionID = this.props.match.params.sessionID;

        this.setState({
            loading: 'Fetching analysis...',
        });

        Api.getAnalysis(sessionID)
        .then(result => normalizeAnalysis(result))
        .then(analysis => this.setState({
            analysis,
            loading: '',
            alerts: [],
        }))
        .catch(err => this.alert(err));
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
        alerts.push(message);

        this.setState({
            loading: '',
            alerts,
        });
    }

    render() {
        const surveyID = this.props.match.params.id;
        const sessionID = this.props.match.params.sessionID;
        const { analysis, loading, alerts } = this.state;
        const evaluations = (analysis) ? analysis.evaluations : [];

        if(alerts.length) {
            return (
                <section>
                     <h1>Analysis</h1>
                     Unfortunately we encountered an error retrieving your data.
                    { this.state.alerts.map((entry, index) => <ApiAlert key={ index } error={ entry } />) }
                     <button onClick={ () => window.location.reload() } className="btn btn-secondary">Reload</button>&nbsp;
                     <Link to={ `/survey/${surveyID}` } className="btn bn-lg btn-secondary">Back to survey</Link>
                </section>
            );
        }

        if(!evaluations.length && !loading) {
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
                    <Preloader loading={ loading } message={ loading }/>
                </section>
            );
        }

        return (
            <section>
                <Preloader loading={ loading } message={ loading }/>
                <h1>Analysis</h1>

                <AnalysisMeta surveyID={ surveyID } sessionID={ sessionID } analysis={ analysis } />
                <EvaluationGroup evaluations={ evaluations } />
                <Dev.Pretty label="raw analyis" data={ evaluations } open={ false }/>
            </section>
        );
    }
}

Analysis.defaultProps = {};

Analysis.propTypes = {};

export default Analysis;
