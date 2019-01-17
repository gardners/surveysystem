import React, { Component } from 'react';
import PropTypes from 'prop-types';

import Question from '../../Question';

class RadioMatrixRow extends Component {
    constructor(props) {
        super(props);
        this.state = {
            value: '',
        };
    }

    handleChange(e) {
        const { value } = e.target;
        const { question } = this.props;

        this.setState({
            value: value
        });

        this.props.handleChange(e.target, question, value);
    }

    render() {
        const { question } = this.props;
        const { choices } = question;

        return (
            <div className="table-row">
                <label className="table-cell" style={ { width: '30%' } }> { question.title } { question.unit && <em>({ question.unit })</em> }</label>
                { choices.map((value, index) => {
                    return <div key={index} className="radio form-check form-check-inline">
                        <label className="table-cell" >
                            <input
                                type="radio"
                                name={ question.name }
                                id={ question.id }
                                value={ value }
                                onChange={ this.handleChange.bind(this)}
                                checked={ value === this.state.value }
                                />
                            { value }
                        </label>
                    </div>
                }) }
            </div>
        );
    }
}

RadioMatrixRow.defaultProps = {
    required: true,
};

RadioMatrixRow.propTypes = {
    handleChange: PropTypes.func.isRequired,
    question: Question.propTypes(true).isRequired,
    required: PropTypes.bool,

    matrixState: PropTypes.arrayOf([
        PropTypes.bool.isRequired,
        PropTypes.bool.isRequired,
    ]).isRequired,
};

export default RadioMatrixRow;
