import React, { Component } from 'react';

// data
import Api, { BaseUri } from '../Api';
import  { mapQuestionGroups } from '../Question';
import  { isArray } from '../Utils';
import LocalStorage from '../storage/LocalStorage';
import Answer from '../Answer';
import SurveyManager from '../SurveyManager';

// context
import { SurveyContext } from '../Context';

// form components
import SurveySection from './survey/SurveySection';
import SurveyForm from './survey/SurveyForm';
import Question from './survey/Question';
import QuestionGroup from './survey/QuestionGroup';
import FeedbackItem from './survey/FeedbackItem';
import SurveyButtons from './survey/SurveyButtons';
import SurveyMessage from './survey/SurveyMessage';
import SurveyProgress from './survey/SurveyProgress';
import FinishSurvey from './survey/FinishSurvey';

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
        };
    }

    /**
     * Sends a message or an Error to this.state.alerts
     * This method ist tpically used by the request api in case of response or network errors
     * @param {Error} error
     * @returns {void}
     */
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
        }, () => this.initNextQuestion());
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

    /**
     * Callback for question form elements,
     * - receive,
     * - validate
     * - serialize answer,
     * - store serialized answer in this.state.answers
     *
     * @param {HTMLElement|null} element
     * @param {Question} question
     * @param {mixed} value
     * @returns {void}
     */
    handleChange(element, question, value) {
        const { answers, errors } = this.state;
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

        this.setState({
            errors,
            answers,
        });

    }

    ////
    // form submissions
    ////

    /**
     * Create a new session
     *
     * @param {Event}
     * @returns {void}
     */
    initNewSession(e) {
        e && e.preventDefault();

        const { survey } = this.state;
        this.setState({ loading: 'Initializing survey...' });

        // 1. request a new session id
        // 2. initialize a new survey insatance
        // 3. fetch the first question set andd add it to survey instance

        Api.createNewSession(survey.surveyID)
        .then(sessID => survey.init(sessID))
        .then(() => Api.nextQuestion(survey.sessionID))
        .then((response) => {
            const { status, message, progress, next_questions } = response;
            return survey.add(next_questions, status, message, progress)
        })
        .then(() => this.setState({
            loading: '',
            survey,
            answers: getDefaultAnswers(survey.current()),
            errors: {}, // clear errors
            alerts: [], // clear alerts
        }))
        .then(() => LocalStorage.set(CACHE_KEY, survey))
        .catch(err => this.alert(err));
    }

    /**
     * Initialize a previously cached session
     *
     * @param {Event}
     * @returns {void}
     */
    initNextQuestion(e) {
        e && e.preventDefault();

        const { survey } = this.state;
        this.setState({ loading: 'Initializing survey...' });

        // 1. fetch the current question set, providing a previoiusly cached sessionid
        // 2. add question set to survey instance, (survey.add() will internally evaluate if the new question set matches the old one - based on ids - and skip adding in thast case)

        Api.nextQuestion(survey.sessionID)
        .then((response) => {
            const { status, message, progress, next_questions } = response;
            return survey.add(next_questions, status, message, progress)
        })
        .then(() => this.setState({
            loading: '',
            survey,
            answers: getDefaultAnswers(survey.current()),
            errors: {}, // clear errors
            alerts: [], // clear alerts
        }))
        .catch(err => this.alert(err));
    }

    /**
     * Submit answers
     *
     * @param {Event}
     * @returns {void}
     */
    handleUpdateAnswers(e) {
        e && e.preventDefault();

        const { survey, answers, errors } = this.state;
        this.setState({ loading: 'Sending answer...' });

        if(Object.keys(errors).length) {
            this.alert('Send answers: Errors found!', 'error');
            return;
        }

        const questions = survey.current();

        // create an ordered sequence of answer ID based on the current questions array
        const answerIds = [];
        questions.forEach((q) => {
            answerIds.push(q.id);
        })

        if(answerIds.length !== Object.keys(answers).length) {
            this.alert('Send answers: Missing answers!', 'error');
            return;
        }

        // 1. collect answers csv fragment strings from state
        // 2. send answers (step by step)
        // 3. the last next_questions[] response will be the new survey form state, add it to survey instance

        const csvFragments = answerIds.map(id => answers[id]);

        Api.updateAnswers_SEQUENTIAL(survey.sessionID, csvFragments)
        .then(responses => responses.pop()) // last
        .then((response) => {
            const { status, message, progress, next_questions } = response;
            return survey.add(next_questions, status, message, progress)
        })
        .then(() => this.setState({
            loading: '',
            survey,
            answers: getDefaultAnswers(survey.current()),
            errors: {}, // clear errors
            alerts: [], // clear alerts
        }))
        .then(() => LocalStorage.set(CACHE_KEY, survey))
        .catch(err => this.alert(err));
    }

    /**
     * Delete last answers, fetch updated question set (now with default values!) and set this as the current question set
     *
     * @param {Event}
     * @returns {void}
     */
    handleDelAnswer(e) {
        e && e.preventDefault();

        const { survey } = this.state;
        // #379 removed isClosed condition for session

        // 1. set survey to last step a, this sets survey.current() to the question set received before
        // 2. request deletion of all answers until, and including the foiirst question of the last step
        // 3. replace survey.current() with received questions (includes default_values)

        survey.reset();
        const prev = survey.current();
        if(!prev.length) {
            this.alert('No question found to delete', 'error');
            return;
        }
        const questionId = prev[0].id;
        this.setState({ loading: `Deleting ${prev.length} answers...` });

        Api.deleteAnswerAndFollowing(survey.sessionID, questionId)
        .then((response) => {
            const { status, message, progress, next_questions } = response;
            return survey.replaceCurrent(next_questions, status, message, progress)
        })
        .then(() => this.setState({
            loading: '',
            survey,
            answers: getDefaultAnswers(survey.current()),
            errors: {}, // clear errors
            alerts: [], // clear alerts
        }))
        .then(() => LocalStorage.set(CACHE_KEY, survey))
        .catch(err => this.alert(err));
    }

    render() {

        // @see surveysystem/backend/include/survey.h, struct question
        const { survey, errors, answers } = this.state;

        const questions = survey.current();
        const withGroups = mapQuestionGroups(questions);

        const answersCount = Object.keys(answers).length;
        const errorCount = Object.keys(errors).length;

        const hasQuestions = questions.length > 0;
        const hasErrors = errorCount > 0;
        const hasAnswers = answersCount > 0 ;
        const hasAllAnswers = (answersCount === questions.length);
        const didAnswerBefore = survey.questions.length > 1;
        const isFinished = survey.isFinished();

        return (
            <React.Fragment>
                <Dev.SurveyBar survey={ survey } />
                <SurveySection session={ survey }>
                    <Preloader loading={ this.state.loading } message={ this.state.loading }/>

                    {
                        (this.state.alerts.length) ?
                            <React.Fragment>
                                Unfortunately we encountered an error sending your data.
                                { this.state.alerts.map((entry, index) => <ApiAlert key={ index } error={ entry } />) }
                            </React.Fragment> : null
                    }

                    {
                        /* #379 show if survey is finished but not closed yet */
                        (isFinished) ?
                            <FinishSurvey
                                session = { survey }
                                handleDelAnswer = { this.handleDelAnswer.bind(this) }
                            />
                        : null
                    }

                    <SurveyContext.Provider value={ {
                        surveyID: survey.surveyID,
                        sessionID: survey.sessionID,
                    } }>
                        <SurveyMessage session={ survey } />
                        <SurveyProgress className="mb-2" session={ survey }/>

                        <SurveyForm
                            show={ !isFinished && questions.length > 0 && !this.state.loading }
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
                                hasErrors && <FeedbackItem className="list-group-item list-group-item-danger">Please fix errors above</FeedbackItem>
                            }

                            {
                                !hasAllAnswers && <FeedbackItem className="list-group-item list-group-item-warning">Please answer all questions</FeedbackItem>
                            }

                            <SurveyButtons
                                className="list-group-item pt-4 pb-4"
                                handlePrev={ this.handleDelAnswer.bind(this) }
                                handleNext={ this.handleUpdateAnswers.bind(this) }

                                hasQuestions={ hasQuestions }
                                hasErrors={ hasErrors }
                                hasAnswers={ hasAnswers }
                                hasAllAnswers={ hasAllAnswers }
                                didAnswerBefore={ didAnswerBefore }
                            />
                        </SurveyForm>
                    </SurveyContext.Provider>
                </SurveySection>

                <Dev.Pretty label="survey" data={ survey } open={ false }/>
                <Dev.Pretty label="questions" data={ questions } open={ false }/>
                <Dev.Pretty label="answers" data={ answers } open={ false }/>
                <Dev.Pretty label="errors" data={ errors } open={ false }/>

            </React.Fragment>
        );
    }
}

Survey.propTypes = {
};

export default Survey;
