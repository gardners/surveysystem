import React, { Component } from 'react';
import PropTypes from 'prop-types';

// apis
import api from '../api';
import { serializeAnswer } from '../payload-serializer';
import SurveyManager from '../SurveyManager';
import LocalStorage from '../storage/LocalStorage';
import Log from '../Log';

import TextInput from './form/TextInput';
import RadioGroup from './form/RadioGroup';
import GeoLocation from './form/GeoLocation';
import PeriodRange from './form/PeriodRange';

// components
import LoadingSpinner from './LoadingSpinner';
import Alert from './Alert';

// devel
import Dev from './Dev';

// config
const CACHE_KEY = process.env.REACT_APP_SURVEYCACHEKEY;

const Panel = function(props) {
    const pStyle = props.panelStyle || 'default';
    const glyphicon = props.glyphicon || '';
    return(
        <div className={ `panel panel-${pStyle}` }>
            { props.heading && <div className="panel-heading">
                { glyphicon && <span className={ `glyphicon glyphicon-${glyphicon}` } style={ { marginRight: '1em' } }></span>}
                { props.heading }
            </div> }
            <div className="panel-body">{ props.children }</div>
        </div>
    );
};

Panel.propTypes = {
    panelStyle: PropTypes.string,
    heading:  PropTypes.string,
    glyphicon:  PropTypes.string,
};

class Survey extends Component {
    constructor(props) {
        super(props);
        const surveyID = props.match.params.id;

        this.state = {
            survey: new SurveyManager(surveyID),
            answers: {}, // TODO consider moving answer map into SurveyManager as well

            loading: '',
            alerts: [],
        };
    }

    componentDidMount() {
        const { survey } = this.state;

        const cached = LocalStorage.get(CACHE_KEY);

        if(!survey.canMerge(cached)) {
            this.initNewSession();
            return;
        }

        survey.merge(cached);
        this.setState({
            loading: '',
            survey,
            answers: [],
        });
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

    ////
    // form processing
    ////

    handleChange(answer, question) {
        const { answers } = this.state;
        const { id } = question;
        this.setState({
            answers: Object.assign(answers, {
                [id]: {
                    answer,
                    question,
                },
            })
        });
    }

    ////
    // form submissions
    ////

    initNewSession(e) {
        e && e.preventDefault();

        const { survey } = this.state;
        this.setState({ loading: 'Initializing survey...' });

        api.createNewSession(survey.surveyID)
        .then(sessID => survey.init(sessID))
        .then(() => api.nextQuestion(survey.sessionID))
        .then(response => survey.add(response.next_questions))
        .then(() => this.setState({
            loading: '',
            survey,
            answers: [],
        }))
        .then(() => LocalStorage.set(CACHE_KEY, survey))
        .catch(err => this.alert(err));
    }

    handleUpdateAnswers(e) {
        e && e.preventDefault();

        const { survey, answers } = this.state;
        this.setState({ loading: 'Sending answer...' });

        //TODO check if errors returned from serializer
        const csvFragments = Object.keys(answers).map((id) => {
            const entry = answers[id];
            const { answer, question } = entry;
            const type = question.type;
            return serializeAnswer(id, answer, type);
        });

        // send current answer(s)
        // loading while the questions are sent and the next question is not displayed
        // each answer is a single request, we are bundling them into Promise.all
        Promise.all(
            Object.keys(csvFragments).map(id => api.updateAnswer(survey.sessionID, csvFragments[id]))
        )
        .then(responses => responses.pop()) // last
        .then(response => survey.add(response.next_questions))
        .then(() => this.setState({
            loading: '',
            survey,
            answers: {}, // TODO
        }))
        .then(() => LocalStorage.set(CACHE_KEY, survey))
        .catch(err => this.alert(err));
    }

    handleDelAnswer(e) {
        e && e.preventDefault();

        const { survey } = this.state;
        this.setState({ loading: 'Fetching previous question...' });

        // FIRST go back to previous (does not render) and then get questionIds
        survey.back();
        const questionIds = survey.current().map(question => question.id);

        // delete current answer(s)
        // each answer is a single request, we are bundling them into Promise.all
        Promise.all(
            questionIds.map(id => api.deleteAnswer(survey.sessionID, id))
        )
        .then(responses => responses.pop()) // last
        .then(response => survey.add(response.next_questions))
        .then(() => this.setState({
            loading: '',
            survey,
            answers: {},
        }))
        .then(() => LocalStorage.set(CACHE_KEY, survey))
        .catch(err => this.alert(err));
    }

    handleFinishSurvey(e) {
        e && e.preventDefault();

        const { survey } = this.state;
        this.setState({ loading: 'Fetching previous question...' });

        Promise.resolve('TODO ANALYTICS')
        .then(() => this.setState({
            loading: '',
            survey,
        }))
        .then(() => LocalStorage.delete(CACHE_KEY))
        .catch(err => this.alert(err));
    }

    render() {
        const { survey, answers } = this.state;
        const questions = survey.current();


        return (
            <section className="container">
                <pre>step: { this.state.survey.step }, session: { this.state.survey.sessionID }, env: { process.env.NODE_ENV }</pre>
                <h1>{ survey.surveyID } <small>Step { this.state.survey.step }</small></h1>

                <LoadingSpinner loading={ this.state.loading } message={ this.state.loading }/>

                { survey.finished && <Panel panelStyle="primary" heading="Survey completed." glyphicon="ok">Thank you for your time!</Panel> }

                <form id={ Date.now() /*trickout autofill*/ }>
                    <input type="hidden" value={ Date.now() /*trick autofill*/ } />

                    { this.state.alerts.map((entry, index) => <Alert key={ index } severity={ entry.severity } message={ entry.message } />) }

                    {
                        questions.map((question, index) => {
                            switch(question.type) {
                                case 'radiogroup':
                                    return <Panel key={ index } heading={ question.name }>
                                    <RadioGroup
                                        question={ question }
                                        handleChange={ this.handleChange.bind(this) } />
                                    </Panel>

                                case 'geolocation':
                                    return <Panel key={ index } heading={ question.name }>
                                    <GeoLocation
                                        question={ question }
                                        handleChange={ this.handleChange.bind(this) } />
                                    </Panel>

                                case 'datetime':
                                    return <Panel key={ index } heading={ question.name }>
                                    <PeriodRange
                                        question={ question }
                                        handleChange={ this.handleChange.bind(this) } />
                                    </Panel>

                                default:
                                    return  <Panel key={ index } heading={ question.name }>
                                    <TextInput
                                        question={ question }
                                        handleChange={ this.handleChange.bind(this) } />
                                    </Panel>

                            }

                        })
                    }

                    { (survey.step <= 0 && questions.length) ?
                        <div className="well">
                            <button type="submit" className="btn btn-default btn-primary btn-arrow-right"
                                onClick={ this.handleUpdateAnswers.bind(this) }>Next Question</button>
                        </div>
                    : null }

                    { (!survey.finished && survey.step > 0 && questions.length) ?
                        <div className="well">
                            <button type="submit" className="btn btn-default btn-arrow-left"
                                onClick={ this.handleDelAnswer.bind(this) }>Previous Question</button>
                            <button type="submit" className="btn btn-default btn-primary btn-arrow-right"
                                onClick={ this.handleUpdateAnswers.bind(this) }>Next Question</button>
                        </div>
                    : null }

                    { (survey.finished) ?
                        <div className="well">
                            <button type="submit" className="btn btn-default btn-primary"
                                onClick={ this.handleFinishSurvey.bind(this) }>Finish Survey</button>
                        </div>
                    : null }

                </form>

                <Dev label="survey" data={ survey } open={ true }/>
                <Dev label="questions" data={ questions } open={ true }/>
                <Dev label="answers" data={ answers } open={ true }/>

            </section>
        );
    }
}

Survey.propTypes = {
};

export default Survey;
