import React from 'react';
import PropTypes from 'prop-types';

import { normalizeEvaluation, evaluationPropTypes } from '../../Analysis';

import Toggle from '../Toggle';

const Evaluation = function({ index, evaluation }) {
    const data = normalizeEvaluation(evaluation);
    const { category, classification, rank, recommendation, riskRating } = data;
    const { condition, subcondition, mainText, learnMore, mainRecommendation, mandatoryTips, additionalInsights }  = data.displayResults.conditions;

    return (
        <div>
            <p>Condition { index + 1 }</p>
            <h3><span className="text-primary">{ condition }</span></h3>
            <div className="list-group mb-4">
                {
                    subcondition &&
                        <div className="list-group-item">
                            <div className="row">
                                <div className="col-sm-3">Subcondition</div>
                                <div className="col-sm-9">{ subcondition }</div>
                            </div>
                        </div>
                }
                {
                    category &&
                        <div className="list-group-item">
                            <div className="row">
                                <div className="col-sm-3">Category</div>
                                <div className="col-sm-9">{ category }</div>
                            </div>
                        </div>
                }
                {
                    classification &&
                        <div className="list-group-item">
                            <div className="row">
                                <div className="col-sm-3">Classification</div>
                                <div className="col-sm-9">{ classification }</div>
                            </div>
                        </div>
                }
                <div className="list-group-item">
                    <div className="row">
                        <div className="col-sm-3">Rank</div>
                        <div className="col-sm-9">{ rank }</div>
                    </div>
                </div>
                <div className="list-group-item">
                    <div className="row">
                        <div className="col-sm-3">Risk rating</div>
                        <div className="col-sm-9">{ riskRating }</div>
                    </div>
                </div>
                {
                    recommendation &&
                        <div className="list-group-item">
                            <div className="row">
                                <div className="col-sm-3">Recommendation</div>
                                <div className="col-sm-9">{ recommendation }</div>
                            </div>
                        </div>
                }

                <div className="list-group-item bg-light">
                    <Toggle title="More..." className="mt-2 mb-2" titleTag="button" titleClassName="btn btn btn-primary d-print-none">

                    <div className="list-group list-group-flush">
                        <div className="list-group-item bg-light">{ mainText }</div>

                        <div className="list-group-item bg-light">
                            <div className="row">
                                <div className="col-sm-3">Learn more</div>
                                <div className="col-sm-9">{ learnMore }</div>
                            </div>
                        </div>
                        <div className="list-group-item bg-light">
                            <div className="row">
                                <div className="col-sm-3">Main Recommendation</div>
                                <div className="col-sm-9">{ mainRecommendation }</div>
                            </div>
                        </div>
                        <div className="list-group-item bg-light">
                            <div className="row">
                                <div className="col-sm-3">Tips</div>
                                <div className="col-sm-9">{ mandatoryTips }</div>
                            </div>
                        </div>
                        <div className="list-group-item bg-light">
                            <div className="row">
                                <div className="col-sm-3">Additional insights</div>
                                <div className="col-sm-9">{ additionalInsights }</div>
                            </div>
                        </div>
                    </div>
                </Toggle>
                </div>
            </div>
        </div>
    );
}

Evaluation.defaultProps = {
    evaluation: null,
};

Evaluation.propTypes = {
    index: PropTypes.number.isRequired,
    evaluation: evaluationPropTypes(),
};

const EvaluationGroup = function({ evaluations }) {
    return (
        <section>
            {
                evaluations.map((item, index) => <Evaluation key={ index } index={ index } evaluation={ item } />)
            }
        </section>
    );
}

EvaluationGroup.defaultProps = {
    evaluations: [],
};

EvaluationGroup.propTypes = {
    analysis: PropTypes.arrayOf(evaluationPropTypes()),
};

export { Evaluation, EvaluationGroup };
