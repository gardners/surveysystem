import React, { Component } from 'react';
import PropTypes from 'prop-types';
import { Link } from 'react-router-dom';

// apis
import api from '../api';
import Log from '../Log';
import { normalizeAnalysis } from '../Analysis';

import ApiAlert from './ApiAlert';

// components
import Toggle from './Toggle';
import Preloader from './Preloader';

// devel
import Dev from './Dev';

const tableStyle = {
    tableLayout: 'fixed',    /* For cells of equal size */
};

const cellStyle = {
    width: '25%',
};


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
        // demo
        if (this.props.evaluation) {
            const { evaluation } = this.props;
            this.setState({
                evaluation,
                loading: '',
                alerts: [],
            });
            return;
        }

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

        Log.add(message, severity);
        alerts.push(message);

        this.setState({
            loading: '',
            alerts,
        });
    }

    render() {
        const surveyID = this.props.match.params.id;
        const sessionID = this.props.match.params.sessionID;
        const { evaluation, loading, alerts } = this.state;

        if(alerts.length) {
            return (
                <section>
                     <h1>Analysis</h1>
                     Unfortunately we encountered an error retrieving your data.
                    { this.state.alerts.map((entry, index) => <ApiAlert key={ index } message={ entry } />) }
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
                    <Preloader loading={ loading } message={ loading }/>
                </section>
            );
        }

        const data = normalizeAnalysis(this.state.evaluation);
        const { category, classification, rank, recommendation, riskRating } = data;
        const { condition, subcondition, mainText, learnMore, mainRecommendation, mandatoryTips, additionalInsights }  = data.displayResults.sleepConditions;

        return (
            <section>
                <Preloader loading={ loading } message={ loading }/>
                <h1>Analysis</h1>
                <h2>Survey: <span className="text-success">{ surveyID }</span></h2>

                <div className="table-responsive-md">
                    <table className="table table-striped table-hover table-sm" style={ tableStyle }>
                        <tbody>
                            <tr>
                                <th style={ cellStyle }>category</th>
                                <td>{ category }</td>
                            </tr>
                            <tr>
                                <th style={ cellStyle }>classification</th>
                                <td>{ classification }</td>
                            </tr>
                            <tr>
                                <th style={ cellStyle }>rank</th>
                                <td>{ rank }</td>
                            </tr>
                            <tr>
                                <th style={ cellStyle }>risk rating</th>
                                <td>{ riskRating }</td>
                            </tr>
                            <tr>
                                <th style={ cellStyle }>recommendation</th>
                                <td>{ recommendation }</td>
                            </tr>
                        </tbody>
                    </table>
                </div>

                <h3>Condition</h3>

                <div className="table-responsive-md">
                    <table className="table table-striped table-hover table-sm" style={ tableStyle }>
                        <tbody>
                            <tr>
                                <th style={ cellStyle }>condition</th>
                                <td>{ condition }</td>
                            </tr>
                            <tr>
                                <th style={ cellStyle }>sub condition</th>
                                <td>{ subcondition }</td>
                            </tr>
                        </tbody>
                    </table>
                </div>

                <h3>Details</h3>

                <p>{ mainText }</p>
                <ul className="list-group list-group-flush">
                    <Toggle className="list-group-item">Learn More { learnMore }</Toggle>
                    <Toggle className="list-group-item">Main Recommendation { mainRecommendation }</Toggle>
                    <Toggle className="list-group-item">Tips { mandatoryTips }</Toggle>
                    <Toggle className="list-group-item">Additional insights { additionalInsights }</Toggle>
                </ul>

                <Dev.Pretty label="raw analyis" data={ evaluation } open={ false }/>
            </section>
        );
    }
}

Analysis.defaultProps = {
    evaluation: null,
};

Analysis.propTypes = {
    evaluation: PropTypes.object,
};

export default Analysis;
