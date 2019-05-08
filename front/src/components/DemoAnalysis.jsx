import React, { Component } from 'react';

import { mockAnalysis } from '../Analysis';
import Analysis from './Analysis';

class DemoAnalysis extends Component {
    render() {
        return (
            <section>
                <Analysis
                    evaluation={ mockAnalysis() }
                    match = { {
                        params: {
                            id: 'DEMOSESSION',
                            surveyID: 'DEMOSURVEY',
                        }
                    } }
                />
            </section>
        );
    }
}

DemoAnalysis.propTypes = {};

export default DemoAnalysis;
