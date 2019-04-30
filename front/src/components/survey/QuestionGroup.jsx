import React from 'react';
import PropTypes from 'prop-types';

import Question from './Question';
import QuestionModel, { findQuestionGroupCommons } from '../../Question';

import RadioMatrix from '../form/RadioMatrix';

const QuestionGroup = function({ questions, answers, handleChange }) {

    const commons = findQuestionGroupCommons(questions);

    const header = (commons !== 'NONE' && questions[0].type === 'HIDDEN') ? questions[0] : null;
    const items = questions.filter((question, index) => {
        if(!header) {
            return true;
        }
        return question.id !== header.id;
    });

    switch (commons) {

        case 'CHOICES':
            return (
                <React.Fragment>
                    { header && <Question question={ header } handleChange={ handleChange } answer= { null } /> }
                    <RadioMatrix
                        handleChange={ handleChange }
                        questions={ items }
                        answers={ answers }
                    />
                </React.Fragment>
            );

        default:
            // nothing
    }

    return (
        <React.Fragment>

        { header && <Question question={ header } handleChange={ handleChange } answer= { null } /> }
        {
            items.map((question, index) => {
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
