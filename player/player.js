const imports = require('esm')(module);
const path = require('path');

const CSV = require('./csv');
const Log = require('./log');
const { getlastSessionEntry } = require('./session');

// frontend serializer
const AppSrcDir = path.resolve(path.join(__dirname, '../front/src'));
const { serializeQuestionAnswer } = imports(`${AppSrcDir}/serializer`);

// api
const Fetch = require('./fetch');

////
// Config
////

const Config = require('./config');

const CustomAnswers = Config.answers || {};

Fetch.SURVEYID = Config.Api.surveyid;
Fetch.HIST = Config.Api.host;
Fetch.PORT = Config.Api.port;

let SESSIONID;
let COUNT = 0;
let LOGFILE;

// misc
const now = new Date();

////
// Answers
////

/**
 * Provides a serialized answer for a given question object
 * @param {object} question object
 * @param {mixed} customAnswer a defined custom answer value or NULL
 *
 * @returns {string} serialized answer (colon-separated csv fragment)
 */
const provideSerializedAnswer = function(question, customAnswer = null) {
    let serialized;

    if (customAnswer !== null) {
        return serializeQuestionAnswer(null, question, customAnswer);
    }

    const { type } = question;

    switch (type) {

        case 'INT':
            serialized = serializeQuestionAnswer(null, question, 42);
            break;
        case 'FIXEDPOINT':
            serialized = serializeQuestionAnswer(null, question, Math.PI);
            break;

        case 'MULTICHOICE':
            serialized = serializeQuestionAnswer(null, question, question.choices[0]);
            break;

        case 'MULTISELECT':
            serialized = serializeQuestionAnswer(null, question, question.choices[0]);
            break;

        case 'LATLON':
            serialized = serializeQuestionAnswer(null, question, 1.01, 2.01);
            break;

            // TODO DATETIME
            // TODO DAYTIME slider/select

        case 'TIMERANGE':
            serialized = serializeQuestionAnswer(null, question, 2.5, 3.5);
            break;

        case 'TEXTAREA':
            serialized = serializeQuestionAnswer(null, question, 'textarea');
            break;
        case 'CHECKBOX':
            serialized = serializeQuestionAnswer(null, question, question.choices[1]);
            break;
        case 'HIDDEN':
            serialized = serializeQuestionAnswer(null, question, 'hidden value');
            break;
        case 'EMAIL':
            serialized = serializeQuestionAnswer(null, question, 'email@test.com');
            break;
        case 'PASSWORD':
            serialized = serializeQuestionAnswer(null, question, 'mypassword');
            break;

        // TODO SINGLECHOICE
        case 'SINGLECHOICE':
            serialized = serializeQuestionAnswer(null, question, question.choices[0]);
            break;

        case 'SINGLESELECT':
            serialized = serializeQuestionAnswer(null, question, question.choices[0]);
            break;

        case 'TEXT':
        default:
            serialized = serializeQuestionAnswer(null, question, 'Answer text');
    }

    return serialized;
};

////
// Questions
////

/**
 * Fetch next questions (initial request)
 * @param {object} question object
 *
 * @returns {Promise} http request response
 */
const nextQuestions = function() {
    return Fetch.json('/surveyapi/nextquestion', {
        sessionid: SESSIONID,
    });
};

/**
 * Handles received answer response and fetches Answer from backend session. Logs to lofileg
 * @param {object} question object
 * @param {string} answer Serialized answer
 * @param {string} answerType ("generic" or "custom")
 * @param {number} count sequence counter
 *
 * @returns {Promise} http request response
 */
const handleAnswer = function(response, question, answer, answerType, count) {
    return getlastSessionEntry(SESSIONID)
        .then((line) => {
            const nextIds = response.next_questions.map(q => q.id).toString();
            CSV.append(count, question.id, question.type, question.title, answerType, answer, line, nextIds);
            return response;
        });
};

/**
 * Send answer to a given question object
 * @param {object} question object
 * @param {string} answer Serialized answer
 * @param {string} answerType ("generic" or "custom")
 * @param {number} count sequence counter
 *
 * @returns {Promise} http request response
 */
const answerQuestion = function(question, answer, answerType, count) {
    return Fetch.json('/surveyapi/updateAnswer', {
        sessionid: SESSIONID,
        answer,
    })
        .then(response => handleAnswer(response, question, answer, answerType, count))
        .then(response => response);
};

/**
 * Send answers to a list of given questions. This function is recursive in case the LAST answer request contains next_questions
 * @param {object[]} array of question objects
 *
 * @returns {Promise} http request response
 */
const answerQuestions = function(questions) {
    COUNT += 1;
    const curr = COUNT;

    const ids = questions.map(q => q.id);
    Log.step(`${Log.colors.blue(curr)}: ${questions.length} questions to answer: ${ids.toString()}`);

    const data = questions.map((question) => {
        const customAnswer = CustomAnswers[question.id] || null;
        const answerType = (customAnswer !== null) ? 'custom' : 'generic';

        const answer = provideSerializedAnswer(question, customAnswer);

        const logType = (answerType === 'custom') ? Log.colors.green(answerType) : answerType;
        Log.log(`${curr} => ${Log.colors.yellow('title')}: ${question.id}: ${Log.colors.yellow('title:')} ${question.title}, ${Log.colors.yellow('answer type:')} ${logType}, ${Log.colors.yellow('answer:')} ${answer}`);

        return {
            question,
            answer,
            answerType,
        };
    });

    return Promise.all(data.map(entry => answerQuestion(entry.question, entry.answer, entry.answerType, curr)))
        .then((responses) => {
            const last = responses[responses.length - 1];
            const newQuestions = last.next_questions;

            Log.log(`${curr}: ${Log.colors.green(newQuestions.length)} new questions received..`);

            if (newQuestions.length) {
                return answerQuestions(newQuestions);
            }

            return Promise.resolve('FINISHED');
        });
};

////
// Play
////

// initialize new session
Fetch.raw('/surveyapi/newsession')
    .then((sessid) => {
        SESSIONID = sessid;
        return Log.note(`SessionId: ${SESSIONID}`, sessid);
    })
// initialize log file
    .then(sessid => CSV.init(`${Config.Api.surveyid}.${sessid}.log.csv`))
    .then((logfile) => {
        LOGFILE = logfile;
        return Log.note(`Logging into: ${LOGFILE}`, LOGFILE);
    })
// csv comment
    .then(() => CSV.append(`# Log for survey ${Config.Api.surveyid} session: ${SESSIONID} executed on: ${now.toLocaleString()}`))
// csv header row
    .then(() => CSV.append('step', 'question id', 'question type', 'question title', 'answer type', 'submitted answer', 'stored answer', 'next questions'))
// fetch first questions
    .then(() => nextQuestions())
// start question/answer loop
    .then(res => answerQuestions(res.next_questions))
// finalise
    .then(res => Log.success(`\nSurvey finished in ${COUNT} steps\n`, res))
    .then(() => CSV.finish())
    .then(logfile => Log.log(`   ${Log.colors.yellow('*')} Log file: ${logfile}\n`))
    .catch(err => Log.error(`REQUEST ERROR ${err}`, err));
