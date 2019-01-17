import React from 'react';
import PropTypes from 'prop-types';

import QuestionModel from '../../Question';
import Question from './Question';

import { FormRow, FieldError } from '../FormHelpers';

/**
 * Compiles questiongroups from a flat array of questions and renders groups
 */

const QuestionGroup = function(props) {
    const { questionGroup, handleChange, answers } = props;
    return (
        <React.Fragment>
            {
                questionGroup.map((question) => {

                    const answer = answers[question.id] || null;
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
                })
            }
        </React.Fragment>
    );
};

QuestionGroup.defaultProps = {
};

QuestionGroup.propTypes = {
    handleChange: PropTypes.func.isRequired,
    questionGroup: PropTypes.arrayOf(
        QuestionModel.propTypes(),
    ).isRequired,
};

export default QuestionGroup;
