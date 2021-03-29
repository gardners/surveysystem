import React, { Component } from 'react';

import { mockAnalysis } from '../../Analysis';
import AnalysisMeta from '../analysis/AnalysisMeta';
import { EvaluationGroup } from '../analysis/Evaluation';

class DemoAnalysis extends Component {

    constructor(props) {
        const analysis = mockAnalysis();

        super(props);
        this.state = {
            json: JSON.stringify(analysis, null, 4),
            analysis,
        };
    }

    handleChange(e) {
        const json = e.target.value;
        let analysis = {};

        try {
            analysis = JSON.parse(json);
        } catch (e) {
            analysis = e;
        }

        if(json[0] !== '{') {
            analysis = new Error('Error: analysis has to be an object containing an array evaluation objects');
        }

        this.setState({
            json,
            analysis,
        });
    }

    render() {
        const { json, analysis } = this.state;

        const { evaluations } = analysis;
        const survey_id = 'DEMO-SURVEY';
        const session_id = 'DEMO-SESSION';
        return (
            <section>
                <textarea className="p-3 bg-dark text-white" style={ { width: '100%', height: '200px' } } onChange={ this.handleChange.bind(this) } value={ json } />
                <hr className="mt-2 mb-2"/>
                {
                    (evaluations instanceof Error) ?
                        <div class="alert alert-danger">{ evaluations.toString() }</div>
                        :
                        <React.Fragment>
                            <AnalysisMeta survey_id={ survey_id } session_id={ session_id } analysis={ analysis } />
                            <EvaluationGroup survey_id={ survey_id } session_id={ session_id } evaluations={ evaluations } />
                        </React.Fragment>
                }
            </section>
        );
    }
}

DemoAnalysis.propTypes = {};

export default DemoAnalysis;
