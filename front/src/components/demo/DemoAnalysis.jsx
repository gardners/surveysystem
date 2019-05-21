import React, { Component } from 'react';

import { mockAnalysis } from '../../Analysis';
import Analysis from '../Analysis';

class DemoAnalysis extends Component {
    render() {
        return (
            <Analysis
                evaluation={ mockAnalysis() }
                match = { {
                    params: {
                        id: 'DEMOSESSION',
                        surveyID: 'DEMOSURVEY',
                        sessionID: 'demo-session-id'
                    }
                } }
            />
        );
    }
}

DemoAnalysis.propTypes = {};

export default DemoAnalysis;
