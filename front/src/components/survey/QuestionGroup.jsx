import React from 'react';
import PropTypes from 'prop-types';

import Question from './Question';
import QuestionModel, { findQuestionGroupCommons } from '../../Question';

import RadioMatrix from '../form/RadioMatrix';

const QuestionGroup = function({ questions, answers, handleChange }) {

    const commons = findQuestionGroupCommons(questions);

    switch (commons) {

        case 'CHOICES':
            return (
                <RadioMatrix
                    handleChange={ handleChange }
                    questions={ questions }
                    answers={ answers }
                />
            );

        default:
            // nothing
    }

    return (
        <React.Fragment>
        {
            questions.map((question, index) => {
                const answer = answers[question.id] || null;
                return(
                    <Question
                        key={ index }
                        handleChange={ handleChange }
                        question={ question }
                        answer= { answer }
                        grouped={ true }
                    />
                );
            })

        }
        </React.Fragment>
    );

};

QuestionGroup.defaultProps = {
    answers: {}
};

QuestionGroup.propTypes = {
    questions: PropTypes.arrayOf(
        QuestionModel.propTypes(),
    ).isRequired,
    answers: PropTypes.object,
    handleChange: PropTypes.func.isRequired,
};

export default QuestionGroup;
