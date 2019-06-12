import React from 'react';
import PropTypes from 'prop-types';

import Question from './Question';
import QuestionModel, { findQuestionGroupCommons } from '../../Question';

import RadioMatrix from '../form/RadioMatrix';
import DaytimeSequence from '../form/DaytimeSequence';

import { addClassNames } from '../../Utils';

const QuestionGroup = function({ handleChange, questions, values, errors, className}) {

    const commons = findQuestionGroupCommons(questions);
    const header = (commons !== 'NONE' && questions[0].type === 'HIDDEN') ? questions[0] : null;
    const cls = addClassNames('question-group', className);

    const items = questions.filter((question, index) => {
        if(!header) {
            return true;
        }
        return question.id !== header.id;
    });

    switch (commons) {

        case 'CHOICES':
            return (
                <div className={ cls }>
                    {
                        header &&
                            <Question
                                handleChange={ handleChange }
                                question={ header }
                                value={ (header) ? values[header.id] : null }
                                error={ (header) ? errors[header.id] : null }
                                grouped={ true }
                            />
                    }
                    <RadioMatrix
                        handleChange={ handleChange }
                        questions={ items }
                        values={ values }
                        errors={ errors }
                        grouped={ true }
                    />
                </div>
            );

        case 'DAYTIME_SEQUENCE':
            return (
                <div className={ cls }>
                    {
                        header &&
                            <Question
                                handleChange={ handleChange }
                                question={ header }
                                value={ (header) ? values[header.id] : null }
                                error={ (header) ? errors[header.id] : null }
                                grouped={ true }
                            />
                    }
                    <DaytimeSequence
                        handleChange={ handleChange }
                        questions={ items }
                        values={ values }
                        errors={ errors }
                        grouped={ true }
                    />
                </div>
            );

        default:
            // nothing
    }

    return (
        <div className={ cls }>
        {
            header &&
                <Question
                    handleChange={ handleChange }
                    question={ header }
                    value={ (header) ? values[header.id] : null }
                    error={ (header) ? errors[header.id] : null }
                    grouped={ true }
                />
        } {
            items.map((question, index) => {
                const value = values[question.id] || null;
                const error = errors[question.id] || null;
                return(
                    <Question
                        key={ index }
                        handleChange={ handleChange }
                        question={ question }
                        value={ value }
                        error={ error }
                        grouped={ true }
                    />
                );
            })

        }
        </div>
    );

};

QuestionGroup.defaultProps = {
    answers: {},
    errors: {},
};

QuestionGroup.propTypes = {
    questions: PropTypes.arrayOf(
        QuestionModel.propTypes(),
    ).isRequired,
    answers: PropTypes.object,
    errors: PropTypes.object,
    handleChange: PropTypes.func.isRequired,
};

export default QuestionGroup;
