import React, { Component } from 'react';

import Typography from '@material-ui/core/Typography';
import Content from '../Content';
import Section from '../Section';
import TextField from '@material-ui/core/TextField';

import { mapQuestionGroups } from '../../Question';
import { isArray } from '../../Utils';

import Question from '../survey/Question';
import QuestionGroup from '../survey/QuestionGroup';
import Dev from '../Dev';

const defaultValue = `
    version 2
    Silly test survey updated
    without python
    QuestionGroup__5:How funny are they? Please rate the Hitchhikers below...::HIDDEN:-1::-999:999:2:0::
    Zaphod__5:Zaphod Beeblebrox::SINGLECHOICE:-1::-999:999:2:2:0,1,2,3,4,5,6,7,8,9:
    Arthur__5:Arthur Dent::SINGLECHOICE:-1::-999:999:2:2:0,1,2,3,4,5,6,7,8,9:
    Trillian__5:Trillian::SINGLECHOICE:-1::-999:999:2:2:0,1,2,3,4,5,6,7,8,9:
    Marvin__5:Marvin the Paranoid Android::SINGLECHOICE:-1::-999:999:2:2:0,1,2,3,4,5,6,7,8,9:
    Ford__5:Ford Prefect::SINGLECHOICE:-1::-999:999:2:2:0,1,2,3,4,5,6,7,8,9:
    Slartibartfast__5:Slartibartfast::SINGLECHOICE:-1::-999:999:2:2:0,1,2,3,4,5,6,7,8,9:
`;

/**
 * Parses a survey manifest,
 * !! replicates backend logic
 * @see survey.h: struct question
 *     struct question {
 *         char *uid;
 *         char *question_text;
 *         char *question_html;
 *         int type;
 *         char *default_value;
 *         long long min_value;
 *         long long max_value;
 *         int decimal_places;
 *         int num_choices;
 *         char *choices;
 *         char *unit;
 *     };
 * @see serialiserc: serialise_question(struct question *q,char *out,int max_len)
 *     do {
 *         SERIALISE_BEGIN(out,len,max_len);
 *         SERIALISE_STRING(q->uid);
 *         SERIALISE_STRING(q->question_text);
 *         SERIALISE_STRING(q->question_html);
 *         SERIALISE_THING(q->type,serialise_question_type);
 *         SERIALISE_INT(q->flags);
 *         SERIALISE_STRING(q->default_value);
 *         SERIALISE_LONGLONG(q->min_value);
 *         SERIALISE_LONGLONG(q->max_value);
 *         SERIALISE_INT(q->decimal_places);
 *         SERIALISE_INT(q->num_choices);
 *         SERIALISE_STRING(q->choices);
 *         SERIALISE_STRING(q->unit);
 *         // Trim terminal separator character
 *         SERIALISE_COMPLETE(out,len,max_len);
 *     } while(0);
 * @see sessions.c: load_session(char *session_id)
 * <id>:<title>:<description>:<type>:-1::-999:999:2:2:No,Yes:
 */
const parseQuestionManifest = function(survey) {
    const lines = survey.split('\n');

    if(typeof survey === 'undefined' || !survey) {
        return new Error('Manifest is empty');
    }

    const questions = [];
    const length = lines.length;

    for (let i = 0; i < length; i++) {

        if (/:/g.test(lines[i]) === false) {
            continue;
        }

        const fields = lines[i].trim().split(':');

        if(fields.length !== 12) {
            return new Error(`(line ${i}) Manifest is invalid. It requires at least one line with 12 fields separated by ":"`);
        }

        const id = fields[0];
        const name = fields[0];
        const title = fields[1];
        const description = fields[2];
        const type = fields[3];
        const unit = fields[11];

        const question = {
            id,
            name,
            title,
            description,
            type,
            unit
        };

        if (typeof fields[10] !== 'undefined' && fields[10]) {
            question.choices = fields[10].split(',');
        };

        questions.push(question);
    }

    return questions;
};

class DemoManifest extends Component {
    constructor(props) {
        super(props);
        this.state = {
            next_questions: [],
        };
    }

    componentDidMount() {
        const next_questions = parseQuestionManifest(defaultValue);
        this.setState({
            next_questions,
        });
    }

    handleChange(e) {
        const { value } = e.target;
        const next_questions = parseQuestionManifest(value);
        this.setState({
            next_questions,
        });
    }

    render() {
        const { next_questions } = this.state;
        const withGroups = mapQuestionGroups(next_questions);

        return (
            <Content title="Demo: Question Manifest">
                <Section noPaper>
                    <Typography variant="h4" component="h3">Manifest</Typography>
                    <TextField
                        multiline
                        fullWidth
                        margin="normal"
                        variant="outlined"
                        label="Edit manifest:"
                        onChange={ this.handleChange.bind(this) }
                        defaultValue={ defaultValue }
                    />

                    <Dev.Pretty label='parsed "next_questions" JSON' data={ this.state.next_questions } />
                </Section>

                <Typography variant="h4" component="h3" gutterBottom>Component</Typography>

                <Section>
                    {
                        (next_questions instanceof Error) && <div className="alert alert-danger">{ next_questions.message }</div>
                    }
                    {
                        isArray(next_questions) &&
                            withGroups.map((entry, index) => {

                                if(isArray(entry)) {
                                    return (
                                        <div key={ index } className="list-group-item">
                                            <QuestionGroup
                                                handleChange={ () => true }
                                                questions={ entry }
                                                answers={ this.state.answers }
                                            />
                                        </div>
                                    );
                                }

                                return (
                                    <div key={ index } className="list-group-item">
                                        <Question
                                            handleChange={ this.handleChange.bind(this) }
                                            question={ entry }
                                            answer={ this.state.answers[entry.id] || null }
                                        />
                                    </div>
                                );

                            })
                    }
                </Section>
            </Content>
        );
    }
}

DemoManifest.propTypes = {};

export default DemoManifest;
