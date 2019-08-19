import React, { Component } from 'react';
import PropTypes from 'prop-types';

import SurveyManager from '../../SurveyManager';
import SurveyList from '../../SurveyList';

import { SurveyItem } from '../Surveys';
import EmbedHtmlFile from '../EmbedHtmlFile';
import Modal from '../Modal';

const { PUBLIC_URL } = process.env;

class SurveyHeader extends Component {

    constructor(props) {
        super(props);
        this.state = {
            survey: null,
        }
    }

    componentDidMount() {
        const { session } = this.props;
        SurveyList.getById(session.surveyID)
        .then(survey => this.setState({ survey }))
        .catch(error => { /* nothing */ });
    }

    render () {

        const { session } = this.props;
        const { survey } = this.state;

        return (
            <React.Fragment>
                <h1>{ (survey) ? survey.name : session.surveyID }</h1>
                {
                    (survey) ?
                        <ul className="nav">
                            <li className="nav-item">
                                <Modal
                                    title="About this survey"
                                    buttonClassName="btn btn-link btn-sm mr-3"
                                    buttonText={ () => 'About this survey' }
                                >
                                    <SurveyItem survey={ survey } withButtons={ false }/>
                                </Modal>
                                {
                                    survey.pages.map(
                                        (page, index) =>
                                            <Modal
                                                key={ index }
                                                title={ page.title }
                                                buttonClassName="btn btn-link btn-sm mr-3"
                                                buttonText={ () => page.title || '' }
                                            >
                                                <EmbedHtmlFile
                                                    title={ page.title || '' }
                                                    src={ (page.src) ? `${PUBLIC_URL}/surveys/${survey.id}/${page.src}` :  '' }
                                                    showTitle={ false }
                                                    />
                                            </Modal>
                                    )
                                }
                            </li>
                        </ul>
                    : null
                }
            </React.Fragment>
        );
    }
};

SurveyHeader.defaultProps = {};

SurveyHeader.propTypes = {
    session: PropTypes.instanceOf(SurveyManager).isRequired,
};

export default SurveyHeader;
