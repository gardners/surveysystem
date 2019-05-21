import React from 'react';
import PropTypes from 'prop-types';

import Box from '@material-ui/core/Box';
import Typography from '@material-ui/core/Typography';
import FormGroup from '@material-ui/core/FormGroup';

import QuestionModel from '../../Question';
import { addClassNames } from '../../Utils';

import InnerHtml from '../InnerHtml';
import { sanitizeKcgiJsonString } from '../../Utils';

const QuestionRow = function({ question, className, grouped, componentName, children, debugData }) {

    const { title, type, description } = question;
    const sanitizedDescription = sanitizeKcgiJsonString(description);

    return (
            <Box
                className={ addClassNames(className, 'question') }
                data-debug={ debugData }
            >
                <Typography variant="h4">{ title }</Typography>
                {
                    sanitizedDescription &&
                        <Box mb={ 1}>
                            <InnerHtml htmlContent={ sanitizedDescription } />
                        </Box>
                }
                { children }
            </Box>
    );
};

QuestionRow.defaultProps = {
    grouped: false,
    className: '',
    debugData: '',
};

QuestionRow.propTypes = {
    className: PropTypes.string,
    question: QuestionModel.propTypes().isRequired,
    componentName: PropTypes.string.isRequired, // form input component name
    grouped: PropTypes.bool,
    debugData: PropTypes.string,
};

export default QuestionRow;
