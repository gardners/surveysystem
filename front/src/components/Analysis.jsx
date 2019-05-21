import React, { Component } from 'react';
import PropTypes from 'prop-types';

import { Link } from 'react-router-dom';
import Typography from '@material-ui/core/Typography';
import Table from '@material-ui/core/Table';
import Box from '@material-ui/core/Box';
import TableBody from '@material-ui/core/TableBody';
import TableCell from '@material-ui/core/TableCell';
import TableRow from '@material-ui/core/TableRow';
import Button from '@material-ui/core/Button';

import ExpansionPanel from '@material-ui/core/ExpansionPanel';
import ExpansionPanelSummary from '@material-ui/core/ExpansionPanelSummary';
import ExpansionPanelDetails from '@material-ui/core/ExpansionPanelDetails';
import ExpandMoreIcon from '@material-ui/icons/ExpandMore';

// apis
import api from '../api';
import Log from '../Log';
import { normalizeAnalysis } from '../Analysis';
// components
import Content from './Content';
import Section from './Section';
import ApiAlert from './ApiAlert';
import Preloader from './Preloader';

// devel
import Dev from './Dev';

const WrapDisplay = function({ surveyID, sessionID, noPaper, children }) {
    return (
        <Content
            noPaper= { noPaper }
            title="Analysis"
            subTitle={ () => <Box component="span" color="primary.main">{ surveyID }</Box> }
            prepend={ sessionID }
        >
        { children }
        </Content>
    );
};

WrapDisplay.defaultProps = {
    noPaper: false,
};

WrapDisplay.propTypes = {
    surveyID: PropTypes.string.isRequired,
    sessionID: PropTypes.string.isRequired,
    noPaper: PropTypes.bool,
};


const tableStyle = {
    tableLayout: 'fixed',    /* For cells of equal size */
};

const cellStyle = {
    width: '25%',
};


class Analysis extends Component {

    constructor(props) {
        super(props);
        this.state = {
            evaluation: null,
            loading: '',
            alerts: [],
        }
    }

    componentDidMount() {
        // demo
        if (this.props.evaluation) {
            const { evaluation } = this.props;
            this.setState({
                evaluation,
                loading: '',
                alerts: [],
            });
            return;
        }

        const sessionID = this.props.match.params.sessionID;

        this.setState({
            loading: 'Fetching evaluation...',
        });

        api.getAnalysis(sessionID)
        .then(evaluation => this.setState({
            evaluation,
            loading: '',
            alerts: [],
        }))
        .catch(err => this.alert(err));
    }

    /**
     * Sends a message or an Error to this.state.alerts
     * This method ist tpically used by the request api in case of response or network errors
     * @param {Error} error
     * @returns {void}
     */
    alert(message, severity) {
        const { alerts } = this.state;

        if(severity !== 'error' && message instanceof Error) {
            severity = 'error';
        }

        Log.add(message, severity);
        alerts.push(message);

        this.setState({
            loading: '',
            alerts,
        });
    }

    render() {
        const surveyID = this.props.match.params.id;
        const sessionID = this.props.match.params.sessionID;
        const { evaluation, loading, alerts } = this.state;

        if (alerts.length) {
            return (
                <WrapDisplay surveyID={ surveyID } sessionID={ sessionID }>
                    <Typography color="error" paragraph>Unfortunately we encountered an error retrieving your data.</Typography>
                    { this.state.alerts.map((entry, index) => <ApiAlert key={ index } message={ entry } />) }
                    <Button variant="contained"
                        color="primary"
                        onClick={ () => window.location.reload() }>
                        Reload
                    </Button>&nbsp;
                    <Button variant="contained"
                        component={ Link } to={ `/survey/${surveyID}` }>
                        Back to Survey
                    </Button>
                </WrapDisplay>
            );
        }

        if (!evaluation && !loading) {
            return (
                <WrapDisplay surveyID={ surveyID } sessionID={ sessionID }>
                    <Typography color="error" paragraph>
                        This survey is not finished yet!<br/>
                        <Button variant="contained"
                        color="primary"
                            component={ Link } to={ `/survey/${surveyID}` }>
                            Back to Survey
                        </Button>
                    </Typography>
                </WrapDisplay>
            );
        }

        const data = normalizeAnalysis(this.state.evaluation);
        const { category, classification, rank, recommendation, riskRating } = data;
        const { condition, subcondition, mainText, learnMore, mainRecommendation, mandatoryTips, additionalInsights }  = data.displayResults.sleepConditions;

        return (
            <WrapDisplay surveyID={ surveyID } sessionID={ sessionID } noPaper>
                <Preloader loading={ loading }/>
                <Section noPadding>
                    <Table className="table table-striped table-hover table-sm" style={ tableStyle }>
                        <TableBody>
                            <TableRow>
                                <TableCell style={ cellStyle }>category</TableCell>
                                <TableCell>{ category }</TableCell>
                            </TableRow>
                            <TableRow>
                                <TableCell style={ cellStyle }>classification</TableCell>
                                <TableCell>{ classification }</TableCell>
                            </TableRow>
                            <TableRow>
                                <TableCell style={ cellStyle }>rank</TableCell>
                                <TableCell>{ rank }</TableCell>
                            </TableRow>
                            <TableRow>
                                <TableCell style={ cellStyle }>risk rating</TableCell>
                                <TableCell>{ riskRating }</TableCell>
                            </TableRow>
                            <TableRow>
                                <TableCell style={ cellStyle }>recommendation</TableCell>
                                <TableCell>{ recommendation }</TableCell>
                            </TableRow>
                        </TableBody>
                    </Table>
                </Section>

                <Typography variant="h3" gutterBottom>Condition</Typography>
                <Section noPadding>
                    <Table style={ tableStyle }>
                        <TableBody>
                            <TableRow>
                                <TableCell style={ cellStyle }>condition</TableCell>
                                <TableCell>{ condition }</TableCell>
                            </TableRow>
                            <TableRow>
                                <TableCell style={ cellStyle }>sub condition</TableCell>
                                <TableCell>{ subcondition }</TableCell>
                            </TableRow>
                        </TableBody>
                    </Table>
                </Section>
                <Typography variant="h3" gutterBottom>Details</Typography>

                <Section>
                    { mainText }
                </Section>

                <ExpansionPanel>
                    <ExpansionPanelSummary
                        expandIcon={ <ExpandMoreIcon /> }
                        aria-controls="ep1-content"
                        id="ep1-header"
                    >
                        <Typography>Learn More</Typography>
                    </ExpansionPanelSummary>
                    <ExpansionPanelDetails>
                        <Typography>{ learnMore }</Typography>
                    </ExpansionPanelDetails>
                </ExpansionPanel>

                <ExpansionPanel>
                    <ExpansionPanelSummary
                        expandIcon={ <ExpandMoreIcon /> }
                        aria-controls="ep2-content"
                        id="rep2-header"
                    >
                        <Typography>Main Recommendation</Typography>
                    </ExpansionPanelSummary>
                    <ExpansionPanelDetails>
                        <Typography>{ mainRecommendation }</Typography>
                    </ExpansionPanelDetails>
                </ExpansionPanel>

                <ExpansionPanel>
                    <ExpansionPanelSummary
                        expandIcon={ <ExpandMoreIcon /> }
                        aria-controls="ep3-content"
                        id="ep3-header"
                    >
                        <Typography>Tips</Typography>
                    </ExpansionPanelSummary>
                    <ExpansionPanelDetails>
                        <Typography>{ mandatoryTips }</Typography>
                    </ExpansionPanelDetails>
                </ExpansionPanel>

                <ExpansionPanel>
                    <ExpansionPanelSummary
                        expandIcon={ <ExpandMoreIcon /> }
                        aria-controls="ep4-content"
                        id="ep4-header"
                    >
                        <Typography>Additional insights</Typography>
                    </ExpansionPanelSummary>
                    <ExpansionPanelDetails>
                        <Typography>{ additionalInsights }</Typography>
                    </ExpansionPanelDetails>
                </ExpansionPanel>

                <Dev.Pretty label="raw analyis" data={ evaluation } open={ false }/>
            </WrapDisplay>
        );
    }
}

Analysis.defaultProps = {
    evaluation: null,
};

Analysis.propTypes = {
    evaluation: PropTypes.object,
};

export default Analysis;
