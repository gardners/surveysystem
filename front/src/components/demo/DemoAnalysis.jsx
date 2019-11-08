import React, { Component } from 'react';

import { mockAnalysis } from '../../Analysis';
import { EvaluationGroup } from '../analysis/Evaluation';

class DemoAnalysis extends Component {

    constructor(props) {
        const evaluations = mockAnalysis();

        super(props);
        this.state = {
            json: JSON.stringify(evaluations, null, 4),
            evaluations,
        };
    }

    handleChange(e) {
        const json = e.target.value;
        let evaluations = [];

        try {
            evaluations = JSON.parse(json);
        } catch (e) {
            evaluations = e;
        }

        if(json[0] !== '[') {
            evaluations = new Error('Error: analysis has to be an array of evaluation objects');
        }

        this.setState({
            json,
            evaluations,
        });
    }

    render() {
        const { json, evaluations } = this.state;

        return (
            <section>
                <textarea className="p-3 bg-dark text-white" style={ { width: '100%', height: '200px' } } onChange={ this.handleChange.bind(this) } value={ json } />
                <hr className="mt-2 mb-2"/>
                {
                    (evaluations instanceof Error) ?
                        <div class="alert alert-danger">{ evaluations.toString() }</div>
                        :
                        <EvaluationGroup
                            evaluations={ evaluations }
                            surveyID="DEMO-SURVEY"
                            sessionID="DEMO-SESSION"
                        />
                }
            </section>
        );
    }
}

DemoAnalysis.propTypes = {};

export default DemoAnalysis;
