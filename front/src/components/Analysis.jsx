import React, { Component } from 'react';
import { Link } from 'react-router-dom';

// apis
import Api from '../Api';
import ApiAlert from './ApiAlert';
import { normalizeAnalysis } from '../Analysis';
import Session, { load_cached_session, save_cached_session } from '../Session';

// components
import Preloader from './Preloader';
import AnalysisMeta from './analysis/AnalysisMeta';
import AnalysisBody from './analysis/AnalysisBody';

// devel
import Dev from './Dev';


class Analysis extends Component {

    constructor(props) {
        super(props);
        const survey_id = props.match.params.id;

        this.state = {
            session: new Session(survey_id, null),
            analysis: null,
            loading: '',
            alerts: [],
        }
    }

    componentDidMount() {
        const { history, match } = this.props;

        const survey_id = match.params.survey_id;
        const session = load_cached_session();

        if(!session) {
            history.push(`/survey/${survey_id}`);
            return;
        }

        this.setState({
            loading: 'Fetching analysis...',
        });

        Api.getAnalysis(session.session_id)
        .then((result) => {
            // close session and save, so we can revisit analysis until we open a new session
            session.close();
            save_cached_session(session);
            return normalizeAnalysis(result);
        })
        .then(analysis => this.setState({
            session,
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
        const { session, analysis, loading, alerts } = this.state;
        const { survey_id } = session; // from router, for generating error links

        if(alerts.length) {
            return (
                <section>
                     <h1>Analysis</h1>
                     Unfortunately we encountered an error retrieving your data.
                    { this.state.alerts.map((entry, index) => <ApiAlert key={ index } error={ entry } />) }
                     <button onClick={ () => window.location.reload() } className="btn btn-secondary">Reload</button>&nbsp;
                     <Link to={ `/survey/${survey_id}` } className="btn bn-lg btn-secondary">Back to survey</Link>
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
                <h1>Analysis</h1>
                <AnalysisMeta analysis={ analysis } />
                <AnalysisBody analysis={ analysis } />

                <Dev.IfDebug>
                    <Dev.Pretty label="raw analyis" data={ analysis } open={ false }/>
                </Dev.IfDebug>
            </section>
        );
    }
}

Analysis.defaultProps = {};

Analysis.propTypes = {};

export default Analysis;
