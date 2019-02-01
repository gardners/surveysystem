const fs = require('fs');
const path = require('path');

const SessionDelimiter = '|';

const loadSurveyFile = function(surveyid) {
    const file = path.resolve(__dirname) + '/surveys/' + surveyid + '.json';
    const exists = fs.existsSync(file);

    if(!exists) {
        return null;
    }

    const contents = fs.readFileSync(file, 'utf8');
    return JSON.parse(contents);
};

const nextQuestionByansweredID = function(answeredID, survey, previous) {
    const { length } = survey;

    let i
    let k
    let nlength;

    for (let i = 0; i < length; i += 1) {
        nlength = survey[i].next_questions.length;

        for (k = 0; k < nlength; k += 1) {

            if (survey[i].next_questions[k].id === answeredID) {
                // return next or last or first
                if (survey.indexOf(i + 1) >= length) {
                    return {
                        current: length - 1,
                        statusCode: '200',
                        statusText: 'OK',
                        payload: JSON.stringify(survey[length - 1]),
                    };
                }

                return {
                    current: i + 1,
                    statusCode: '200',
                    statusText: 'OK',
                    payload: JSON.stringify(survey[i + 1]),
                };

            }
        }
    }

    return {
        current: previous,
        statusCode: '500',
        statusText: 'Internal Server Error',
        payload: 'couldn\'t find answer for ' + answeredID,
    };
};

const newsession = function(surveyid) {
    if(loadSurveyFile(surveyid)) {
        return {
            current: -1,
            statusCode: '200',
            statusText: 'OK',
            payload: surveyid + SessionDelimiter + Date.now(),
        };
    }

    return {
        current: -1,
        statusCode: '404',
        statusText: 'Not Found',
        payload: '',
    };
};

const parseSIDfromSession = function(sessionID) {
    sessionID = sessionID || '';

    if(!sessionID) {
        return '';
    }

    const parts = sessionID.split(SessionDelimiter);
    if(!parts.length) {
        return '';
    }
    return parts[0];
}

const nextquestion = function(surveyid, previous) {
    const survey = loadSurveyFile(surveyid).survey;

    if(!survey) {
        return {
            current: previous,
            statusCode: '404',
            statusText: 'Not Found',
            payload: '',
        };
    }

    const current = (previous < survey.length - 1) ? previous + 1 : survey.length;

    return {
        current: current,
        statusCode: '200',
        statusText: 'OK',
        payload: JSON.stringify(survey[current]),
    };
};

const delanswer = function(surveyid, previous) {
    const survey = loadSurveyFile(surveyid).survey;

    if(!survey) {
        return {
            current: previous,
            statusCode: '404',
            statusText: 'Not Found',
            payload: '',
        };
    }

    const current = (previous > 0) ? previous - 1 : 0;

    return {
        current: current,
        statusCode: '200',
        statusText: 'OK',
        payload: JSON.stringify(survey[current]),
    };
};

const updateanswer = function(surveyid, answer, previous) {

    const survey = loadSurveyFile(surveyid).survey;

    if(!survey) {
        return {
            current: previous,
            statusCode: '404',
            statusText: 'Not Found',
            payload: '',
        };
    }

    const answerID = answer.split(':')[0];
    return nextQuestionByansweredID(answerID, survey, previous);
};

// TODO: temporary endpoint
const getevaluation = function(surveyid, previous = 0) {

    const data = loadSurveyFile(surveyid);
    const surveyLength = data.survey.length;

    //if(previous < data.survey.length - 1) {
    //    return {
    //        current: previous,
    //        statusCode: '400',
    //        statusText: 'Bad Request',
    //        payload: 'The survey has not been finished yet',
    //    };
    //}

    let evaluation = {};

    if(typeof data.evaluationSrc !== 'undefined' && data.evaluationSrc) {
        evaluation = require(path.resolve(__dirname, data.evaluationSrc));
    }

    return {
        current: -1, // reset survey
        statusCode: '200',
        statusText: 'ok',
        payload: JSON.stringify(evaluation),
    };
};

module.exports = {
    parseSIDfromSession,
    newsession,
    nextquestion,
    delanswer,
    updateanswer,
    // TODO: temporary endpoint
    getevaluation,
};

