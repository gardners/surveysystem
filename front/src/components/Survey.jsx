import React, { Component } from 'react';

import { Link } from 'react-router-dom';

// apis
import api, { BaseUri } from '../api';
import Answer from '../Answer';

import SurveyManager from '../SurveyManager';
import  { mapQuestionGroups } from '../Question';

import  { isArray } from '../Utils';

import LocalStorage from '../storage/LocalStorage';

// form components
import SurveyForm from './survey/SurveyForm';
import Question from './survey/Question';
import QuestionGroup from './survey/QuestionGroup';
import FeedbackItem from './survey/FeedbackItem';
import SurveyButtons from './survey/SurveyButtons';

// misc components
import Preloader from './Preloader';
import ApiAlert from './ApiAlert';

// devel
import Dev from './Dev';

/**
 * Create a map of default answers from an array of coerced  questions (see normalizeQuestions())
 * @returns {object} a map of answers with default values
 */

const getDefaultAnswers = function(coercedQuestions) {
    const answers = {};
    coercedQuestions.forEach(question => {
        if (question.default_value) {
            const answerObj = Answer.create(question);
            const serialized = Answer.serialize(answerObj);
            if(serialized instanceof Error) {
                return;
            }
            answers[question.id] = serialized;
        }
    });
    return answers;
};

const EXT = window.SS || {};
const getInstantFeedback = EXT.getInstantFeedback || function() { return []; };

// config
const CACHE_KEY = process.env.REACT_APP_SURVEY_CACHEKEY;

class Survey extends Component {
    constructor(props) {
        super(props);
        const surveyID = props.match.params.id;

        this.state = {
            survey: new SurveyManager(surveyID, BaseUri),
            loading: '',
            answers: {},
            errors: {},
            alerts: [],
            feedback: [],
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
        alerts.push(message);

        this.setState({
            loading: '',
            alerts,
        });
    }

    ////
    // form processing
    ////

    handleChange(element, question, value) {
        const { survey, questions, answers, errors } = this.state;
        const { id } = question;

        delete(errors[id]);

        // @see https://developer.mozilla.org/en-US/docs/Web/API/ValidityState
        if(element && typeof element.validity !== 'undefined') {
            if (!element.validity.valid) {
                errors[id] = new Error (element.validationMessage);
                this.setState({
                    errors,
                });
                return;
            }
        }

        const answer = Answer.setValue(question, value);
        if (answer instanceof Error) {
            errors[id] = answer;
            this.setState({
                errors,
            });
            return;
        }

        const serialized = Answer.serialize(answer);// can be instanceof Error
        if (serialized instanceof Error) {
            errors[id] = answer;
            this.setState({
                errors,
            });
            return;
        }

        // set answer
        answers[id] = serialized;

        // fetch optional custom feedback for this screen

        const feedback = getInstantFeedback(survey.surveyID, question.id, questions, answers);

        this.setState({
            errors,
            answers,
            feedback,
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
            answers: getDefaultAnswers(survey.current()),
            errors: {}, // clear errors
            alerts: [], // clear alerts
            feedback: [], // clear feedback
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
            answers: getDefaultAnswers(survey.current()),
            errors: {}, // clear errors
            alerts: [], // clear alerts
            feedback: [], // clear feedback
        }))
        .catch(err => this.alert(err));
    }

    handleUpdateAnswers(e) {
        e && e.preventDefault();

        const { survey, answers, errors } = this.state;
        this.setState({ loading: 'Sending answer...' });

        if(Object.keys(errors).length) {
            this.alert('Send answers: Errors found!', 'error');
            return;
        }

        const questions = survey.current();
        const answerIds = Object.keys(answers);

        if(questions.length !== answerIds.length) {
            this.alert('Send answers: Missing answers!', 'error');
            return;
        }

        const csvFragments = answerIds.map(id => answers[id]);

        api.updateAnswers_SEQUENTIAL(survey.sessionID, csvFragments)
        .then(responses => responses.pop()) // last
        .then(response => survey.add(response.next_questions))
        .then(() => this.setState({
            loading: '',
            survey,
            answers: getDefaultAnswers(survey.current()),
            errors: {}, // clear errors
            alerts: [], // clear alerts
            feedback: [], // clear feedback
        }))
        .then(() => LocalStorage.set(CACHE_KEY, survey))
        .catch(err => this.alert(err));
    }

    handleDelAnswer(e) {
        e && e.preventDefault();

        const { survey } = this.state;
        if(survey.isClosed()) {
            this.alert('Trying to delete answer on a finished survey', 'error');
            return;
        }

        survey.reset();
        const inversed = survey.currentInversed();
        if(!inversed.length) {
            this.alert('No question found to delete', 'error');
            return;
        }
        const questionId = inversed[0].id;
        this.setState({ loading: `Deleting ${inversed.length} answers...` });

        api.deleteAnswerAndFollowing(survey.sessionID, questionId)
        .then(response => survey.add(response.next_questions))
        .then(() => this.setState({
            loading: '',
            survey,
            answers: getDefaultAnswers(survey.current()),
            errors: {}, // clear errors
            alerts: [], // clear alerts
            feedback: [], // clear feedback
        }))
        .then(() => LocalStorage.set(CACHE_KEY, survey))
        .catch(err => this.alert(err));
    }

    render() {
        // @see surveysystem/backend/include/survey.h, struct question
        const { survey, errors, answers, feedback } = this.state;

        const questions = survey.current();
        const withGroups = mapQuestionGroups(questions);

        const answersCount = Object.keys(answers).length;
        const errorCount = Object.keys(errors).length;

        const hasQuestions = questions.length > 0;
        const hasErrors = errorCount > 0;
        const hasAnswers = answersCount > 0 ;
        const hasAllAnswers = (answersCount === questions.length);
        const didAnswerBefore = survey.questions.length > 1;
        const isClosed = survey.isClosed();

        return (
            <section>
                <Dev.SurveyBar survey = { survey } />
                <h1>{ survey.surveyID }</h1>

                <Preloader loading={ this.state.loading } message={ this.state.loading }/>
                {
                    (this.state.alerts.length) ?
                        <React.Fragment>
                            Unfortunately we encountered an error sending your data.
                            { this.state.alerts.map((entry, index) => <ApiAlert key={ index } message={ entry } />) }
                        </React.Fragment> : null
                }

                {
                    /* show if survey is finished but not closed yet */
                    (isClosed) ?
                        <div className="card">
                            <div className="card-header">
                                <h2 className="card-title"> <i className="fas fa-check-circle"></i> Survey completed.</h2>
                            </div>
                            <div className="card-body">

                                <p className="card-text">Thank you for your time!</p>
                                <Link className="btn btn-default btn-primary" to={ `/analyse/${survey.surveyID}/${survey.sessionID}` }>Finish Survey</Link>
                            </div>
                        </div>
                    : null
                }

                <SurveyForm
                    show={ !isClosed && questions.length > 0 && !this.state.loading }
                    className="list-group"
                >
                    {
                        withGroups.map((entry, index) => {

                            if(isArray(entry)) {
                                return (
                                    <QuestionGroup
                                        key={ index }
                                        className="list-group-item"

                                        handleChange={ this.handleChange.bind(this) }
                                        questions={ entry }
                                        errors={ errors }
                                    />
                                );
                            }

                            return (
                                <Question
                                    key={ index }
                                    className="list-group-item"

                                    handleChange={ this.handleChange.bind(this) }
                                    question={ entry }
                                    error={ errors[entry.id] || null }
                                />
                            );

                        })
                    }

                    {
                        feedback.map((item, index) => <FeedbackItem key={ index } status={ item.status } className="list-group-item" classNamePrefix="list-group-item-">{ item.message }</FeedbackItem>)
                    }

                    {
                        hasErrors && <FeedbackItem className="list-group-item list-group-item-danger">Please fix errors above</FeedbackItem>
                    }

                    {
                        !hasAllAnswers && <FeedbackItem className="list-group-item list-group-item-warning">Please answer all questions</FeedbackItem>
                    }

                    <SurveyButtons
                        className="list-group-item"
                        handlePrev={ this.handleDelAnswer.bind(this) }
                        handleNext={ this.handleUpdateAnswers.bind(this) }

                        hasQuestions={ hasQuestions }
                        hasErrors={ hasErrors }
                        hasAnswers={ hasAnswers }
                        hasAllAnswers={ hasAllAnswers }
                        didAnswerBefore={ didAnswerBefore }
                    />
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
