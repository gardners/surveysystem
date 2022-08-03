import React from 'react';

import { Models,  normalizeAnalysis, analysisPropTypes } from '../../Analysis';
import Condition from './Condition';


const AnalysisBody = function({ analysis }) {
    const { conditions } = normalizeAnalysis(analysis);

    return (
        <React.Fragment>
            {
                conditions.map((condition, index) =>
                    <div key={ index }>
                        <span className="text-muted">Condition { index + 1 }/{ conditions.length }:</span>
                        <Condition prefix={ `${index + 1}/${conditions.length}` } condition={ condition }/>
                    </div>
                )
            }
        </React.Fragment>
    );
}

AnalysisBody.defaultProps = {
    analysis: Models.analysis(),
};

Condition.propTypes = {
    analysis: analysisPropTypes(),
};

export default AnalysisBody;
