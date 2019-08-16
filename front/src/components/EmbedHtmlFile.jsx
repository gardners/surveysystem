import React, { Component } from 'react';
import PropTypes from 'prop-types';

import InnerHtml from './InnerHtml';

class EmbedHtmlFile extends Component {

    constructor(props) {
        super(props);
        this.state = {
            loading: true,
            contents: '',
            error: null,
        }
    }

    componentDidMount() {
        const { src, title } = this.props;

        fetch(src)
        .then(response => response.text())
        .then(contents => this.setState({
            contents,
            loading: false,
            error: null,
        }))
        .catch((err) => this.setState({
            loading: false,
            error: err //new Error(`Not found: Unable to import content for ${title}`),
        }));
    }

    render () {
        const { title } = this.props;
        const { contents, loading, error } = this.state;

        return (
            <React.Fragment>
                <h1>{ title }</h1>
                { loading && <div className="text-primary text-center m-2 p-2" style={ { fontSize: '2em' } }><i className="fas fa-circle-notch fa-spin"></i>{ loading }</div> }
                { error && <div className="text-danger m-2 p-2">{ error.toString() }</div> }
                { contents && <InnerHtml htmlContent={ contents } /> }
            </React.Fragment>
        );
    }
};

EmbedHtmlFile.defaultProps = {
    src: '',
};

EmbedHtmlFile.propTypes = {
   title: PropTypes.string.isRequired,
   src: PropTypes.string,
};

export default EmbedHtmlFile;
