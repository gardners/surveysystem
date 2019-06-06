const imports = require('esm')(module);
const path = require('path');
const fs = require('fs');

const CSV = require('./csv');
const JSONF = require('./jsonf');
const Log = require('./log');

const { getlastSessionEntry } = require('./session');

// frontend serializer
const AppSrcDir = path.resolve(path.join(__dirname, '../front/src'));
const Answer = imports(`${AppSrcDir}/Answer`);

// api
const Fetch = require('./fetch');

////
// Config
////

// print process.argv
const configFile = process.argv[2] || null;

if (!configFile) {
    Log.error('Command line error:');
    Log.log(' * Config file required! Exiting player ...');
    process.exit(1);
}

if (!fs.existsSync(configFile)) {
    Log.error('Command line error: ');
    Log.log(' * Config file does not exist! Exiting player ...');
    process.exit(1);
}

// eslint-disable-next-line import/no-dynamic-require
const Config = require(path.resolve(configFile));

Log.log(`\n    * ${Log.colors.yellow('using config: ')}${configFile}\n`);

const CustomAnswers = Config.answers || {};

Object.assign(Fetch, Config.Api);

let SESSIONID;
let COUNT = 0;
let LOGFILE;
let JSONFILE;

// misc
const now = new Date();

const sleep = millisec => new Promise(resolve => setTimeout(resolve, millisec));

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
    let answer;

    if (customAnswer !== null) {
        answer = Answer.setValue(question, 42);
        return Answer.serialize(answer);
    }

    const { type, id } = question;

    switch (type) {

        case 'INT':
            answer = Answer.setValue(question, 42);
            break;
        case 'FIXEDPOINT':
            answer = Answer.setValue(question, Math.PI);
            break;

        case 'MULTICHOICE':
            answer = Answer.setValue(question, [question.choices[0]]);
            break;

        case 'MULTISELECT':
            answer = Answer.setValue(question, [question.choices[0]]);
            break;

        case 'LATLON':
            answer = Answer.setValue(question, [1.01, 2.01]);
            break;

            // TODO DATETIME
            // TODO DAYTIME slider/select

        case 'TIMERANGE':
            answer = Answer.setValue(question, [2.5, 3.5]);
            break;

        case 'TEXTAREA':
            answer = Answer.setValue(question, 'textarea');
            break;
        case 'CHECKBOX':
            answer = Answer.setValue(question, question.choices[1]);
            break;
        case 'HIDDEN':
            answer = Answer.setValue(question, 'hidden value');
            break;
        case 'EMAIL':
            answer = Answer.setValue(question, 'email@test.com');
            break;
        case 'PASSWORD':
            answer = Answer.setValue(question, 'mypassword');
            break;

        // TODO SINGLECHOICE
        case 'SINGLECHOICE':
            answer = Answer.setValue(question, question.choices[0]);
            break;

        case 'SINGLESELECT':
            answer = Answer.setValue(question, question.choices[0]);
            break;

        case 'TEXT':
        default:
            Log.error('Question type error:');
            throw new Error(`Question id: ${id} Unknown question type ${type}`)
    }

    return Answer.serialize(answer);
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
const analyse = function() {
    return Fetch.json('/surveyapi/analyse', {
        sessionid: SESSIONID,
    });
};

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
    const nextIds = response.next_questions.map(q => q.id).toString();

    return getlastSessionEntry(SESSIONID)
        // workaround for mockserver, TODO
        .catch((err) => {
            return (err.code === 'ENOENT') ? 'no entry' : err;
        })
        .then((line) => {
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
 * @returns {Promise} http request response
 */
const answerQuestionsSequential = function(entries, count, responses = []) {
    const next = responses.length;

    const { question, answer, answerType } = entries[next];

    return answerQuestion(question, answer, answerType, count)
        .then(response => handleAnswer(response, question, answer, answerType, count))
        .then(response => Log.log(`    |   └── ${answer}`, response))
        .then((response) => {
            responses.push(response);
            if (responses.length < entries.length) {
                return answerQuestionsSequential(entries, count, responses);
            }
            return responses;
        });
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

        if(typeof answer === Error) {
            Log.error(answer.message);
            console.log(question, answer);
            process.exit(1);
        }

        const logType = (answerType === 'custom') ? Log.colors.green(answerType) : answerType;
        Log.log(`  ${Log.colors.yellow('title')}: ${question.id}: ${Log.colors.yellow('title:')} ${question.title}, ${Log.colors.yellow('answer type:')} ${logType}, ${Log.colors.yellow('answer:')} ${answer}`);

        return {
            question,
            answer,
            answerType,
        };
    });

    Log.log(`    ├── Sending ${Log.colors.green(data.length)} answers`);

    return sleep(50)
        .then(() => answerQuestionsSequential(data, curr))
        .then((responses) => {
            const last = responses[responses.length - 1];
            const newQuestions = last.next_questions;
            const qids = newQuestions.map(q => q.id).toString();
            Log.log(`    └── ${Log.colors.green(newQuestions.length)} new questions received.. (${qids})`);

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
        return Log.note(`SessionId: ${SESSIONID}`);
    })
// initialize CSV log file
    .then(() => CSV.init(`${Config.Api.SURVEYID}.${SESSIONID}.log.csv`))
    .then((logfile) => {
        LOGFILE = logfile;
        return Log.note(`Logging into: ${LOGFILE}`, LOGFILE);
    })
// initialize Analysis file
    .then(() => JSONF.init(`${Config.Api.SURVEYID}.${SESSIONID}.analyse.json`))
    .then((jsonfile) => {
        JSONFILE = jsonfile;
        return Log.note(`Logging analysis into: ${JSONFILE}`, JSONFILE);
    })
// csv comment
    .then(() => CSV.append(`# Log for survey ${Config.Api.SURVEYID} session: ${SESSIONID} executed on: ${now.toLocaleString()}`))
// csv header row
    .then(() => CSV.append('step', 'question id', 'question type', 'question title', 'answer type', 'submitted answer', 'stored answer', 'next questions ids'))
// fetch first questions
    .then(() => nextQuestions())
// start question/answer loop
    .then(res => answerQuestions(res.next_questions))
// finalise
    .then(() => CSV.finish())
    .then(res => Log.success(`\nSurvey questions finished in ${COUNT} steps\n`, res))
    .then(logfile => Log.log(`   ${Log.colors.yellow('*')} Log file: ${logfile}\n`))
// analyse
// initialize log file
    .then(() => analyse())
    .then(res => JSONF.save(res))
    .then(() => JSONF.finish())
    .then(jsonfile => Log.success('\nSurvey analysis retrieved\n', jsonfile))
    .then(jsonfile => Log.log(`   ${Log.colors.yellow('*')} File: ${jsonfile}\n`))
    .catch(err => Log.error(`REQUEST ERROR ${err}`, err));
