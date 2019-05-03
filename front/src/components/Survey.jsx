import React, { Component } from 'react';

import { Link } from 'react-router-dom';

// apis
import api, { BaseUri } from '../api';
import { serializeQuestionAnswer } from '../serializer';
import SurveyManager from '../SurveyManager';
import  { mapQuestionGroups } from '../Question';
import  { isArray } from '../Utils';

import LocalStorage from '../storage/LocalStorage';
import Log from '../Log';

// form components
import SurveyForm from './survey/SurveyForm';
import Question from './survey/Question';
import QuestionGroup from './survey/QuestionGroup';

// misc components
import LoadingSpinner from './LoadingSpinner';
import Card from './bootstrap/Card';
import Alert from './Alert';


// devel
import Dev from './Dev';

// config
const CACHE_KEY = process.env.REACT_APP_SURVEY_CACHEKEY;

class Survey extends Component {
    constructor(props) {
        super(props);
        const surveyID = props.match.params.id;

        this.state = {
            survey: new SurveyManager(surveyID, BaseUri),
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
            answers: {},
        });
        this.initNextQuestion();
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

    initNextQuestion(e) {
        e && e.preventDefault();

        const { survey } = this.state;
        this.setState({ loading: 'Initializing survey...' });

        api.nextQuestion(survey.sessionID)
        .then(response => survey.add(response.next_questions))
        .then(() => this.setState({
            loading: '',
            survey,
            answers: {}, // clear answers
            alerts: [], // clear previous alerts
        }))
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

        api.updateAnswers_SEQUENTIAL(survey.sessionID, csvFragments)
        .then(responses => responses.pop()) // last
        .then(response => survey.add(response.next_questions))
        .then(() => this.setState({
            loading: '',
            survey,
            alerts: [], // clear previous alerts
            answers: {}, // clear previous answers
        }))
        .then(() => LocalStorage.set(CACHE_KEY, survey))
        .catch(err => this.alert(err));
    }

    handleDelAnswer(e) {
        e && e.preventDefault();

        const { survey } = this.state;
        if(survey.isClosed()) {
            Log.warn('Trying to delete answer on a finished survey');
            return;
        }
        this.setState({ loading: 'Fetching previous question...' });

        survey.reset();
        const inversed = survey.currentInversed();
        const questionIds = inversed.map(question => question.id);

        api.deleteAnswers_SEQUENTIAL(survey.sessionID, questionIds)
        .then(responses => responses.pop()) // last ?TODO multi elements mode
        .then(response => survey.add(response.next_questions))
        .then(() => this.setState({
            loading: '',
            survey,
            alerts: [], // clear previous alerts
            answers: {}, // clear previous answers
        }))
        .then(() => LocalStorage.set(CACHE_KEY, survey))
        .catch(err => this.alert(err));
    }

    render() {

        // @see surveysystem/backend/include/survey.h, struct question

        const { survey, answers } = this.state;

        const questions = survey.current();
        const withGroups = mapQuestionGroups(questions);
        const errors = this.getFormErrors();

        const questionCount = Object.keys(questions).length;
        const answerCount = Object.keys(answers).length;
        const errorCount = Object.keys(errors).length;

        const hasQuestions = questionCount > 0;
        const hasErrors = errorCount > 0;
        const hasAnswers = answerCount > 0 ;
        const hasAllAnswers = (answerCount === questions.length);
        const didAnswerBefore = (survey.steps > 0);
        const isClosed = survey.isClosed();

        return (
            <section>
                <Dev.SurveyBar survey = { survey } />
                <h1>{ survey.surveyID }</h1>

                <LoadingSpinner loading={ this.state.loading } message={ this.state.loading }/>
                { this.state.alerts.map((entry, index) => <Alert key={ index } severity={ entry.severity } message={ entry.message } />) }

                {
                    /* show if survey is finished but not closed yet */
                    (isClosed) ?
                        <Card>
                            <h2> <i className="fas fa-check-circle"></i> Survey completed.</h2>
                            <p>Thank you for your time!</p>
                            <Link className="btn btn-default btn-primary" to={ `/analyse/${survey.surveyID}/${survey.sessionID}` }>Finish Survey</Link>
                        </Card>
                    : null
                }

                <SurveyForm
                    show={ !isClosed && questionCount > 0 && !this.state.loading }
                    handlePrev={ this.handleDelAnswer.bind(this) }
                    handleNext={ this.handleUpdateAnswers.bind(this) }

                    hasQuestions={ hasQuestions }
                    hasErrors={ hasErrors }
                    hasAnswers={ hasAnswers }
                    hasAllAnswers={ hasAllAnswers }
                    didAnswerBefore= { didAnswerBefore }

                    className="list-group"
                    rowClassName="list-group-item"
                >
                {
                    withGroups.map((entry, index) => {

                        if(isArray(entry)) {
                            return (
                                <div key={ index } className="list-group-item">
                                    <QuestionGroup
                                        handleChange={ this.handleChange.bind(this) }
                                        questions={ entry }
                                        answers={ this.state.answers }
                                    />
                                </div>
                            );
                        }

                        return (
                            <div key={ index } className="list-group-item">
                                <Question
                                    handleChange={ this.handleChange.bind(this) }
                                    question={ entry }
                                    answer={ this.state.answers[entry.id] || null }
                                />
                            </div>
                        );

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
