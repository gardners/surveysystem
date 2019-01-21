import React from 'react';
import PropTypes from 'prop-types';

import QuestionsModel from '../../Questions';
import QuestionModel from '../../Question';
import Question from './Question';

import RadioMatrix from '../form/RadioMatrix';

import { FormRow, FieldError } from '../FormHelpers';

const getError = function(answer) {
    return (answer && answer.serialized instanceof Error) ? answer.serialized : null;
};

const answerPropTypes = function() {
    return PropTypes.shape({
        values: PropTypes.any,
        serialized: PropTypes.oneOfType([
            PropTypes.string,
            PropTypes.instanceOf(Error),
        ])
    });
};

/**
 *
 */
const Many = function({ questions, answers, handleChange}) {
    const first = questions[0];
console.log(questions);
    if (first.type === 'SINGLECHOICE') {
        return (
            <FormRow
                key={ first.id }
                className="list-group-item"
                legend={ first.name }
                description={ first.title_text }
            >
                <RadioMatrix
                    questions={ questions }
                    handleChange={ handleChange }
                />
            </FormRow>
        );
    }

    return (
        <FormRow
            key={ first.id }
            className="list-group-item"
            legend={ first.name }
            description={ first.title_text }
        >
            {
                questions.map((question, index) => {
                    const answer = answers[question.id] || null;
                    return (
                        <React.Fragment>
                            <Question
                                key ={ index }
                                handleChange={ handleChange }
                                question={ question }
                                answer= { answer }
                            />
                            { (question.type !== 'HIDDEN') ? <FieldError error={ getError(answer) }/> : null }
                        </React.Fragment>
                    );
                })
            }

        </FormRow>
    );

};

Many.defaultProps = {
    answers: [],
};

Many.propTypes = {
    questions: PropTypes.arrayOf(
        QuestionModel.propTypes()
    ).isRequired,
    answers: PropTypes.arrayOf(
        answerPropTypes(),
    ).isRequired,
    handleChange: PropTypes.func.isRequired,
};

/**
 *
 */
const One = function({ question, answer, handleChange }) {

    const error = (answer && answer.serialized instanceof Error) ? answer.serialized : null;

    return (
        <FormRow
            key={ question.id }
            className="list-group-item"
            legend={ question.name }
            description={ question.title_text }
        >
            <Question
                handleChange={ handleChange }
                question={ question }
                answer= { answer }
            />
            { (question.type !== 'HIDDEN') ? <FieldError error={ error }/> : null }
        </FormRow>
    );

};

One.defaultProps = {
    answer: null,
};

One.propTypes = {
    question: QuestionModel.propTypes().isRequired,
    answer: answerPropTypes(),
    handleChange: PropTypes.func.isRequired,
};

/**
 * Compiles questiongroups from a flat array of questions and renders groups
 */
const Questions = function({ questions, answers, handleChange }) {

    const processed = QuestionsModel.createQuestionGroups(questions);

    return (
        <React.Fragment>
            {
                processed.map((question, index) => {

                    const isGroup = Object.prototype.toString.call(question) === '[object Array]';

                    // group

                    if(isGroup) {
                        return (
                            <Many
                                key={ index }
                                handleChange={ handleChange }
                                questions={ question }
                                answers= { answers }
                                row
                            />
                        );
                    }

                    return (
                        <One
                            key={ index }
                            handleChange={ handleChange }
                            question={ question }
                            answer={ answers[question.id] || null }
                        />
                    );

                })
            }
        </React.Fragment>
    );

};

Questions.defaultProps = {
    answers: {}
};

Questions.propTypes = {
    questions: PropTypes.arrayOf(
        QuestionModel.propTypes(),
    ).isRequired,
    answers: PropTypes.object,
    handleChange: PropTypes.func.isRequired,
};

export default Questions;
