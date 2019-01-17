import React from 'react';
import PropTypes from 'prop-types';

import { matrixState, propTypes } from '../../QuestionGroup';
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
                questionGroup.map((question, index) => {
                    const answer = answers[question.id] || null;
                    const error = (answer && answer.serialized instanceof Error) ? answer.serialized : null;

                    const mState = matrixState(index, questionGroup);

                    return (
                        <FormRow
                            key={ index }
                            className="list-group-item"
                            legend={ question.name }
                            description={ question.title_text }
                        >
                            <Question
                                matrix={ mState }
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
    questionGroup: propTypes().isRequired,
};

export default QuestionGroup;
