
(function(global, document) {

    /**
     * hook function pattern: <surveyId>__<questionId>
     * @param {string} questionId
     * @param {Question[]} questions An array of survey question objects (current screen)
     * @param {object} A map of answers values where the key is the corrensponding question id, repesenting the current state. Note that not all questions may have answers and that answer values can be Error objects.
     *
     * @returns {object[]} an array of message objects with the following members: {string} questionId, {string} status (one of "warning", "info", "success", "error"), {string} message. reurn an empty array if you don't want to display a message.
     */

    const Callbacks = {

        exampleSurveyId__exampleQuestionId: function(questionId, questions, answers) {

            // An answer was not given yet...
            if (typeof answers.exampleQuestionId === 'undefined') {
                return [];
            }

            // Keep silent if ANY of the answers is an Error...
            // Answer values may be be typeof Error. These are survey validation errors and will be handled internally. It does not need to be reflected here.
            if (answers.filter(answer => answer instanceof Error).length) {
                return [];
            }

            // answer inspection && logic...
            if (answers.exampleQuestionId === 'petting the tiger') {
                return [{
                    questionId,
                    status: 'warning',
                    message: 'You shouldn\'t do that.',
                }];
            }

            return [];
        },

    };

    //// do not edit below this line ////

    const getInstantFeedback = function(surveyId, questionId, questions, answers) {
        const key = `${surveyId}__${questionId}`;

        let messages = [];
        if (typeof Callbacks[key] === 'function') {
            try {
                messages = Callbacks[key](questionId, questions, answers);
            } catch (e) {
                console.error('getInstantFeedback: callback, "' + key + '" error: ' + e.toString());
                return messages;
            }
            return messages;
        }

        return [];
    };

    if (typeof global.ss === 'undefined') {
        global.SS = {};
    }

    global.SS.getInstantFeedback = getInstantFeedback;

}(window, document));
