import React, { Component } from 'react';

import { Redirect } from 'react-router-dom';

// apis
import api, { BaseUri } from '../api';
import { serializeQuestionAnswer } from '../serializer';
import SurveyManager from '../SurveyManager';
import LocalStorage from '../storage/LocalStorage';
import Log from '../Log';

import  { createDisplayGroups } from '../DisplayGroups';

import SurveyForm from './survey/SurveyForm';
import QuestionGroup from './survey/QuestionGroup';

// components
import LoadingSpinner from './LoadingSpinner';
import Card from './bootstrap/Card';
import Alert from './Alert';

// devel
import Dev from './Dev';

// config
const CACHE_KEY = process.env.REACT_APP_SURVEYCACHEKEY;

class Survey extends Component {
    constructor(props) {
        super(props);
        const surveyID = props.match.params.id;

        this.state = {
            survey: new SurveyManager(surveyID, BaseUri),
            answers: {}, // TODO consider moving answer map into SurveyManager as well

            loading: '',
            alerts: [],
            // TODO tmp
            // TODO if ready, include resetting in handle responses
            evaluation: null,  // TODO remove
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
            answers: {},
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

    handleChange(element, question, ...values) {
        const { answers } = this.state;
        const { id } = question;

        // Error or csv
        const serialized = serializeQuestionAnswer(element, question, ...values);

        answers[id] = {
            values,
            serialized,
        };

        this.setState({
            answers,
        });
    }

    getFormErrors() {
        const { answers } = this.state;
        return Object.keys(answers).filter(key => answers[key] && typeof answers[key].serialized !== 'undefined' && answers[key].serialized instanceof Error)
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
            answers: {}, // clear answers
            alerts: [], // clear previous alerts
        }))
        .then(() => LocalStorage.set(CACHE_KEY, survey))
        .catch(err => this.alert(err));
    }

    handleUpdateAnswers(e) {
        e && e.preventDefault();

        const { survey, answers } = this.state;
        this.setState({ loading: 'Sending answer...' });

        const errors = this.getFormErrors();
        if(errors.length) {
            Log.error(`handleUpdateAnswers: ${errors.length} errors found`);
            return;
        }

        const csvFragments = Object.keys(answers).map(id => answers[id].serialized);

        // each answer is a single request, we are bundling them into Promise.all
        Promise.all(
            csvFragments.map(fragment => api.updateAnswer(survey.sessionID, fragment))
        )
        .then(responses => responses.pop()) // last
        .then(response => survey.add(response.next_questions))
        .then(() => this.setState({
            loading: '',
            survey,
            alerts: [], // clear previous alerts
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
        .then(responses => responses.pop()) // last ?TODO multi elements mode
        .then(response => survey.add(response.next_questions))
        .then(() => this.setState({
            loading: '',
            survey,
            alerts: [], // clear previous alerts
        }))
        .then(() => LocalStorage.set(CACHE_KEY, survey))
        .catch(err => this.alert(err));
    }

    handleFinishSurvey(e) {
        e && e.preventDefault();

        const { survey } = this.state;
        this.setState({ loading: 'Fetching results...' });

        api.finishSurvey(survey.sessionID)
        .then(evaluation => this.setState({
            loading: '',
            survey,
            answers: {}, // clear previous answers
            alerts: [], // clear previous alerts
            evaluation, // TODO remove
        }))
        .then(() => survey.close())
        .then(() => LocalStorage.delete(CACHE_KEY))
        .catch(err => this.alert(err));
    }

    render() {

        // @see surveysystem/backend/include/survey.h, struct question

        const { survey, answers } = this.state;

        if (survey.isFinished()) { // TODO surveymanager method
            return(
                <Redirect to={ {
                    pathname: `/analyse/${survey.surveyID}`,
                    state: { survey }
                    } }
                />
            );
        }

        const questions = survey.current();
        const errors = this.getFormErrors();
        const groups = createDisplayGroups(questions);

        return (
            <section>
                <Dev.SurveyBar survey = { survey } />
                <h1>{ survey.surveyID } <small>Step { this.state.survey.step }</small></h1>

                <LoadingSpinner loading={ this.state.loading } message={ this.state.loading }/>
                { this.state.alerts.map((entry, index) => <Alert key={ index } severity={ entry.severity } message={ entry.message } />) }

                {
                    /* show if survey is finished but not closed yet */
                    (survey.finished && !survey.closed) ?
                        <Card>
                            <h2> <i className="fas fa-check-circle"></i> Survey completed.</h2>
                            Thank you for your time!
                        </Card>
                    : null
                }

                {
                    /* show if survey is finished but not closed yet */
                    (survey.finished && !survey.closed) ?
                        <Card className="text-white bg-success">
                            <h2> <i className="fas fa-check-circle"></i> Survey completed.</h2>
                            Thank you for your time!
                        </Card>
                    : null
                }

                <SurveyForm
                    show={ !this.state.loading }
                    handlePrev={ this.handleDelAnswer.bind(this) }
                    handleNext={ this.handleUpdateAnswers.bind(this) }
                    handleFinish={ this.handleFinishSurvey.bind(this) }

                    isFirst={ survey.step <= 0 }
                    isFinished={ survey.finished }
                    hasErrors={ errors.length > 0 }
                    hasAnswers={ Object.keys(answers).length > 0 }
                >
                    {
                        groups.map((questionGroup, index) => {
                            return <QuestionGroup
                                key={ index }
                                handleChange={ this.handleChange.bind(this) }
                                questionGroup={ questionGroup }
                                answers={ answers }
                            />
                        })
                    }
                </SurveyForm>

                <Dev.Pretty label="survey" data={ survey } open={ false }/>
                <Dev.Pretty label="questions" data={ questions } open={ false }/>
                <Dev.Pretty label="answers" data={ answers } open={ false }/>
                <Dev.Pretty label="errors" data={ errors } open={ false }/>

            </section>
        );
    }
}

Survey.propTypes = {
};

export default Survey;
