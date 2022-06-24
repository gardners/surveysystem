import React, { Component } from 'react';

import { mockAnalysis } from '../../Analysis';

import AnalysisMeta from '../analysis/AnalysisMeta';
import AnalysisBody from '../analysis/AnalysisBody';


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
            analysis = new Error('Error: analysis has to be an object containing an array of Condition objects');
        }

        this.setState({
            json,
            analysis,
        });
    }

    render() {
        const { json, analysis } = this.state;

        return (
            <React.StrictMode>
                <section>
                    <textarea className="p-3 bg-dark text-white" style={ { width: '100%', height: '200px' } } onChange={ this.handleChange.bind(this) } value={ json } />
                    <hr className="mt-2 mb-2"/>
                    {
                        (analysis instanceof Error) ?
                            <div class="alert alert-danger">{ analysis.toString() }</div>
                            :
                            <React.Fragment>
                                <AnalysisMeta analysis={ analysis } />
                                <AnalysisBody analysis={ analysis } />
                            </React.Fragment>
                    }
                </section>
            </React.StrictMode>
        );
    }
}

DemoAnalysis.propTypes = {};

export default DemoAnalysis;
