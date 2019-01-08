import React, { Component } from 'react';
import PropTypes from 'prop-types';

const Thead = function(props) {

    const { question } = props;
    const { choices } = question;

    return(
        <tr>
            <th>{ question.unit && <em>({ question.unit })</em> }</th>
            {
                choices.map((value, index) => <th key={ index }>{ value }</th>)
            }
        </tr>
    );
};

Thead.propTypes = {
    question: PropTypes.shape({
        id: PropTypes.string.isRequired,
        name: PropTypes.string.isRequired,
        title: PropTypes.string.isRequired,
        title_text: PropTypes.string.isRequired,
        type: PropTypes.string.isRequired,
        unit: PropTypes.string.isRequired,
        // eunum
        choices: PropTypes.array.isRequired,
    }),
};


class Row  extends Component {

   constructor(props) {
        super(props);

        this.state = {
            values: {},
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

        return(
            <tr>
                <td>{ question.title }</td>
                {
                    choices.map((val, index) => <td key={ index }>
                        <input
                            type="radio"
                            name={ question.name }
                            id={ question.id }
                            value={ val }
                            onChange={ this.handleChange.bind(this) }
                        />
                    </td>)
                }
            </tr>
        );
    }
}

Row.defaultProps = {
    checked: false,
};


Row.propTypes = {
    question: PropTypes.shape({
        id: PropTypes.string.isRequired,
        name: PropTypes.string.isRequired,
        title: PropTypes.string.isRequired,
        title_text: PropTypes.string.isRequired,
        type: PropTypes.string.isRequired,
        unit: PropTypes.string.isRequired,
        // eunum
        choices: PropTypes.array.isRequired,
    }),
    handleChange: PropTypes.func.isRequired,
};


class RadioMatrix extends Component {

    constructor(props) {
        super(props);

        this.state = {
            values: {},
        };
    }

    handleChange(question, event) {
        event.preventDefault();
        const { values } = this.state;
        values[question.id] = event.target.value;

        this.setState({
            values
        });

        const args = [event.target].concat(Object.values(values));
        this.props.handleChange.apply(null, args);
    }

    render() {
        const { questionGroup } =  this.props;
        const first = questionGroup[0] || null;

        return (
            <div className="form-group">
                <div className="table-responsive">
                <table className="table table-sm table-hover">
                    <thead>
                        <Thead question={ first } />
                    </thead>
                    <tbody>
                        { questionGroup.map(question => <Row
                                key={ question.id }
                                question={ question }
                                handleChange={ this.props.handleChange }
                        />) }
                    </tbody>
                </table>
                </div>
            </div>
        );
    }
}

RadioMatrix.defaultProps = {
};

RadioMatrix.propTypes = {
    handleChange: PropTypes.func.isRequired,
    questionGroup: PropTypes.arrayOf(
        PropTypes.shape({
            id: PropTypes.string.isRequired,
            name: PropTypes.string.isRequired,
            title: PropTypes.string.isRequired,
            title_text: PropTypes.string.isRequired,
            type: PropTypes.string.isRequired,
            unit: PropTypes.string.isRequired,
            // eunum
            choices: PropTypes.array.isRequired,
        })
    ),
};

export default RadioMatrix;
